#!/usr/bin/env bash
#
# run_nas.sh — build, run, and diff brtrace on NPB single-file fp32/fp64 pairs.
#
# Assumes each benchmark lives as:
#     <nas>/<B>/<B>_fp32/<B>.c
#     <nas>/<B>/<B>_fp64/<B>.c
# (the SP/BT/LU layout in this tree). Produces, per benchmark:
#     <B>/fp32.out          fp32 branch trace
#     <B>/fp64.out          fp64 branch trace
#     <B>/<B>_flips.csv      per-flip rows (site, fp32/fp64 taken, location)
#     <B>/<B>_report.txt     saved diff report
#
# Usage:
#     ./run_nas.sh                 # all benchmarks found (sp bt lu ...)
#     ./run_nas.sh sp lu           # only the named ones
#     BRX=/path/to/brtrace ./run_nas.sh sp
#
# Env:
#     BRX   path to the brtrace dir (has libBranchTrace.so, brtrace_runtime.o,
#           tools/brtrace_diff.py). Default: this script's own directory's
#           parent guess, else must be set.
#     CC    C compiler (default: clang). Must be the LLVM-19 clang whose
#           llvm-config built the plugin.
#     EXTRA_CFLAGS  appended to every compile (e.g. "-fopenmp -DFOO").
#
# NOTES
#  - Runs single-threaded (OMP_NUM_THREADS=1) so the trace is deterministic and
#    the two builds stay lock-step. Do not remove that without reason.
#  - Both builds of a benchmark MUST print the same "[BranchTrace] instrumented
#    N branch sites". The script checks this and aborts the benchmark if they
#    differ (misaligned IDs => meaningless diff).
#  - The .brsites site table is written next to each source as <B>.c.brsites.
#
set -uo pipefail

# ---- locate brtrace dir --------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
: "${BRX:=$SCRIPT_DIR}"
: "${CC:=clang}"
: "${EXTRA_CFLAGS:=}"

PLUGIN="$BRX/libBranchTrace.so"
RT="$BRX/brtrace_runtime.o"
DIFF="$BRX/tools/brtrace_diff.py"

for f in "$PLUGIN" "$RT" "$DIFF"; do
  if [[ ! -e "$f" ]]; then
    echo "ERROR: missing $f" >&2
    echo "       build the plugin first:  (cd $BRX && bash build.sh)" >&2
    exit 1
  fi
done

# ---- where the benchmarks live ------------------------------------------
# Default: current directory is the 'nas' dir containing sp/ bt/ lu/.
NAS_DIR="$(pwd)"

