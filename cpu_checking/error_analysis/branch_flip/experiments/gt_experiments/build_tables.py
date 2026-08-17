#!/usr/bin/env python3
"""build_tables.py -- aggregate adjudicate_cell.py JSON records into the
paper's tables.

    ./build_tables.py adj/*.json --text tab.txt --latex tab.tex

Reads any number of per-cell JSON records and emits:

  1. a per-cell table (one row per tool x benchmark x precision)
  2. suite-level micro- and macro-averaged metrics per tool
  3. a LaTeX version of (1) with the exclusion columns kept separate

--------------------------------------------------------------------------
WHY MICRO-AVERAGE
--------------------------------------------------------------------------

Macro-averaging (mean of the per-cell F1s) weights a benchmark with one
ground-truth site the same as one with three hundred, and it has to decide
what to do with cells where F1 is undefined.  Micro-averaging pools the
confusion matrices first and computes one F1 from the totals, so every
scored site carries equal weight and GT=0 cells contribute their FPs
without needing an undefined F1 to be invented for them.

Both are printed.  Micro is the defensible suite-level number; macro is
included because reviewers ask for it, and the gap between them is itself
informative -- a large gap means the tool's performance is concentrated in
a few cells.

--------------------------------------------------------------------------
GT = 0 CELLS
--------------------------------------------------------------------------

Cells with no ground-truth positives have no precision or recall.  They are
shown with their false-alarm rate instead and marked so the table does not
imply a missing measurement.  They still contribute FP and TN to the micro
average, which is the point: a tool that stays quiet on a clean benchmark
should be rewarded for it, and one that floods it should pay.

--------------------------------------------------------------------------
BOUNDS
--------------------------------------------------------------------------

Cells adjudicated at less than 100% coverage carry a marker.  Their FP is
an upper bound and FN a lower bound, so precision is a lower bound and
recall an upper bound.  The marker propagates into the LaTeX output as a
footnote symbol; a suite-level number that pools bounded and exact cells is
itself bounded, and the summary says so when that happens.
"""

import argparse
import json
import sys
from collections import OrderedDict, defaultdict

BENCH_ORDER = ["LULESH", "AMG", "QuickSilver", "BT", "CG", "EP", "IS",
               "LU", "MG", "SP"]


def order_key(r):
    b = r.get("benchmark", "?")
    try:
        bi = BENCH_ORDER.index(b)
    except ValueError:
        bi = len(BENCH_ORDER)
    return (r.get("tool", ""), bi, b, r.get("precision", ""))


def f(x, nd=3):
    return "--" if x is None else ("%.*f" % (nd, x))


def micro(records):
    """Pool the confusion matrices, then compute one set of metrics."""
    TP = sum(r["TP"] for r in records)
    FP = sum(r["FP"] for r in records)
    FN = sum(r["FN"] for r in records)
    TN = sum(r["TN"] for r in records)
    p = TP / (TP + FP) if (TP + FP) else None
    rc = TP / (TP + FN) if (TP + FN) else None
    f1 = (2 * p * rc / (p + rc)) if (p and rc) else (
        0.0 if (p is not None and rc is not None) else None)
    acc = (TP + TN) / (TP + FP + FN + TN) if (TP + FP + FN + TN) else None
    far = FP / (FP + TN) if (FP + TN) else None
    return dict(TP=TP, FP=FP, FN=FN, TN=TN, precision=p, recall=rc, f1=f1,
                accuracy=acc, false_alarm_rate=far)


def micro_events(records):
    """Same pooling, over event counts instead of site counts."""
    TP = sum(r.get("event_TP", 0) for r in records)
    FP = sum(r.get("event_FP", 0) for r in records)
    FN = sum(r.get("event_FN", 0) for r in records)
    TN = sum(r.get("event_TN", 0) for r in records)
    p = TP / (TP + FP) if (TP + FP) else None
    rc = TP / (TP + FN) if (TP + FN) else None
    f1 = (2 * p * rc / (p + rc)) if (p and rc) else (
        0.0 if (p is not None and rc is not None) else None)
    acc = (TP + TN) / (TP + FP + FN + TN) if (TP + FP + FN + TN) else None
    far = FP / (FP + TN) if (FP + TN) else None
    return dict(TP=TP, FP=FP, FN=FN, TN=TN, precision=p, recall=rc, f1=f1,
                accuracy=acc, false_alarm_rate=far)


