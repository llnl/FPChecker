#!/bin/bash
# run_adjudication.sh -- score every EFTSan cell against its brtrace census.
#
#   ./run_adjudication.sh
#
# Edit CENSUS_DIR and the COV table below to match your tree, then run.
# Produces adj/<bench>_<prec>.{txt,json} and the aggregated tables.
#
# The COVERAGE numbers are NOT discoverable from the sites.txt file -- they
# come from the brtrace diff report's "lock-step compared ... (N% of longer
# trace)" line. They must be passed in, because a cell scored without one is
# silently treated as exact when it may not be.
set -u

ROOT="/usr/workspace/das9/fpchecker_bf/cpu_checking/error_analysis/branch_flip"
EXP="$ROOT/experiments/eftsan_experiments"
CENSUS_DIR="$ROOT/experiments/brtrace_experiments"   # <-- adjust
OUT="$EXP/adj"
mkdir -p "$OUT"

# benchmark  precision  coverage%  census-sites-file
#   coverage from the brtrace report; census path relative to CENSUS_DIR
CELLS="
LULESH      fp32  21.62  lulesh/lulesh_fp32vs64.sites.txt
LULESH      fp64 100.00  lulesh/lulesh_ldvs64.sites.txt
AMG         fp32  13.76  amg/amg_fp32vs64.sites.txt
AMG         fp64 100.00  amg/amg_ldvs64.sites.txt
QuickSilver fp64 100.00  quicksilver/qs_ldvs64.sites.txt
BT          fp32 100.00  nas/bt_fp32vs64.sites.txt
BT          fp64 100.00  nas/bt_ldvs64.sites.txt
CG          fp32 100.00  nas/cg_fp32vs64.sites.txt
CG          fp64 100.00  nas/cg_ldvs64.sites.txt
EP          fp32 100.00  nas/ep_fp32vs64.sites.txt
EP          fp64 100.00  nas/ep_ldvs64.sites.txt
LU          fp32 100.00  nas/lu_fp32vs64.sites.txt
LU          fp64 100.00  nas/lu_ldvs64.sites.txt
MG          fp32 100.00  nas/mg_fp32vs64.sites.txt
MG          fp64 100.00  nas/mg_ldvs64.sites.txt
SP          fp32 100.00  nas/sp_fp32vs64.sites.txt
SP          fp64 100.00  nas/sp_ldvs64.sites.txt
IS          fp32 100.00  nas/is_fp32vs64.sites.txt
IS          fp64 100.00  nas/is_ldvs64.sites.txt
"

# where each benchmark's results live, and the lowercase tag used in filenames
resdir () {
  case "$1" in
    LULESH)      echo "$EXP/lulesh/results/O0/$2         lulesh" ;;
    AMG)         echo "$EXP/amg/results/O0/$2            amg" ;;
    QuickSilver) echo "$EXP/quicksilver/results/O0/$2    qs" ;;
    *)           b=$(echo "$1" | tr 'A-Z' 'a-z')
                 echo "$EXP/nas/$b/results/O0/$2         $b" ;;
  esac
}

n=0; skipped=0
echo "$CELLS" | while read -r BENCH PREC COV CENSUS; do
  [ -z "${BENCH:-}" ] && continue
  read -r DIR TAG <<< "$(resdir "$BENCH" "$PREC")"
  CENSUS_PATH="$CENSUS_DIR/$CENSUS"
  UNIVERSE="$DIR/instrumented_sites.csv"
  DETECTED="$DIR/${TAG}_${PREC}_eftsan_summary.csv"

  miss=""
  [ -f "$CENSUS_PATH" ] || miss="$miss census"
  [ -f "$UNIVERSE" ]    || miss="$miss universe"
  [ -f "$DETECTED" ]    || miss="$miss detected"
  if [ -n "$miss" ]; then
    printf 'SKIP  %-12s %-5s  missing:%s\n' "$BENCH" "$PREC" "$miss"
    continue
  fi

  ./adjudicate_cell.py \
      --census   "$CENSUS_PATH" \
      --universe "$UNIVERSE" \
      --detected "$DETECTED" \
      --tool EFTSan --benchmark "$BENCH" --precision "$PREC" \
      --coverage "$COV" \
      --report "$OUT/${BENCH}_${PREC}.txt" \
      --json   "$OUT/${BENCH}_${PREC}.json" > /dev/null \
    && printf 'ok    %-12s %-5s\n' "$BENCH" "$PREC" \
    || printf 'FAIL  %-12s %-5s\n' "$BENCH" "$PREC"
done

echo
./build_tables.py "$OUT"/*.json \
    --text "$OUT/table.txt" --latex "$OUT/table.tex"