# ---- which benchmarks ----------------------------------------------------
if [[ $# -gt 0 ]]; then
  BENCHES=("$@")
else
  BENCHES=()
  for d in "$NAS_DIR"/*/; do
    b="$(basename "$d")"
    [[ -f "$NAS_DIR/$b/${b}_fp32/${b}.c" && -f "$NAS_DIR/$b/${b}_fp64/${b}.c" ]] \
      && BENCHES+=("$b")
  done
fi

if [[ ${#BENCHES[@]} -eq 0 ]]; then
  echo "No fp32/fp64 benchmark pairs found under $NAS_DIR" >&2
  exit 1
fi

echo "brtrace dir : $BRX"
echo "compiler    : $CC ($($CC --version 2>/dev/null | head -1))"
echo "benchmarks  : ${BENCHES[*]}"
[[ -n "$EXTRA_CFLAGS" ]] && echo "extra cflags: $EXTRA_CFLAGS"
echo

SUMMARY="$NAS_DIR/BRTRACE_SUMMARY.txt"
: > "$SUMMARY"

overall_rc=0
for B in "${BENCHES[@]}"; do
  echo "=================== $B ==================="
  SRC32="$NAS_DIR/$B/${B}_fp32/${B}.c"
  SRC64="$NAS_DIR/$B/${B}_fp64/${B}.c"
  if [[ ! -f "$SRC32" || ! -f "$SRC64" ]]; then
    echo "  skip: missing $SRC32 or $SRC64"
    continue
  fi

  BIN32="$NAS_DIR/$B/${B}_fp32/${B}.brx"
  BIN64="$NAS_DIR/$B/${B}_fp64/${B}.brx"
  OUT32="$NAS_DIR/$B/fp32.out"
  OUT64="$NAS_DIR/$B/fp64.out"
  SITES="$NAS_DIR/$B/${B}_fp64/${B}.c.brsites"
  CSV="$NAS_DIR/$B/${B}_flips.csv"
  REPORT="$NAS_DIR/$B/${B}_report.txt"

  # --- build fp32 (capture the site count line) ---
  echo "  [build fp32]"
  N32=$($CC -O0 -g -fpass-plugin="$PLUGIN" $EXTRA_CFLAGS \
        "$SRC32" "$RT" -lm -o "$BIN32" 2>&1 \
        | tee /dev/stderr | sed -n 's/.*instrumented \([0-9]*\) branch sites/\1/p' | tail -1)
  if [[ ! -x "$BIN32" ]]; then echo "  ERROR: fp32 build failed"; overall_rc=1; continue; fi

  # --- build fp64 ---
  echo "  [build fp64]"
  N64=$($CC -O0 -g -fpass-plugin="$PLUGIN" $EXTRA_CFLAGS \
        "$SRC64" "$RT" -lm -o "$BIN64" 2>&1 \
        | tee /dev/stderr | sed -n 's/.*instrumented \([0-9]*\) branch sites/\1/p' | tail -1)
  if [[ ! -x "$BIN64" ]]; then echo "  ERROR: fp64 build failed"; overall_rc=1; continue; fi

  # --- site-count alignment guard ---
  if [[ -n "$N32" && -n "$N64" && "$N32" != "$N64" ]]; then
    echo "  ERROR: site count mismatch (fp32=$N32 fp64=$N64) — IDs won't align, skipping diff"
    echo "$B: BUILD-MISMATCH fp32=$N32 fp64=$N64" >> "$SUMMARY"
    overall_rc=1
    continue
  fi
  echo "  site count: $N64 (matched)"

  # --- run both, single-threaded ---
  echo "  [run fp32]"
  ( cd "$(dirname "$BIN32")" && OMP_NUM_THREADS=1 BRTRACE_OUT="$OUT32" "./$(basename "$BIN32")" >/dev/null )
  echo "  [run fp64]"
  ( cd "$(dirname "$BIN64")" && OMP_NUM_THREADS=1 BRTRACE_OUT="$OUT64" "./$(basename "$BIN64")" >/dev/null )

  # --- diff ---
  echo "  [diff]"
  SITES_ARG=()
  [[ -f "$SITES" ]] && SITES_ARG=(--sites "$SITES")
  python3 "$DIFF" "$OUT32" "$OUT64" "${SITES_ARG[@]}" --csv "$CSV" | tee "$REPORT"
  rc=${PIPESTATUS[0]}

  # --- one-line summary (flip count from the report) ---
  flips=$(sed -n 's/^FLIP EVENTS: \([0-9]*\).*/\1/p' "$REPORT" | tail -1)
  [[ -z "$flips" ]] && flips=0
  sites=$(sed -n 's/.*across \([0-9]*\) distinct sites.*/\1/p' "$REPORT" | tail -1)
  [[ -z "$sites" ]] && sites=0
  echo "$B: sites=$N64 events=$(wc -c < "$OUT64" | awk '{print $1/8}') flips=$flips flip_sites=$sites" >> "$SUMMARY"
  echo
done

echo "=================== summary ==================="
cat "$SUMMARY"
echo
echo "Per-benchmark artifacts: <B>/fp32.out <B>/fp64.out <B>/<B>_flips.csv <B>/<B>_report.txt"
echo "Summary: $SUMMARY"
exit $overall_rc
