#!/usr/bin/env bash
# run_experiments.sh -- ground truth, then NSan, EFTSan, FPChecker, each scored; then tables.
#
#   bash run_experiments.sh                       # everything (QuickSilver dominates)
#   bash run_experiments.sh --quick               # no QuickSilver, no NAS SP
#   bash run_experiments.sh --tools fpchecker,nsan
#   bash run_experiments.sh --bench amg           # one benchmark: lulesh amg quicksilver bt cg ep is lu mg sp
#   bash run_experiments.sh --skip-gt             # reuse an existing census
#
set -uo pipefail
export PYTHONUNBUFFERED=1

FPC_SRC="${FPC_SRC:-/opt/cgo2026_artifact/fpchecker_bf}"
EXP="${EXP:-$FPC_SRC/cpu_checking/error_analysis/branch_flip/experiments}"
EXPECTED="$FPC_SRC/cpu_checking/error_analysis/branch_flip/expected"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="${OUT:-$HERE/results}"

QUICK=0; SKIP_GT=0; TOOLS="nsan,eftsan,fpchecker"; BENCH=""
while [ $# -gt 0 ]; do
  case "$1" in
    --quick)   QUICK=1 ;;
    --skip-gt) SKIP_GT=1 ;;
    --tools)   shift; TOOLS="$1" ;;
    --bench)   shift; BENCH="$1" ;;
    -h|--help) sed -n '2,10p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1"; exit 2 ;;
  esac
  shift
done

mkdir -p "$OUT"
LOG="$OUT/run.log"
: > "$LOG"
T0=$(date +%s)

section() { printf '\n== %s\n' "$1" | tee -a "$LOG"; }
step() {
  local label="$1"; shift
  local t=$(date +%s)
  printf '   %-42s' "$label"
  printf '\n$ %s\n' "$*" >> "$LOG"
  if "$@" >> "$LOG" 2>&1; then
    printf 'done  %4ds\n' $(( $(date +%s) - t ))
  else
    printf 'FAILED (rc=%d, see %s)\n' $? "$LOG"
  fi
}
want() { case ",$TOOLS," in *",$1,"*) return 0 ;; *) return 1 ;; esac; }
elapsed() { printf '%dh%02dm' $(( ($(date +%s) - T0) / 3600 )) $(( (($(date +%s) - T0) % 3600) / 60 )); }

check() { printf '   %-42s' "$1"; if eval "$2"; then printf 'ok\n'; else printf 'MISSING\n'; MISSING=1; fi; }

NAS="bt cg ep is lu mg sp"
[ "$QUICK" = 1 ] && NAS="bt cg ep is lu mg"
DO_LULESH=1; DO_AMG=1; DO_QS=$(( QUICK == 0 ))
if [ -n "$BENCH" ]; then
  DO_LULESH=0; DO_AMG=0; DO_QS=0; NAS=""
  case "$BENCH" in
    lulesh) DO_LULESH=1 ;; amg) DO_AMG=1 ;; quicksilver) DO_QS=1 ;;
    bt|cg|ep|is|lu|mg|sp) NAS="$BENCH" ;;
    *) echo "unknown benchmark: $BENCH"; exit 2 ;;
  esac
  TABLE_ARGS="--benchmarks $BENCH"
else
  TABLE_ARGS=""
fi

MISSING=0
section "prerequisites  (host $(hostname -s))"
check "FPChecker install"        '[ -x "$FPC_SRC/install/bin/clang++-fpchecker" ] && [ -f "$FPC_SRC/install/src/FPC_SiteCounter.h" ]'
check "brtrace plugin + runtime" '[ -f "$EXP/gt_experiments/brtrace/libBranchTrace_mtu.so" ] && [ -f "$EXP/gt_experiments/brtrace/brtrace_runtime_mtu.o" ]'
if want nsan; then
  check "nsan plugin + shim"     '[ -f "$EXP/nsan_experiments/nsan/plugin/libNsanBFSites.so" ] && [ -f "$EXP/nsan_experiments/nsan/runtime/libnsan_bf.a" ]'
