#!/bin/bash
# run_fpchecker_adjudication.sh -- convert every FPChecker cell and score it
# against the brtrace census, one table per eta.
#
# Run from gt_experiments/ (where the censuses and adjudicate_cell.py live).
#
# ETA IS NOT A CELL DIMENSION. The three eta records are the same run scored
# at three sensitivities, so they must not be pooled into one micro-average --
# that would count every site three times. Each eta gets its own table.
set -u

F="../fpchecker_experiments"       # FPChecker results tree
# FPC_UNIVERSE=CENSUS (default) assumes FPChecker instrumented every site the
# census executed, so silence counts as a correct rejection. Set it to a real
# instrumented_sites.csv once the harness emits one.
CONV="conv"                        # converted CSVs land here
OUT="adj_fpc"                      # per-cell reports and JSON
mkdir -p "$CONV" "$OUT"

# benchmark : results-subdir : census-subdir
CELLS="
LULESH:lulesh:lulesh
AMG:amg:amg
QuickSilver:quicksilver:quicksilver
BT:nas/bt:nas/results/bt/O0
CG:nas/cg:nas/results/cg/O0
EP:nas/ep:nas/results/ep/O0
LU:nas/lu:nas/results/lu/O0
MG:nas/mg:nas/results/mg/O0
SP:nas/sp:nas/results/sp/O0
"

for row in $CELLS; do
  [ -z "$row" ] && continue
  IFS=: read -r B SUB CEN <<< "$row"
  low=$(echo "$B" | tr 'A-Z' 'a-z')
  for p in fp32 fp64; do
    [ "$p" = fp32 ] && pair=fp32_vs_fp64 || pair=fp64_vs_ld
    # census lives in two different layouts: nas/results/<b>/O0/<pair>/
    # for NAS, <bench>/results/O0/<pair>/ for the other three
    case "$CEN" in
      nas/*) census="$CEN/$pair/sites.txt" ;;
      *)     census="$CEN/results/O0/$pair/sites.txt" ;;
    esac
    case "$SUB" in
      nas/*) sj="$F/nas/results/${SUB#nas/}/O0/$p/summary.json" ;;
      *)     sj="$F/$SUB/results/O0/$p/summary.json" ;;
    esac
    [ -f "$sj" ]     || { printf 'SKIP %-12s %-5s no summary.json\n' "$B" "$p"; continue; }
    [ -f "$census" ] || { printf 'SKIP %-12s %-5s no census\n'      "$B" "$p"; continue; }

    ./fpchecker_to_csv.py "$sj" --bench "$low" --precision "$p" \
        --outdir "$CONV" > "$CONV/${low}_${p}.convert.log" 2>&1 \
      || { printf 'FAIL %-12s %-5s convert\n' "$B" "$p"; continue; }

    cov=$(grep -m1 "lock-step compared" "$(dirname "$census")/report.txt" \
          2>/dev/null | grep -o "([0-9.]*%" | tr -d '(%')

    for det in "$CONV/${low}_${p}"_eta*_fpchecker_summary.csv; do
      [ -f "$det" ] || continue
      eta=$(basename "$det" | sed "s/.*_eta\(.*\)_fpchecker.*/\1/")
      uni="${FPC_UNIVERSE:-CENSUS}"
      ./adjudicate_cell.py --census "$census" --universe "$uni" \
          --detected "$det" --tool "FPChecker-$eta" --benchmark "$B" \
          --precision "$p" --coverage "${cov:-100}" \
          --report "$OUT/${B}_${p}_eta${eta}.txt" \
          --json   "$OUT/${B}_${p}_eta${eta}.json" > /dev/null \
        && printf 'ok   %-12s %-5s eta %-8s\n' "$B" "$p" "$eta" \
        || printf 'FAIL %-12s %-5s eta %-8s\n' "$B" "$p" "$eta"
    done
  done
done

echo
# One table per eta. Grouping by tool name (FPChecker-<eta>) keeps the
# micro-average within a single sensitivity setting.
for eta in $(ls "$OUT"/*.json 2>/dev/null | sed 's/.*_eta\(.*\)\.json/\1/' | sort -u); do
  echo "########## eta = $eta ##########"
  ./build_tables.py "$OUT"/*_eta${eta}.json \
      --text "$OUT/table_eta${eta}.txt" \
      --latex "$OUT/table_eta${eta}.tex" \
      --label "tab:fpchecker-eta${eta}"
  echo
done