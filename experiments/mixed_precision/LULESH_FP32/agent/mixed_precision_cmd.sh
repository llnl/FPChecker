#!/usr/bin/env bash
set -euo pipefail

LULESH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SIZE="${SIZE:-100}"
ITERATIONS="${ITERATIONS:-20}"
COMPILER="${CXX:-clang++}"
export CCACHE_DISABLE=1

activate_fpchecker_env_for_compiler() {
  if command -v "$COMPILER" >/dev/null 2>&1; then
    return 0
  fi

  if [ -n "${CXX:-}" ]; then
    printf "compiler not found: %s\n" "$COMPILER" >&2
    return 1
  fi

  local env_name prefix conda_sh
  for env_name in tutorial_env fpchecker_env; do
    for prefix in \
      "${CONDA_PREFIX:-}/../envs/$env_name" \
      "${CONDA_PREFIX:-}/../$env_name" \
      "$HOME/conda_env/$env_name" \
      "$HOME/miniconda3/envs/$env_name" \
      "$HOME/anaconda3/envs/$env_name" \
      "/opt/conda/envs/$env_name" \
      "/opt/miniconda3/envs/$env_name" \
      "/opt/anaconda3/envs/$env_name"; do
      if [ ! -d "$prefix" ]; then
        continue
      fi
      export CONDA_PREFIX="$(cd "$prefix" && pwd)"
      export CONDA_DEFAULT_ENV="$env_name"
      export PATH="$CONDA_PREFIX/bin:$PATH"
      if [ -d "$CONDA_PREFIX/lib" ]; then
        export LD_LIBRARY_PATH="$CONDA_PREFIX/lib:${LD_LIBRARY_PATH:-}"
      fi
      if command -v "$COMPILER" >/dev/null 2>&1; then
        return 0
      fi
    done
  done

  for conda_sh in \
    "${CONDA_SH:-}" \
    "${CONDA_PREFIX:-}/etc/profile.d/conda.sh" \
    "/opt/conda/etc/profile.d/conda.sh" \
    "/opt/miniconda3/etc/profile.d/conda.sh" \
    "$HOME/miniconda3/etc/profile.d/conda.sh" \
    "$HOME/anaconda3/etc/profile.d/conda.sh" \
    "/opt/anaconda3/etc/profile.d/conda.sh"; do
    if [ ! -f "$conda_sh" ]; then
      continue
    fi
    # shellcheck source=/dev/null
    source "$conda_sh"
    for env_name in tutorial_env fpchecker_env; do
      if conda activate "$env_name" >/dev/null 2>&1 && command -v "$COMPILER" >/dev/null 2>&1; then
        return 0
      fi
    done
  done

  printf "proper environment not found; clang++ is missing. Expected tutorial_env or fpchecker_env.\n" >&2
  return 1
}

activate_fpchecker_env_for_compiler

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
