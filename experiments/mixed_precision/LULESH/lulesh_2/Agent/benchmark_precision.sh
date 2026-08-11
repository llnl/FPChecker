#!/usr/bin/env bash
set -euo pipefail

SIZE=${SIZE:-100}
ITERATIONS=${ITERATIONS:-20}

if [ "$#" -gt 0 ]; then
  RUN_ARGS=("$@")
else
  RUN_ARGS=(-s "$SIZE" -i "$ITERATIONS")
fi

declare -A elapsed
declare -A fom
declare -A energy
declare -A maxabs

extract_metric() {
  local label="$1"
  awk -F= -v label="$label" '
    index($0, label) {
      value=$2
      sub(/\(.*/, "", value)
      gsub(/^[ \t]+|[ \t]+$/, "", value)
      print value
      exit
    }
  '
}

run_config() {
  local config="$1"
  local output
  local build_output

  if ! build_output=$(make clean 2>&1); then
    printf '%s\n' "$build_output"
    return 1
  fi

  if ! build_output=$(make PRECISION="$config" 2>&1); then
    printf '%s\n' "$build_output"
    return 1
  fi

  output=$(./lulesh.0 "${RUN_ARGS[@]}")

  elapsed[$config]=$(printf '%s\n' "$output" | extract_metric "Elapsed time")
  fom[$config]=$(printf '%s\n' "$output" | extract_metric "FOM")
  energy[$config]=$(printf '%s\n' "$output" | extract_metric "Final Origin Energy")
  maxabs[$config]=$(printf '%s\n' "$output" | extract_metric "MaxAbsDiff")
}

calc_speedup() {
  awk -v val="$1" -v ref="$2" 'BEGIN { printf "%.6fx", val/ref }'
}

calc_faster_percent() {
  awk -v val="$1" -v ref="$2" 'BEGIN { printf "%+.2f%%", ((val/ref)-1.0)*100.0 }'
}

calc_energy_error() {
  awk -v ref="$1" -v val="$2" '
    BEGIN {
      diff = ref - val
      if (diff < 0) diff = -diff
      denom = ref
      if (denom < 0) denom = -denom
      if (denom == 0) {
        printf "nan"
      } else {
        printf "%.12e", diff/denom
      }
    }
  '
}

echo "Running native LULESH builds with args: ${RUN_ARGS[*]}"
echo "FPChecker is not used for these timing runs."
echo

for config in SINGLE MIXED DOUBLE; do
  run_config "$config"
done

baseline_fom=${fom[DOUBLE]}
baseline_energy=${energy[DOUBLE]}

printf "%-8s %-12s %-14s %-14s %-14s %-29s %-14s\n" \
  "Config" "Elapsed(s)" "FOM" "Speedup" "FasterVsFP64" \
  "Final Energy Relative Error" "MaxAbsDiff"
printf "%-8s %-12s %-14s %-14s %-14s %-29s %-14s\n" \
  "------" "----------" "---" "-------" "------------" \
  "---------------------------" "----------"

for config in SINGLE MIXED DOUBLE; do
  printf "%-8s %-12s %-14s %-14s %-14s %-29s %-14s\n" \
    "$config" \
    "${elapsed[$config]}" \
    "${fom[$config]}" \
    "$(calc_speedup "${fom[$config]}" "$baseline_fom")" \
    "$(calc_faster_percent "${fom[$config]}" "$baseline_fom")" \
    "$(calc_energy_error "$baseline_energy" "${energy[$config]}")" \
    "${maxabs[$config]}"
done
