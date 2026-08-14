#!/usr/bin/env bash
set -euo pipefail

LULESH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SIZE="${SIZE:-100}"
ITERATIONS="${ITERATIONS:-20}"
COMPILER="${CXX:-clang++}"
export CCACHE_DISABLE=1

if [ "$#" -gt 0 ]; then
  RUN_ARGS=("$@")
else
  RUN_ARGS=(-s "$SIZE" -i "$ITERATIONS")
fi

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/lulesh-fixed-mixed-precision.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT

declare -A PRECISION_MODE=(
  [SINGLE]=1
  [MIXED]=3
  [DOUBLE]=2
)

declare -A ENERGY
declare -A FOM
declare -A ELAPSED
declare -A MAX_ABS_DIFF

build_config() {
  local config="$1"
  local exe="$TMP_DIR/lulesh_$config"

  "$COMPILER" \
    -DUSE_MPI=0 \
    -DPRECISION_MODE="${PRECISION_MODE[$config]}" \
    -g -O2 -I"$LULESH_DIR" -Wall -Wno-reorder \
    "$LULESH_DIR/lulesh.cc" \
    "$LULESH_DIR/lulesh-comm.cc" \
    "$LULESH_DIR/lulesh-viz.cc" \
    "$LULESH_DIR/lulesh-util.cc" \
    "$LULESH_DIR/lulesh-init.cc" \
    -lm -o "$exe"

  printf '%s\n' "$exe"
}

extract_metric() {
  local label="$1"
  awk -F'= *' -v label="$label" '
    index($0, label) {
      print $2
      exit
    }
  ' | awk '{print $1}'
}

run_config() {
  local config="$1"
  local exe
  local output

  exe="$(build_config "$config")"
  output="$("$exe" "${RUN_ARGS[@]}")"

  ENERGY[$config]="$(printf '%s\n' "$output" | extract_metric "Final Origin Energy")"
  FOM[$config]="$(printf '%s\n' "$output" | extract_metric "FOM")"
  ELAPSED[$config]="$(printf '%s\n' "$output" | extract_metric "Elapsed time")"
  MAX_ABS_DIFF[$config]="$(printf '%s\n' "$output" | extract_metric "MaxAbsDiff")"
}

relative_error() {
  local value="$1"
  local reference="$2"

  awk -v value="$value" -v reference="$reference" '
    BEGIN {
      diff = reference - value
      if (diff < 0) diff = -diff

      denom = reference
      if (denom < 0) denom = -denom

      if (denom == 0) {
        print "N/A"
      } else {
        printf "%.12e", diff / denom
      }
    }
  '
}

speedup() {
  local value="$1"
  local reference="$2"

  awk -v value="$value" -v reference="$reference" '
    BEGIN {
      if (reference == 0) {
        print "N/A"
      } else {
        printf "%.6fx", value / reference
      }
    }
  '
}

faster_percent() {
  local value="$1"
  local reference="$2"

  awk -v value="$value" -v reference="$reference" '
    BEGIN {
      if (reference == 0) {
        print "N/A"
      } else {
        printf "%+.2f%%", ((value / reference) - 1.0) * 100.0
      }
    }
  '
}

echo "INPUT: ${RUN_ARGS[*]}"
echo

for config in SINGLE MIXED DOUBLE; do
  run_config "$config"
done

printf "%-8s %-12s %-14s %-14s %-16s %-28s %-14s\n" \
  "Config" "Elapsed(s)" "FOM" "Speedup" "FasterVsFP64" \
  "EnergyRelErrorVsFP64" "MaxAbsDiff"
printf "%-8s %-12s %-14s %-14s %-16s %-28s %-14s\n" \
  "------" "----------" "---" "-------" "------------" \
  "--------------------" "----------"

for config in SINGLE MIXED DOUBLE; do
  printf "%-8s %-12s %-14s %-14s %-16s %-28s %-14s\n" \
    "$config" \
    "${ELAPSED[$config]}" \
    "${FOM[$config]}" \
    "$(speedup "${FOM[$config]}" "${FOM[DOUBLE]}")" \
    "$(faster_percent "${FOM[$config]}" "${FOM[DOUBLE]}")" \
    "$(relative_error "${ENERGY[$config]}" "${ENERGY[DOUBLE]}")" \
    "${MAX_ABS_DIFF[$config]}"
done
