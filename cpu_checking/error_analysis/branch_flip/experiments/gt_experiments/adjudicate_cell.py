#!/usr/bin/env python3
"""adjudicate_cell.py -- score ONE tool/benchmark/precision cell against a
brtrace census, with the adjudication window handled correctly.

    ./adjudicate_cell.py --census  lulesh_fp32vs64.sites.txt \
                         --universe lulesh/results/O0/fp32/instrumented_sites.csv \
                         --detected lulesh/results/O0/fp32/lulesh_fp32_eftsan_summary.csv \
                         --tool EFTSan --benchmark LULESH --precision fp32 \
                         --coverage 21.62 \
                         --json out.json

Writes a human-readable report to stdout (and --report), plus a machine
readable --json record that stack_table.py aggregates into the paper table.

--------------------------------------------------------------------------
WHY THIS EXISTS RATHER THAN A FEW LINES OF SET ARITHMETIC
--------------------------------------------------------------------------

The naive scoring -- intersect the tool's reported sites with the census's
flipped sites -- produces numbers that cannot be defended, for three
reasons this script handles explicitly.

1. MIXED DENOMINATORS.  The census adjudicates a PREFIX of the run: once
   the two trajectories visit different sites, lock-step ends and there is
   no oracle verdict for anything after.  brtrace's TN count therefore
   covers only that prefix.  The tool, meanwhile, reports over the WHOLE
   run.  Counting tool detections against a prefix-derived TN universe
   mixes two different populations.  Fixing this is not a matter of
   scaling: the fix is to define the scored universe as the intersection
   of what the census adjudicated and what the tool instrumented, and to
   report everything outside that intersection in its own column.

2. NOT EVERY MISS IS A FALSE NEGATIVE.  A census site the tool never
   instrumented cannot be detected, so scoring it FN measures coverage,
   not accuracy.  Likewise a tool detection at a site the census never
   instrumented is out of scope, not a false positive.  These are counted
   and reported, but kept out of the 2x2.

3. DEAD IS NOT TN.  brtrace classifies an instrumented site that never
   executed as DEAD.  Folding those into TN inflates the negative universe
   with branches no run ever reached -- on AMG that is ~86% of the static
   universe, which would make every accuracy figure meaningless.

--------------------------------------------------------------------------
THE SCORED UNIVERSE
--------------------------------------------------------------------------

    S  =  { sites brtrace EXECUTED in the adjudication window }
             INTERSECT
          { sites the tool INSTRUMENTED }

From the census: the TP and TN rows of --sites-txt.  DEAD rows are excluded.
From the tool:   every row of instrumented_sites.csv.

Within S:

                        census TP        census TN
    tool reported          TP               FP
    tool silent            FN               TN

Reported alongside, never folded in:

    coverage gap    census TP/TN sites the tool did not instrument
    out of scope    tool detections at sites absent from the census universe
    dead hits       tool detections at census-DEAD sites

--------------------------------------------------------------------------
BOUNDS WHEN COVERAGE < 100%
--------------------------------------------------------------------------

One asymmetry survives the intersection and must be stated rather than
hidden.  The tool reports over the whole run; the census verdict covers
only the prefix.  So:

    FP is an UPPER BOUND.  A tool detection at a census-TN site is either
    a real false positive inside the window, or a genuine flip after the
    divergence point where the census is simply silent.  Nothing in the
    tool's output distinguishes them, because its flip lines carry no
    event index.

    FN is a LOWER BOUND.  The census's TP set only contains sites that
    flipped before divergence; a site that would have flipped later is not
    in it, so the tool cannot be charged for missing it.

At 100% coverage both are exact.  Pass --coverage so the report says which
regime it is in; the JSON carries the flag through to the table.

--------------------------------------------------------------------------
METRICS
--------------------------------------------------------------------------

    precision = TP / (TP + FP)
    recall    = TP / (TP + FN)
    F1        = 2PR / (P + R)
    accuracy  = (TP + TN) / |S|

Accuracy is emitted but should not be a headline number: TN outnumbers TP
by orders of magnitude, so a tool that reports nothing at all scores higher
accuracy than one that finds every real flip.  It is in the output so that
a reviewer asking for it gets a consistent answer, not because it means
much.

When the census found no flips at all (GT+ = 0), precision and recall are
undefined -- there is no positive class.  That cell is scored by
false-alarm rate instead:

    far_sites = FP / |S|            (fraction of scored sites that misfired)

Both are emitted; the table builder picks by GT.

--------------------------------------------------------------------------
SITE KEYS
--------------------------------------------------------------------------

Everything is keyed on "basename:line".  The census writes
"file:line  [function]" and the tool writes "file:line"; both are
normalised by stripping any bracketed suffix and reducing the path to its
basename.  Basename rather than full path because the two sides reach the
same file by different routes -- the census records the path as compiled,
the tool resolves it from debug info -- and only the basename is reliably
common.  This is the same convention brtrace uses for its FNV-1a module
ids, so it cannot introduce a mismatch the rest of the pipeline does not
already have.

Sites whose key the tool marked ambiguous (column 2 of
instrumented_sites.csv) are counted and named in the report.  Since the
file-id patch they should be zero; a non-zero count means a genuine hash
collision and is worth investigating before trusting the cell.
"""

