#!/usr/bin/env bash
# run_experiments.sh -- ground truth, three tools, scoring, comparison.
#
#   ./run_experiments.sh                       # everything (QuickSilver dominates)
#   ./run_experiments.sh --quick               # no QuickSilver, no NAS SP
#   ./run_experiments.sh --tools fpchecker,nsan
#   ./run_experiments.sh --skip-gt             # reuse an existing census
#
# Per-tool results stay under $EXP/*_experiments/; scorer JSONs, tables and
# compare.txt go to $OUT.
set -uo pipefail

FPC_SRC="${FPC_SRC:-/opt/cgo2026_artifact/fpchecker_bf}"
EXP="${EXP:-$FPC_SRC/cpu_checking/error_analysis/branch_flip/experiments}"
EXPECTED="$FPC_SRC/cpu_checking/error_analysis/branch_flip/expected"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="${OUT:-$HERE/results}"

QUICK=0; SKIP_GT=0; TOOLS="fpchecker,eftsan,nsan"
while [ $# -gt 0 ]; do
  case "$1" in
    --quick)   QUICK=1 ;;
    --skip-gt) SKIP_GT=1 ;;
    --tools)   shift; TOOLS="$1" ;;
    -h|--help) sed -n '2,10p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1"; exit 2 ;;
  esac
  shift
done

mkdir -p "$OUT"
LOG="$OUT/run.log"
: > "$LOG"
hr()  { printf '\n===== %s =====\n' "$1" | tee -a "$LOG"; }
run() { printf '$ %s\n' "$*" | tee -a "$LOG"; "$@" 2>&1 | tee -a "$LOG"; return "${PIPESTATUS[0]}"; }
want() { case ",$TOOLS," in *",$1,"*) return 0 ;; *) return 1 ;; esac; }

NAS_BENCHES="bt cg ep is lu mg sp"
if [ "$QUICK" = 1 ]; then NAS_BENCHES="bt cg ep is lu mg"; fi

# ---------------------------------------------------------------- ground truth
if [ "$SKIP_GT" = 0 ]; then
  hr "brtrace census"
  source activate_fpchecker_env.sh >/dev/null
  cd "$EXP/gt_experiments"
  run ./run_lulesh_brtrace.py
  run ./run_amg_brtrace.py
  [ "$QUICK" = 1 ] || run ./run_quicksilver_brtrace.py
  run ./run_nas_brtrace.py -b $NAS_BENCHES
fi

# ---------------------------------------------------------------- FPChecker
if want fpchecker; then
  hr "FPChecker"
  source activate_fpchecker_env.sh >/dev/null
  cd "$EXP/fpchecker_experiments"
  run ./run_lulesh_fpchecker.py --bf-mode both
  run ./run_amg_fpchecker.py --bf-mode both
  [ "$QUICK" = 1 ] || run ./run_quicksilver_fpchecker.py --bf-mode both
  run ./run_nas_fpchecker.py --bf-mode both -b $NAS_BENCHES
  cd "$EXP/gt_experiments"
  run ./fpc_exact_metrics.py --rule both --json "$OUT/fpc_metrics.json" --text "$OUT/fpc_metrics.txt"
fi

# ---------------------------------------------------------------- EFTSanitizer
if want eftsan; then
  hr "EFTSanitizer"
  source activate_eftsan_env.sh >/dev/null
  cd "$EXP/eftsan_experiments"
  run ./run_lulesh_eftsan.py
  run ./run_amg_eftsan.py
  [ "$QUICK" = 1 ] || run ./run_quicksilver_eftsan.py
  for b in $NAS_BENCHES; do run ./run_nas_eftsan.py "$b"; done
  cd "$EXP/gt_experiments"
  run ./eftsan_exact_metrics.py --json "$OUT/eftsan_metrics.json" --text "$OUT/eftsan_metrics.txt"
fi

# ---------------------------------------------------------------- NSan
if want nsan; then
  hr "NSan"
  source activate_nsan_env.sh >/dev/null
  cd "$EXP/nsan_experiments"
  run ./run_lulesh_nsan.py
  run ./run_amg_nsan.py
  [ "$QUICK" = 1 ] || run ./run_quicksilver_nsan.py
  run ./run_nas_nsan.py -b $NAS_BENCHES
  cd "$EXP/gt_experiments"
  run ./nsan_exact_metrics.py --json "$OUT/nsan_metrics.json" --text "$OUT/nsan_metrics.txt"
fi

# ---------------------------------------------------------------- tables
hr "tables and comparison with expected results"
source activate_fpchecker_env.sh >/dev/null
run python3 "$HERE/branch_flip_tables.py" --main --full --pdf --results "$OUT" --expected "$EXPECTED"
echo
echo "tables under $OUT, full log in $LOG"