fi
if want eftsan; then
  source activate_eftsan_env.sh >/dev/null
  check "EFTSan pass"            '[ -f "$EFT_HOME/llvm_pass/build/EFTSan/libEFTSanitizer.so" ]'
  check "EFTSan runtime (clang 10)" '[ -f "$EFT_HOME/runtime/obj/libeftsanitizer.so" ] && strings "$EFT_HOME/runtime/obj/handleReal.o" | grep "clang version 10" >/dev/null'
  conda deactivate
fi
[ "$MISSING" = 0 ] || { echo "missing prerequisites; see above"; exit 1; }

if [ "$SKIP_GT" = 0 ]; then
  section "ground truth (brtrace census)"
  source activate_fpchecker_env.sh >/dev/null
  cd "$EXP/gt_experiments"
  [ "$DO_LULESH" = 1 ] && step "LULESH"         python3 run_lulesh_brtrace.py
  [ "$DO_AMG" = 1 ] && step "AMG"            python3 run_amg_brtrace.py
  [ "$DO_QS" = 1 ] && step "QuickSilver" python3 run_quicksilver_brtrace.py
  for b in $NAS; do step "NAS $b" python3 run_nas_brtrace.py -b "$b"; done
fi

if want nsan; then
  section "NSan  ($(elapsed) elapsed)"
  source activate_nsan_env.sh >/dev/null
  cd "$EXP/nsan_experiments"
  [ "$DO_LULESH" = 1 ] && step "LULESH"         python3 run_lulesh_nsan.py
  [ "$DO_AMG" = 1 ] && step "AMG"            python3 run_amg_nsan.py
  [ "$DO_QS" = 1 ] && step "QuickSilver" python3 run_quicksilver_nsan.py
  for b in $NAS; do step "NAS $b" python3 run_nas_nsan.py -b "$b"; done
  cd "$EXP/gt_experiments"
  step "scoring" python3 nsan_exact_metrics.py --json "$OUT/nsan_metrics.json" --text "$OUT/nsan_metrics.txt"
fi

if want eftsan; then
  section "EFTSanitizer  ($(elapsed) elapsed)"
  source activate_eftsan_env.sh >/dev/null
  cd "$EXP/eftsan_experiments"
  [ "$DO_LULESH" = 1 ] && step "LULESH"         python3 run_lulesh_eftsan.py
  [ "$DO_AMG" = 1 ] && step "AMG"            python3 run_amg_eftsan.py
  [ "$DO_QS" = 1 ] && step "QuickSilver" python3 run_quicksilver_eftsan.py
  for b in $NAS; do step "NAS $b" python3 run_nas_eftsan.py "$b"; done
  cd "$EXP/gt_experiments"
  step "scoring" python3 eftsan_exact_metrics.py --json "$OUT/eftsan_metrics.json" --text "$OUT/eftsan_metrics.txt"
fi

if want fpchecker; then
  section "FPChecker  ($(elapsed) elapsed)"
  source activate_fpchecker_env.sh >/dev/null
  cd "$EXP/fpchecker_experiments"
  [ "$DO_LULESH" = 1 ] && step "LULESH (interval + shadow, eta sweep)" python3 run_lulesh_fpchecker.py --bf-mode both
  [ "$DO_AMG" = 1 ] && step "AMG"            python3 run_amg_fpchecker.py --bf-mode both
  [ "$DO_QS" = 1 ] && step "QuickSilver" python3 run_quicksilver_fpchecker.py --bf-mode both
  for b in $NAS; do step "NAS $b" python3 run_nas_fpchecker.py --bf-mode both -b "$b"; done
  cd "$EXP/gt_experiments"
  step "scoring" python3 fpc_exact_metrics.py --rule both --json "$OUT/fpc_metrics.json" --text "$OUT/fpc_metrics.txt"
fi

section "tables and comparison with expected  ($(elapsed) elapsed)"
source activate_fpchecker_env.sh >/dev/null
python3 "$HERE/branch_flip_tables.py" --main --full --pdf --results "$OUT" --expected "$EXPECTED" $TABLE_ARGS 2>&1 | tee -a "$LOG"
echo
echo "results: $OUT   log: $LOG   total $(elapsed)"