import argparse
import csv
import json
import os
import re
import sys
from collections import OrderedDict

BRACKET_RE = re.compile(r"\s*\[.*\]\s*$")


def norm(loc):
    """'/path/to/gmres.c:573  [hypre_GMRESSolve]' -> 'gmres.c:573'."""
    loc = BRACKET_RE.sub("", loc.strip())
    if ":" in loc:
        path, _, line = loc.rpartition(":")
        return os.path.basename(path) + ":" + line
    return os.path.basename(loc)


def read_census(path):
    """Parse brtrace's --sites-txt.

    Format:  class  flips  executions  first_event  location
    Returns (gt_pos, gt_neg, dead, flips_by_site, execs_by_site).

    gt_pos = TP rows  (flipped at least once in the window)
    gt_neg = TN rows  (executed in the window, never flipped)
    dead   = DEAD rows (instrumented, never executed -- NOT part of S)
    """
    gt_pos, gt_neg, dead = set(), set(), set()
    flips, execs = {}, {}
    fast_mode = False
    with open(path) as fh:
        for line in fh:
            if line.startswith("#"):
                if "--fast was used" in line:
                    fast_mode = True
                continue
            if not line.strip():
                continue
            parts = line.split(None, 4)
            if len(parts) < 5:
                continue
            cls, nf, ne, _first, where = parts
            key = norm(where)
            if cls == "TP":
                gt_pos.add(key)
            elif cls == "TN":
                gt_neg.add(key)
            elif cls == "DEAD":
                dead.add(key)
            else:
                continue
            flips[key] = flips.get(key, 0) + int(nf)
            execs[key] = execs.get(key, 0) + int(ne)
    if fast_mode:
        sys.stderr.write(
            "[FATAL] the census was produced with --fast, so executions are 0 "
            "and TN cannot be distinguished from DEAD. Re-run brtrace_diff "
            "without --fast before scoring this cell.\n")
        sys.exit(2)
    return gt_pos, gt_neg, dead, flips, execs


def read_universe(path):
    """Parse the tool's instrumented_sites.csv (location,ambiguous)."""
    inst, ambiguous = set(), set()
    with open(path) as fh:
        r = csv.reader(fh)
        header = next(r, None)
        for row in r:
            if not row or not row[0].strip():
                continue
            key = norm(row[0])
            inst.add(key)
            if len(row) > 1 and row[1].strip() == "1":
                ambiguous.add(key)
    return inst, ambiguous