def macro(records):
    """Mean over cells that HAVE an F1. Cells with GT=0 are skipped and
    counted, because averaging over an undefined value is not a choice the
    script should make silently."""
    vals = [r["f1"] for r in records if r.get("f1") is not None]
    skipped = len(records) - len(vals)
    return (sum(vals) / len(vals) if vals else None), skipped


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("json", nargs="+")
    ap.add_argument("--text", help="write the text table here")
    ap.add_argument("--latex", help="write the LaTeX table here")
    ap.add_argument("--label", default="tab:branch-flips")
    args = ap.parse_args()

    records = []
    for p in args.json:
        try:
            with open(p) as fh:
                records.append(json.load(fh))
        except Exception as e:
            sys.stderr.write("[warn] skipping %s: %s\n" % (p, e))
    if not records:
        sys.stderr.write("no records\n")
        return 1
    records.sort(key=order_key)

    L = []
    def out(s=""):
        L.append(s)

    # ------------------------------------------------------------ per cell
    hdr = ("%-8s %-12s %-5s %6s %5s %5s %5s %8s %6s %6s %6s %6s %6s  %s"
           % ("tool", "benchmark", "prec", "|S|", "TP", "FP", "FN", "TN",
              "P", "R", "F1", "Acc", "cov%", "notes"))
    out("=" * len(hdr))
    out("PER-CELL")
    out("=" * len(hdr))
    out(hdr)
    out("-" * len(hdr))

    any_bounded = False
    for r in records:
        notes = []
        if r.get("partial_window"):
            notes.append("bounded")
            any_bounded = True
        if r.get("gt_pos", 0) == 0:
            notes.append("GT=0 far=%s" % f(r.get("false_alarm_rate"), 5))
        if r.get("coverage_gap"):
            notes.append("gap %d" % r["coverage_gap"])
        if r.get("out_of_scope"):
            notes.append("oos %d" % r["out_of_scope"])
        if r.get("dead_hits"):
            notes.append("dead %d" % r["dead_hits"])
        if r.get("unresolved_events"):
            notes.append("UNRESOLVED %d" % r["unresolved_events"])
        cov = r.get("coverage_pct")
        out("%-8s %-12s %-5s %6d %5d %5d %5d %8d %6s %6s %6s %6s %6s  %s"
            % (r.get("tool", "?"), r.get("benchmark", "?"),
               r.get("precision", "?"), r.get("scored_universe", 0),
               r["TP"], r["FP"], r["FN"], r["TN"],
               f(r.get("precision_metric"), 3), f(r.get("recall"), 3),
               f(r.get("f1"), 3), f(r.get("accuracy"), 3),
               ("%.1f" % cov) if cov is not None else "--",
               ", ".join(notes)))
    out()

    # ------------------------------------------------------ per cell events
    ehdr = ("%-8s %-12s %-5s %10s %8s %8s %6s %10s %6s %6s %6s %8s  %s"
            % ("tool", "benchmark", "prec", "events", "TP", "FP", "FN", "TN",
               "P", "R", "F1", "Acc", "notes"))
    out("=" * len(ehdr))
    out("PER-CELL, EVENT-BASED")
    out("=" * len(ehdr))
    out("  One branch evaluation is one item. Built per site from flip,")
    out("  execution and detection counts, since neither side emits an event")
    out("  index; the tool is credited whenever a flag could match a real")
    out("  flip, so event TP is an upper bound and event FP a lower bound.")
    out("=" * len(ehdr))
    out(ehdr)
    out("-" * len(ehdr))
    for r in records:
        notes = []
        if r.get("partial_window"):
            notes.append("bounded")
        if r.get("event_over_window"):
            notes.append("over-window %d" % r["event_over_window"])
        if (r.get("event_TP", 0) + r.get("event_FN", 0)) == 0:
            notes.append("GT=0 far=%s" % f(r.get("event_false_alarm_rate"), 7))
        out("%-8s %-12s %-5s %10d %8d %8d %6d %10d %6s %6s %6s %8s  %s"
            % (r.get("tool", "?"), r.get("benchmark", "?"),
               r.get("precision", "?"), r.get("event_total", 0),
               r.get("event_TP", 0), r.get("event_FP", 0),
               r.get("event_FN", 0), r.get("event_TN", 0),
               f(r.get("event_precision"), 3), f(r.get("event_recall"), 3),
               f(r.get("event_f1"), 3), f(r.get("event_accuracy"), 5),
               ", ".join(notes)))
    out()

    # --------------------------------------------------------- suite level
    by_tool = defaultdict(list)
    for r in records:
        by_tool[r.get("tool", "?")].append(r)

    out("=" * 74)
    out("SUITE LEVEL")
    out("=" * 74)
    for tool, rs in sorted(by_tool.items()):
        m = micro(rs)
        mac, skipped = macro(rs)
        bounded = sum(1 for r in rs if r.get("partial_window"))
        out()
        out("  %s   (%d cells)" % (tool, len(rs)))
        out("    pooled   TP %d   FP %d   FN %d   TN %d"
            % (m["TP"], m["FP"], m["FN"], m["TN"]))
        out("    micro    P %s   R %s   F1 %s"
            % (f(m["precision"]), f(m["recall"]), f(m["f1"])))
        out("    macro    F1 %s%s"
            % (f(mac), ("   (%d cell(s) with GT=0 excluded)" % skipped)
               if skipped else ""))
        out("    accuracy %s   false-alarm rate %s"
            % (f(m["accuracy"]), f(m["false_alarm_rate"], 6)))
        me = micro_events(rs)
        ow = sum(r.get("event_over_window", 0) for r in rs)
        out("    -- event-based --")
        out("    pooled   TP %d   FP %d   FN %d   TN %d"
            % (me["TP"], me["FP"], me["FN"], me["TN"]))
        out("    micro    P %s   R %s   F1 %s"
            % (f(me["precision"]), f(me["recall"]), f(me["f1"])))
        out("    accuracy %s   false-alarm rate %s"
            % (f(me["accuracy"], 6), f(me["false_alarm_rate"], 8)))
        if ow:
            out("    over-window detections: %d (tool events past what the "
                "census adjudicated)" % ow)
        if bounded:
            out("    NOTE: %d of %d cells were adjudicated below 100%% "
                "coverage, so the" % (bounded, len(rs)))
            out("          pooled FP is an upper bound and FN a lower bound. "
                "The micro")
            out("          precision is therefore a lower bound and recall an "
                "upper bound.")
        gap = sum(r.get("coverage_gap", 0) for r in rs)
        oos = sum(r.get("out_of_scope", 0) for r in rs)
        dead = sum(r.get("dead_hits", 0) for r in rs)
        unres = sum(r.get("unresolved_events", 0) for r in rs)
        out("    excluded: coverage gap %d sites, out of scope %d sites, "
            "dead hits %d sites" % (gap, oos, dead))
        if unres:
            out("    [warn] %d unresolved tool event(s) could not be scored"
                % unres)
    out()

    if any_bounded:
        out("-" * 74)
        out("Cells marked 'bounded' were adjudicated over a prefix of the run.")
        out("The census stops at the first control-flow divergence, so it has")
        out("no verdict after that point, while the tool reported over the")
        out("whole run. Their FP is an upper bound and FN a lower bound.")
        out()

    text = "\n".join(L)
    print(text)
    if args.text:
        with open(args.text, "w") as fh:
            fh.write(text + "\n")
        print("wrote %s" % args.text)

    # -------------------------------------------------------------- LaTeX
    if args.latex:
        T = []
        T.append("% generated by build_tables.py -- do not edit by hand")
        T.append("\\begin{table*}[t]")
        T.append("\\centering")
        T.append("\\small")
        T.append("\\caption{Branch-flip detection scored against the brtrace "
                 "census. The scored universe $S$ is the intersection of the "
                 "sites the census executed within its adjudication window "
                 "and the sites the tool instrumented; census-dead sites are "
                 "excluded. Coverage is the fraction of the longer trace "
                 "adjudicated in lock-step. Cells marked $\\dagger$ were "
                 "adjudicated over a prefix, so their FP is an upper bound "
                 "and FN a lower bound. Gap counts census sites the tool did "
                 "not instrument (not scored as misses); OOS counts tool "
                 "detections at sites outside the census universe.}")
        T.append("\\label{%s}" % args.label)
        T.append("\\begin{tabular}{lll rrrrr rrrr rr}")
        T.append("\\toprule")
        T.append("Tool & Benchmark & Prec. & $|S|$ & TP & FP & FN & TN & "
                 "P & R & F1 & Acc. & Cov.\\% & Gap/OOS \\\\")
        T.append("\\midrule")
        last_tool = None
        for r in records:
            if last_tool is not None and r.get("tool") != last_tool:
                T.append("\\midrule")
            last_tool = r.get("tool")
            dag = "$\\dagger$" if r.get("partial_window") else ""
            if r.get("gt_pos", 0) == 0:
                pcell = rcell = f1cell = "\\textemdash"
            else:
                pcell = f(r.get("precision_metric"))
                rcell = f(r.get("recall"))
                f1cell = f(r.get("f1"))
            cov = r.get("coverage_pct")
            T.append("%s & %s & %s & %d & %d & %d%s & %d%s & %d & %s & %s & "
                     "%s & %s & %s & %d/%d \\\\"
                     % (r.get("tool", "?"), r.get("benchmark", "?"),
                        r.get("precision", "?"), r.get("scored_universe", 0),
                        r["TP"], r["FP"], dag, r["FN"], dag, r["TN"],
                        pcell, rcell, f1cell, f(r.get("accuracy")),
                        ("%.1f" % cov) if cov is not None else "--",
                        r.get("coverage_gap", 0), r.get("out_of_scope", 0)))
        T.append("\\midrule")
        for tool, rs in sorted(by_tool.items()):
            m = micro(rs)
            T.append("\\multicolumn{4}{l}{\\textit{%s, micro-averaged}} & "
                     "%d & %d & %d & %d & %s & %s & %s & %s & & \\\\"
                     % (tool, m["TP"], m["FP"], m["FN"], m["TN"],
                        f(m["precision"]), f(m["recall"]), f(m["f1"]),
                        f(m["accuracy"])))
        T.append("\\bottomrule")
        T.append("\\end{tabular}")
        T.append("\\end{table*}")
        with open(args.latex, "w") as fh:
            fh.write("\n".join(T) + "\n")
        print("wrote %s" % args.latex)

    return 0


if __name__ == "__main__":
    sys.exit(main())