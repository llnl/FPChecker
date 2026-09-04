#!/usr/bin/env bash
#
# brtrace_run.sh - generic fp32-vs-fp64 branch-flip runner.
#
# Knows NOTHING about any specific benchmark. You tell it, via flags, how to
# build the two variants and how to run them; it instruments, runs both
# single-threaded, and diffs on (module_id, site_id).
#
# It supports two source models:
#   * two source DIRECTORIES that differ only in precision, or
#   * a single source tree built twice with different flags/defines
#     (same tree built twice with different flags/defines).
#
# ---------------------------------------------------------------------------
# USAGE
#   brtrace_run.sh \
#       --brx      DIR          # brtrace dir (plugin, runtime, diff). default: script dir
#       --cc       CMD          # compiler (clang or clang++). default: clang
#       --out      DIR          # where to write traces/report. default: cwd
#       --fp32-build 'CMD'      # shell command that builds the fp32 binary (see vars)
#       --fp64-build 'CMD'      # shell command that builds the fp64 binary
#       --fp32-run   'CMD'      # shell command that runs the fp32 binary
#       --fp64-run   'CMD'      # shell command that runs the fp64 binary
#       --mods     DIR          # dir holding .brmods/.brsites (for source mapping)
#       [--label   NAME]        # tag for output files. default: brtrace
#
# The build/run commands are arbitrary shell, so this runner has zero benchmark
# knowledge. Inside them these variables are exported and available:
#       $BRX_PLUGIN   absolute path to libBranchTrace_mtu.so
#       $BRX_RT       absolute path to brtrace_runtime_mtu.o
#       $BRX_CFLAGS   "-O0 -g -fpass-plugin=$BRX_PLUGIN"  (add to your compile)
#       $BRTRACE_OUT  set per-run to the correct trace file (use it in --*-run)
#
# EXAMPLE (single source each):
#   brtrace_run.sh --out . --label sp \
#     --fp32-build 'clang $BRX_CFLAGS fp32/prog.c $BRX_RT -lm -o prog32' \
#     --fp64-build 'clang $BRX_CFLAGS fp64/prog.c $BRX_RT -lm -o prog64' \
#     --fp32-run   'OMP_NUM_THREADS=1 ./prog32' \
#     --fp64-run   'OMP_NUM_THREADS=1 ./prog64' \
#     --mods fp64
#
# EXAMPLE (multi-TU, two dirs):
#   SRCS='a.cc b.cc c.cc'
#   brtrace_run.sh --out . --label myprog --cc clang++ \
#     --fp32-build "cd fp32 && clang++ \$BRX_CFLAGS $SRCS \$BRX_RT -lm -o x.brx" \
#     --fp64-build "cd fp64 && clang++ \$BRX_CFLAGS $SRCS \$BRX_RT -lm -o x.brx" \
#     --fp32-run   'cd fp32 && OMP_NUM_THREADS=1 ./prog.brx <args>' \
#     --fp64-run   'cd fp64 && OMP_NUM_THREADS=1 ./prog.brx <args>' \
#     --mods fp64
# ---------------------------------------------------------------------------
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BRX="$SCRIPT_DIR"
CC="clang"
OUT="$(pwd)"
LABEL="brtrace"
FP32_BUILD="" FP64_BUILD="" FP32_RUN="" FP64_RUN="" MODS=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --brx)        BRX="$2"; shift 2;;
    --cc)         CC="$2"; shift 2;;
    --out)        OUT="$2"; shift 2;;
    --label)      LABEL="$2"; shift 2;;
    --fp32-build) FP32_BUILD="$2"; shift 2;;
    --fp64-build) FP64_BUILD="$2"; shift 2;;
    --fp32-run)   FP32_RUN="$2"; shift 2;;
    --fp64-run)   FP64_RUN="$2"; shift 2;;
    --mods)       MODS="$2"; shift 2;;
    -h|--help)    sed -n '2,12p' "$0"; exit 0;;
    *) echo "unknown arg: $1" >&2; exit 2;;
  esac
done

export BRX_PLUGIN="$BRX/libBranchTrace_mtu.so"
export BRX_RT="$BRX/brtrace_runtime_mtu.o"
export BRX_CFLAGS="-O0 -g -fpass-plugin=$BRX_PLUGIN"
DIFF="$BRX/tools/brtrace_diff_mtu.py"

for f in "$BRX_PLUGIN" "$BRX_RT" "$DIFF"; do
  [[ -e "$f" ]] || { echo "ERROR: missing $f  (run build_mtu.sh in $BRX)"; exit 1; }
done
for req in FP32_BUILD FP64_BUILD FP32_RUN FP64_RUN; do
  [[ -n "${!req}" ]] || { echo "ERROR: --${req,,//_/-} is required"; exit 2; }
done

mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
FP32_OUT="$OUT/${LABEL}_fp32.out"
FP64_OUT="$OUT/${LABEL}_fp64.out"
CSV="$OUT/${LABEL}_flips.csv"
REPORT="$OUT/${LABEL}_report.txt"

echo "== brtrace: $LABEL =="
echo "brx=$BRX  out=$OUT"

echo "  [build fp32]"; bash -c "$FP32_BUILD" 2>&1 | sed -n 's/^\[BranchTrace\]/    &/p'
echo "  [build fp64]"; bash -c "$FP64_BUILD" 2>&1 | sed -n 's/^\[BranchTrace\]/    &/p'

echo "  [run fp32]"; BRTRACE_OUT="$FP32_OUT" bash -c "$FP32_RUN" >/dev/null
echo "  [run fp64]"; BRTRACE_OUT="$FP64_OUT" bash -c "$FP64_RUN" >/dev/null

echo "  [diff]"
MODS_ARG=(); [[ -n "$MODS" ]] && MODS_ARG=(--mods "$MODS")
python3 "$DIFF" "$FP32_OUT" "$FP64_OUT" "${MODS_ARG[@]}" --csv "$CSV" | tee "$REPORT"
rc=${PIPESTATUS[0]}

echo
echo "artifacts: $FP32_OUT  $FP64_OUT  $CSV  $REPORT"
exit $rc