def read_detected(path):
    """Parse the tool's per-cell summary CSV (location,flips).

    Rows whose location the harness could not resolve to a file are kept
    separately: 'line:NNN' (no debug info) and 'AMBIGUOUS:NNN' (the key
    collided).  Neither can be scored, and silently dropping them would
    understate the tool's output.
    """
    det, unresolved = OrderedDict(), OrderedDict()
    with open(path) as fh:
        r = csv.reader(fh)
        header = next(r, None)
        for row in r:
            if not row or not row[0].strip():
                continue
            raw = row[0].strip()
            try:
                n = int(row[1]) if len(row) > 1 and row[1].strip() else 0
            except ValueError:
                n = 0
            if raw.startswith("AMBIGUOUS:") or raw.startswith("line:"):
                unresolved[raw] = unresolved.get(raw, 0) + n
                continue
            key = norm(raw)
            det[key] = det.get(key, 0) + n
    return det, unresolved


def safe_div(a, b):
    return (a / b) if b else None


def fmt(x, nd=4):
    return "n/a" if x is None else ("%.*f" % (nd, x))


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--census", required=True,
                    help="brtrace --sites-txt output for this benchmark and "
                         "precision pair")
    ap.add_argument("--universe", required=True,
                    help="the tool's instrumented_sites.csv. Pass the literal "
                         "string CENSUS to assume the tool instrumented "
                         "everything the census executed -- see below.")
    ap.add_argument("--detected", required=True,
                    help="the tool's per-cell summary CSV (location,flips)")
    ap.add_argument("--tool", default="tool")
    ap.add_argument("--benchmark", default="?")
    ap.add_argument("--precision", default="?")
    ap.add_argument("--coverage", type=float, default=None,
                    help="adjudication coverage %% from the brtrace report. "
                         "Below 100, FP is an upper bound and FN a lower "
                         "bound; the report and the JSON say so.")
    ap.add_argument("--report", help="also write the report here")
    ap.add_argument("--json", help="write the machine-readable record here")
    ap.add_argument("--list-limit", type=int, default=25,
                    help="max sites to name in each list (0 = all)")
    args = ap.parse_args()

    gt_pos, gt_neg, dead, gt_flips, gt_execs = read_census(args.census)

    # --universe CENSUS: assume the tool instrumented every site the census
    # executed, so a site the tool never reported counts as a correct
    # rejection rather than as a coverage gap.
    #
    # This ASSUMES rather than measures the negative universe. It is
    # reasonable when the two instrument the same branch class -- both
    # brtrace and FPChecker wrap fcmp feeding a conditional branch -- and it
    # is the only option when the tool does not emit its static site list.
    # But it credits the tool with a TN at every site it never watched, so
    # it can only inflate accuracy and deflate the false-alarm rate, never
    # the reverse. Precision, recall and F1 do not depend on TN and are
    # unaffected either way.
    #
    # Verify before relying on it: count calls to the tool's comparison hook
    # in the instrumented IR and check the total against the census's
    # instrumented site count for the same benchmark. If they match, the
    # assumption is evidence-backed rather than convenient.
    universe_assumed = (args.universe.upper() == "CENSUS")
    if universe_assumed:
        inst, ambiguous = set(gt_pos) | set(gt_neg) | set(dead), set()
    else:
        inst, ambiguous = read_universe(args.universe)
    det, unresolved = read_detected(args.detected)

    census_exec = gt_pos | gt_neg          # what the oracle adjudicated
    census_all = census_exec | dead        # census static universe

    # ---- the scored universe -------------------------------------------
    S = census_exec & inst

    S_pos = gt_pos & S
    S_neg = gt_neg & S

    TP = sorted(S_pos & set(det))
    FN = sorted(S_pos - set(det))
    FP = sorted(S_neg & set(det))
    TN = sorted(S_neg - set(det))

    # ---- everything deliberately kept out of the 2x2 --------------------
    coverage_gap = sorted(census_exec - inst)      # census saw it, tool blind
    dead_hits = sorted(dead & set(det))            # tool fired on DEAD
    out_of_scope = sorted(set(det) - census_all)   # tool fired outside census
    tool_only = sorted(inst - census_all)          # instrumented, not in census

    ntp, nfn, nfp, ntn = len(TP), len(FN), len(FP), len(TN)
    prec = safe_div(ntp, ntp + nfp)
    rec = safe_div(ntp, ntp + nfn)
    f1 = (2 * prec * rec / (prec + rec)) if (prec and rec) else (
        0.0 if (prec is not None and rec is not None) else None)
    acc = safe_div(ntp + ntn, len(S))
    far = safe_div(nfp, len(S))

    partial = args.coverage is not None and args.coverage < 99.995

    # ---- event-level -----------------------------------------------------
    # One branch EVALUATION is one item, rather than one static site.
    #
    # Neither side emits an event index: the census reports per-site flip and
    # execution counts, the tool reports a per-site detection count.  So the
    # two cannot be aligned event by event and the matrix has to be built
    # per site from the three counts:
    #
    #     F(s) = census flips        E(s) = census executions
    #     D(s) = tool detections
    #
    #     TP(s) = min(D, F)          flags that COULD correspond to real flips
    #     FP(s) = max(0, D - F)      flags beyond what could possibly be real
    #     FN(s) = max(0, F - D)      real flips not covered by a flag
    #     TN(s) = E - F - FP(s)      correct silences
    #
    # This is the OPTIMISTIC alignment: it credits the tool whenever a flag
    # could match a real flip.  Event TP is therefore an upper bound and
    # event FP a lower bound -- separately from, and in addition to, the
    # window bounds that apply when coverage is below 100%.  A pessimistic
    # alignment (assume no flag matches) would give TP=0 everywhere and is
    # useless; the truth is between them and cannot be recovered without
    # event indices on both sides.
    #
    # D(s) > E(s) is possible and common: the tool reports over the whole
    # run while the census adjudicated only a prefix.  Those excess events
    # are counted separately as over-window detections rather than being
    # forced into FP, because they are outside the adjudicated population
    # entirely.
    eTP = eFP = eFN = eTN = 0
    over_window = 0
    for s in S:
        F = gt_flips.get(s, 0)
        E = gt_execs.get(s, 0)
        D = det.get(s, 0)
        if D > E:
            over_window += D - E
            D = E
        tp_s = min(D, F)
        fp_s = max(0, D - F)
        fn_s = max(0, F - D)
        tn_s = max(0, E - F - fp_s)
        eTP += tp_s
        eFP += fp_s
        eFN += fn_s
        eTN += tn_s

    e_prec = safe_div(eTP, eTP + eFP)
    e_rec = safe_div(eTP, eTP + eFN)
    e_f1 = (2 * e_prec * e_rec / (e_prec + e_rec)) if (e_prec and e_rec) else (
        0.0 if (e_prec is not None and e_rec is not None) else None)
    e_total = eTP + eFP + eFN + eTN
    e_acc = safe_div(eTP + eTN, e_total)
    e_far = safe_div(eFP, eFP + eTN)

    # ---------------------------------------------------------------- report
    L = []
    def out(s=""):
        L.append(s)

    def num(x):
        return "{:,}".format(x)

    def block(title, items, counts=None, note=None):
        out("  %s: %d" % (title, len(items)))
        if note:
            out("      %s" % note)
        if items:
            lim = len(items) if args.list_limit == 0 else args.list_limit
            for k in items[:lim]:
                extra = ""
                if counts:
                    bits = []
                    if k in gt_flips and gt_flips[k]:
                        bits.append("oracle %d" % gt_flips[k])
                    if k in det:
                        bits.append("tool %d" % det[k])
                    if bits:
                        extra = "   (%s)" % ", ".join(bits)
                out("      %s%s" % (k, extra))
            if len(items) > lim:
                out("      ... %d more" % (len(items) - lim))
        out()

    bar = "=" * 74
    out(bar)
    out("%s  --  %s %s" % (args.tool, args.benchmark, args.precision))
    out(bar)
    out()
    out("  census   %s" % args.census)
    out("  universe %s%s" % (args.universe,
        "   [ASSUMED: census-executed set; TN not measured]"
        if universe_assumed else ""))
    out("  detected %s" % args.detected)
    out()

    out("-" * 74)
    out("POPULATIONS")
    out("-" * 74)
    out("  census: executed %d  (TP %d, TN %d),  dead %d,  static %d"
        % (len(census_exec), len(gt_pos), len(gt_neg), len(dead),
           len(census_all)))
    out("  tool:   instrumented %d%s,  reported %d site(s)"
        % (len(inst),
           " (ASSUMED = census static set)" if universe_assumed else "",
           len(det)))
    out("  SCORED UNIVERSE |S| = %d   (census-executed INTERSECT "
        "tool-instrumented)" % len(S))
    out("      of which GT+ = %d, GT- = %d" % (len(S_pos), len(S_neg)))
    if args.coverage is not None:
        out("  adjudication coverage: %.2f%%" % args.coverage)
    out()

    out("-" * 74)
    out("CONFUSION MATRIX  (site-based, within S)")
    out("-" * 74)
    out()
    out("                     census +        census -")
    out("      tool +   %12d   %13d" % (ntp, nfp))
    out("      tool -   %12d   %13d" % (nfn, ntn))
    out()
    if partial:
        out("  FP <= %d  and  FN >= %d   -- coverage is %.2f%%, so the census"
            % (nfp, nfn, args.coverage))
        out("  verdict covers only the pre-divergence prefix while the tool")
        out("  reported over the whole run. A tool detection at a GT- site may")
        out("  be a genuine post-divergence flip the oracle never saw.")
    else:
        out("  Coverage is 100%, so these counts are exact.")
    out()

    out("-" * 74)
    out("METRICS")
    out("-" * 74)
    if len(S_pos) == 0:
        out("  GT+ is empty: precision, recall and F1 are undefined for this")
        out("  cell (no positive class). Score it by false-alarm rate.")
        out()
        out("    false-alarm rate   %s   (%d FP / %d scored sites)"
            % (fmt(far, 6), nfp, len(S)))
        out("    accuracy           %s" % fmt(acc))
    else:
        out("    precision          %s%s" % (fmt(prec),
                                             "   (lower bound)" if partial else ""))
        out("    recall             %s%s" % (fmt(rec),
                                             "   (upper bound)" if partial else ""))
        out("    F1                 %s" % fmt(f1))
        out("    accuracy           %s   [TN-dominated; do not headline]"
            % fmt(acc))
        out("    false-alarm rate   %s" % fmt(far, 6))
    out()

    out("-" * 74)
    out("EVENT-BASED  (one branch evaluation = one item, within S)")
    out("-" * 74)
    out()
    out("                     census +        census -")
    out("      tool +   %12s   %13s" % (num(eTP), num(eFP)))
    out("      tool -   %12s   %13s" % (num(eFN), num(eTN)))
    out()
    out("    adjudicated events in S: %s" % num(e_total))
    if over_window:
        out("    over-window detections:  %s   [tool events beyond what the"
            % num(over_window))
        out("                                  census adjudicated at that "
            "site]")
    out()
    if eTP + eFN == 0:
        out("  GT+ is empty at event level: no positive class.")
        out("    false-alarm rate   %s   (%s FP / %s negative events)"
            % (fmt(e_far, 8), num(eFP), num(eFP + eTN)))
        out("    accuracy           %s" % fmt(e_acc, 6))
    else:
        out("    precision          %s" % fmt(e_prec))
        out("    recall             %s" % fmt(e_rec))
        out("    F1                 %s" % fmt(e_f1))
        out("    accuracy           %s   [TN-dominated]" % fmt(e_acc, 6))
        out("    false-alarm rate   %s" % fmt(e_far, 8))
    out()
    out("  Events cannot be aligned individually -- neither the census nor")
    out("  the tool emits an event index -- so this matrix is built per site")
    out("  from flip, execution and detection COUNTS, crediting the tool")
    out("  whenever a flag could match a real flip. Event TP is therefore an")
    out("  upper bound and event FP a lower bound.")
    out()

    out("-" * 74)
    out("SITES")
    out("-" * 74)
    block("TP  detected, oracle confirms", TP, counts=True)
    block("FN  oracle flipped, tool silent", FN, counts=True)
    block("FP  tool fired, oracle says no flip", FP, counts=True,
          note=("upper bound -- may include post-divergence flips"
                if partial else None))
    out("  TN  executed, both silent: %d" % ntn)
    out()

    out("-" * 74)
    out("EXCLUDED FROM THE 2x2  (reported, never folded in)")
    out("-" * 74)
    block("coverage gap  census site, tool did not instrument", coverage_gap,
          note="cannot be detected, so NOT scored FN")
    block("out of scope  tool fired, site absent from census universe",
          out_of_scope, counts=True,
          note="e.g. fcmp->select sites brtrace does not instrument")
    block("dead hits     tool fired at a census-DEAD site", dead_hits,
          counts=True,
          note="census says never executed; investigate before trusting")
    out("  tool-only instrumented (no census entry): %d" % len(tool_only))
    out()

    if unresolved:
        out("-" * 74)
        out("UNRESOLVED TOOL OUTPUT  (not scorable)")
        out("-" * 74)
        for k, v in unresolved.items():
            out("      %-40s %d event(s)" % (k, v))
        out()
        out("  Since the file-id patch these should be absent. A non-zero")
        out("  count means the harness could not key a flip to a file.")
        out()

    amb_in_S = sorted(ambiguous & S)
    if amb_in_S:
        out("  [warn] %d scored site(s) are flagged ambiguous in the tool's"
            % len(amb_in_S))
        out("         universe: %s" % ", ".join(amb_in_S[:8]))
        out()

    text = "\n".join(L)
    print(text)
    if args.report:
        with open(args.report, "w") as fh:
            fh.write(text + "\n")
        print("wrote %s" % args.report)

    if args.json:
        rec_out = {
            "tool": args.tool,
            "benchmark": args.benchmark,
            "precision": args.precision,
            "coverage_pct": args.coverage,
            "partial_window": partial,
            "scored_universe": len(S),
            "gt_pos": len(S_pos),
            "gt_neg": len(S_neg),
            "TP": ntp, "FP": nfp, "FN": nfn, "TN": ntn,
            "precision_metric": prec,
            "recall": rec,
            "f1": f1,
            "accuracy": acc,
            "false_alarm_rate": far,
            "event_TP": eTP, "event_FP": eFP,
            "event_FN": eFN, "event_TN": eTN,
            "event_total": e_total,
            "event_over_window": over_window,
            "event_precision": e_prec,
            "event_recall": e_rec,
            "event_f1": e_f1,
            "event_accuracy": e_acc,
            "event_false_alarm_rate": e_far,
            "fp_is_upper_bound": partial,
            "fn_is_lower_bound": partial,
            "coverage_gap": len(coverage_gap),
            "out_of_scope": len(out_of_scope),
            "dead_hits": len(dead_hits),
            "unresolved_events": sum(unresolved.values()),
            "universe_assumed": universe_assumed,
            "census_executed": len(census_exec),
            "census_dead": len(dead),
            "tool_instrumented": len(inst),
            "tool_reported": len(det),
            "sites": {
                "TP": TP, "FN": FN, "FP": FP,
                "coverage_gap": coverage_gap,
                "out_of_scope": out_of_scope,
                "dead_hits": dead_hits,
            },
        }
        with open(args.json, "w") as fh:
            json.dump(rec_out, fh, indent=2)
        print("wrote %s" % args.json)

    return 0


if __name__ == "__main__":
    sys.exit(main())