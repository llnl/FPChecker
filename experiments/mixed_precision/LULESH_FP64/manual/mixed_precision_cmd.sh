#!/usr/bin/env bash

set -euo pipefail

LULESH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SIZE="${SIZE:-100}"
ITERATIONS="${ITERATIONS:-20}"
COMPILER="${CXX:-clang++}"
export CCACHE_DISABLE=1

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/lulesh-fp64-manual.XXXXXX")"
STAGE_DIR="$TMP_DIR/src"
mkdir -p "$STAGE_DIR"
trap 'rm -rf "$TMP_DIR"' EXIT

declare -A PRECISION_MODE=(
  [DOUBLE]=2
  [MIXED]=3
  [SINGLE]=1
)

declare -A ENERGY
declare -A FOM
declare -A ELAPSED
declare -A MAX_ABS_DIFF

while [ "$#" -gt 0 ]; do
  case "$1" in
    --mode)
      shift 2
      ;;
    --mode=*)
      shift
      ;;
    -h|--help)
      printf 'Usage: ./compare_precision.sh [-- LULESH_ARGS...]\n'
      printf 'Default LULESH args: -s %s -i %s\n' "$SIZE" "$ITERATIONS"
      exit 0
      ;;
    --)
      shift
      break
      ;;
    *)
      break
      ;;
  esac
done

if [ "$#" -gt 0 ]; then
  RUN_ARGS=("$@")
else
  RUN_ARGS=(-s "$SIZE" -i "$ITERATIONS")
fi

find_input_file() {
  local file=$1

  if [ -f "$LULESH_DIR/$file" ]; then
    printf '%s/%s\n' "$LULESH_DIR" "$file"
    return 0
  fi

  printf 'missing required source file: %s\n' "$file" >&2
  return 1
}

stage_sources() {
  local file

  for file in \
    lulesh.h \
    lulesh_tuple.h \
    lulesh.cc \
    lulesh-comm.cc \
    lulesh-viz.cc \
    lulesh-util.cc \
    lulesh-init.cc; do
    cp "$(find_input_file "$file")" "$STAGE_DIR/$file"
  done

  sed -i \
    -e 's/std::scientific << std::setprecision(6)/std::scientific << std::setprecision(17)/' \
    -e 's/std::setw(12) << locDom.e(ElemId)/std::setw(24) << locDom.e(ElemId)/' \
    -e 's/std::setprecision(2)/std::setprecision(6)/' \
    "$STAGE_DIR/lulesh-util.cc"
}

build_config() {
  local config=$1
  local exe="$TMP_DIR/lulesh_$config"

  if ! "$COMPILER" \
    -DUSE_MPI=0 \
    -DPRECISION_MODE="${PRECISION_MODE[$config]}" \
    -DFPC_LULESH_DISABLE_INJECTION \
    -g -O2 -I"$STAGE_DIR" -Wall \
    -Wno-reorder -Wno-unknown-pragmas \
    "$STAGE_DIR/lulesh.cc" \
    "$STAGE_DIR/lulesh-comm.cc" \
    "$STAGE_DIR/lulesh-viz.cc" \
    "$STAGE_DIR/lulesh-util.cc" \
    "$STAGE_DIR/lulesh-init.cc" \
    -lm -o "$exe" >"$TMP_DIR/build_$config.log" 2>&1; then
    cat "$TMP_DIR/build_$config.log" >&2
    return 1
  fi

  printf '%s\n' "$exe"
}

extract_value() {
  local pattern=$1
  awk -F'= *' -v pattern="$pattern" '$0 ~ pattern {print $2}' | awk '{print $1}'
}

stage_sources

for config in DOUBLE MIXED SINGLE; do
  exe="$(build_config "$config")"
  output="$("$exe" "${RUN_ARGS[@]}" 2>&1)"
  ENERGY[$config]="$(extract_value "Final Origin Energy" <<< "$output")"
  FOM[$config]="$(extract_value "FOM" <<< "$output")"
  ELAPSED[$config]="$(extract_value "Elapsed time" <<< "$output")"
  MAX_ABS_DIFF[$config]="$(extract_value "MaxAbsDiff" <<< "$output")"
done

printf 'INPUT: %s\n' "${RUN_ARGS[*]}"
printf '%-8s %-12s %-14s %-14s %-16s %-28s %-14s\n' \
  "Config" "Elapsed(s)" "FOM" "Speedup" "FasterVsFP64" "Final Energy Relative Error" "MaxAbsDiff"
printf '%-8s %-12s %-14s %-14s %-16s %-28s %-14s\n' \
  "------" "----------" "---" "-------" "------------" "---------------------------" "----------"

for config in SINGLE MIXED DOUBLE; do
  speedup="$(awk -v fom="${FOM[$config]}" -v ref="${FOM[DOUBLE]}" 'BEGIN {
    if (ref == 0) print "N/A"; else printf "%.6fx", fom / ref
  }')"

  percent_faster="$(awk -v fom="${FOM[$config]}" -v ref="${FOM[DOUBLE]}" 'BEGIN {
    if (ref == 0) {
      print "N/A"
    } else {
      printf "%+.2f%%", ((fom / ref) - 1.0) * 100.0
    }
  }')"

  energy_error="$(awk -v energy="${ENERGY[$config]}" -v ref="${ENERGY[DOUBLE]}" 'BEGIN {
    if (ref == 0) {
      print "N/A"
    } else {
      printf "%.12e", (ref > energy ? ref - energy : energy - ref) / (ref < 0 ? -ref : ref)
    }
  }')"

  printf '%-8s %-12s %-14s %-14s %-16s %-28s %-14s\n' \
    "$config" "${ELAPSED[$config]}" "${FOM[$config]}" "$speedup" "$percent_faster" "$energy_error" "${MAX_ABS_DIFF[$config]}"
done
