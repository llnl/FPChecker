#!/usr/bin/env python3
"""
site_table.py -- site-based confusion matrix across tools and cells.

    python3 site_table.py --out site_table.txt \\
      --row "LULESH fp32/fp64" lulesh/results/O0/fp32_vs_fp64/sites.txt \\
            ../eftsan_experiments/lulesh/results/O0/fp32/summary.txt \\
      --row "LULESH fp32/fp64" lulesh/results/O0/fp32_vs_fp64/sites.txt \\
            ../fpchecker_experiments/lulesh/results/O0/fp32/summary.txt

Repeat --row once per (cell, ground truth, tool) triple. Every eta / threshold
block inside a tool summary becomes its own line.

Companion to event_table.py, sharing the same inputs, the same --row interface
and the same site matching (both import adjudicate.py). The difference is the
unit:

    site_table.py   one STATIC BRANCH SITE is one item
    event_table.py  one BRANCH EVALUATION is one item

This one is the primary table. Both sides answer the same yes/no question
about the same object -- does this site flip at least once? -- so all four
cells are symmetric and P/R/F1 mean what they normally mean. The event table
weights every site by its execution count, which makes a single hot site swing
the numbers and compresses every F1 into a narrow band near zero.

Accuracy is reported alongside P/R/F1, but note what it measures here: the
positive class is a small minority of sites, so TN dominates the numerator and
a detector that stayed completely silent would outscore every real tool. Use
P/R/F1 for comparison and treat accuracy as descriptive.

DEAD sites (instrumented, never executed) are excluded from the scored
universe. The oracle has no verdict on code that never ran, and counting them
as true negatives would inflate every denominator with branches no execution
reached -- on AMG that is 489 of 566 sites.
"""

import argparse
import importlib.util
import os
import sys


def load_adjudicate():
    here = os.path.dirname(os.path.abspath(__file__))
    path = os.path.join(here, "adjudicate.py")
    if not os.path.exists(path):
        sys.exit("adjudicate.py must sit next to this script (looked in %s)"
                 % here)
    spec = importlib.util.spec_from_file_location("adjudicate", path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


A = load_adjudicate()


def site_matrix(gt_rows, flagged_keys):
    """TP/TN/FP/FN counted over EXECUTED sites."""
    tp = tn = fp = fn = 0
    for r in gt_rows:
        if r["class"] == "DEAD":
            continue
        flipped = r["flips"] > 0
        flagged = r["key"] in flagged_keys
        if flipped and flagged:
            tp += 1
        elif flipped:
            fn += 1
        elif flagged:
            fp += 1
        else:
            tn += 1
    return tp, tn, fp, fn


def prf(tp, tn, fp, fn):
    P = tp / (tp + fp) if tp + fp else float("nan")
    R = tp / (tp + fn) if tp + fn else float("nan")
    F = 2 * tp / (2 * tp + fp + fn) if (2 * tp + fp + fn) else float("nan")
    tot = tp + tn + fp + fn
    A = (tp + tn) / tot if tot else float("nan")
    return P, R, F, A


def fmt(x):
    return "—" if x != x else "%.4f" % x          # NaN check


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--row", nargs=3, action="append", required=True,
                    metavar=("LABEL", "SITES_TXT", "TOOL_SUMMARY"),
                    help="one (cell, ground truth, tool) triple; repeatable")
    ap.add_argument("--out", help="also write the table to this .txt file")
    ap.add_argument("--sort", action="store_true",
                    help="within each cell, order rows by F1 descending")
    args = ap.parse_args()

    lines, failed = [], []
    unres, amb, nflag = {}, {}, {}

    for label, gt_path, tool_path in args.row:
        gt = A.load_gt(gt_path)
        if not gt:
            sys.exit("no site rows parsed from %s" % gt_path)
        blocks = A.load_tool(tool_path)

        tool = "tool"
        for part in os.path.abspath(tool_path).split(os.sep):
            if "eftsan" in part.lower():
                tool = "EFTSan"
            elif "fpchecker" in part.lower():
                tool = "FPChecker"

        gt_keys = {r["key"] for r in gt}
        by_func = set(gt_keys)
        by_file = {}
        for r in gt:
            by_file.setdefault(r["fkey"], []).append(r["key"])

        if not blocks:
            failed.append((label, tool, tool_path))
            continue

        for block, sites in blocks.items():
            flagged, dropped, tool_amb = set(), [], []
            for s in sites:
                keys = [k for k in A.resolve(s, by_func, by_file)
                        if k in gt_keys]
                if keys:
                    flagged.update(keys)
                elif s.get("tool_ambiguous"):
                    tool_amb.append(s)
                else:
                    dropped.append(s)
            for d in dropped:
                key = (label, tool, "%s:%d%s"
                       % (d["file"], d["line"],
                          ("  [%s]" % d["func"]) if d["func"] else ""))
                unres.setdefault(key, set()).add(block)
                nflag[(label, tool)] = len(sites)
            for d in tool_amb:
                key = (label, tool,
                       "line %d (tool could not determine the file; "
                       "candidate %s)" % (d["line"], d["func"]))
                amb.setdefault(key, set()).add(block)

            tp, tn, fp, fn = site_matrix(gt, flagged)
            lines.append((label, tool, block, tp, tn, fp, fn) + prf(tp, tn, fp, fn))

    order = list(dict.fromkeys(r[0] for r in lines))
    grouped = []
    for cell in order:
        rows = [r for r in lines if r[0] == cell]
        if args.sort:
            rows.sort(key=lambda r: -(r[9] if r[9] == r[9] else -1))
        grouped.extend(rows)

    w_label = max([len(r[0]) for r in grouped] + [4])
    w_tool = max([len(r[1]) for r in grouped] + [4])
    w_cfg = max([len(r[2]) for r in grouped] + [6])
    hdr = ("%-*s  %-*s  %-*s  %5s %5s %5s %5s  %8s %8s %8s %9s"
           % (w_label, "cell", w_tool, "tool", w_cfg, "config",
              "TP", "TN", "FP", "FN", "prec", "recall", "F1", "accuracy"))
    out = ["SITE-BASED  (one static branch site = one item)", "",
           hdr, "-" * len(hdr)]
    last = None
    for r in grouped:
        if last is not None and r[0] != last:
            out.append("")
        last = r[0]
        out.append("%-*s  %-*s  %-*s  %5d %5d %5d %5d  %8s %8s %8s %9s"
                   % (w_label, r[0], w_tool, r[1], w_cfg, r[2],
                      r[3], r[4], r[5], r[6],
                      fmt(r[7]), fmt(r[8]), fmt(r[9]), fmt(r[10])))

    out += ["",
            "TP = site flips and the tool flagged it",
            "FN = site flips and the tool stayed silent",
            "FP = site never flips and the tool flagged it",
            "TN = site never flips and the tool stayed silent",
            "",
            "Scored over EXECUTED sites only. DEAD sites (instrumented but",
            "never reached) are excluded: the oracle has no verdict on code",
            "that never ran, and counting them TN would pad the denominator.",
            "",
            "On a prefix census -- any cell whose coverage is below 100% --",
            "FP is an UPPER bound and FN a LOWER bound, since a site that",
            "only flips after the divergence point cannot be credited.",
            "",
            "ACCURACY = (TP+TN)/(TP+TN+FP+FN). Read it with care: flipping",
            "sites are a small minority of the universe (5 of 70 on LULESH",
            "fp32), so TN dominates and the metric largely rewards silence.",
            "A detector that reported nothing at all would score 0.93 there,",
            "above every tool measured. Precision, recall and F1 contain no",
            "TN term and are not distorted this way."]

    if failed:
        out += ["", "NO RESULT (tool produced no flip data)",
                "  The tool did not run to completion, so there is nothing to",
                "  score. A capability finding, NOT recall 0."]
        for label, tool, path in failed:
            out.append("    %-18s %-10s %s" % (label, tool, path))
    if unres:
        out += ["", "OUTSIDE THE CENSUS UNIVERSE (contributed to no column)",
                "  Sites the tool flagged that brtrace never instrumented, so",
                "  the oracle has no verdict. Counting them FP would punish the",
                "  tool for looking where the oracle did not."]
        for (label, tool, where), blocks_ in sorted(unres.items()):
            out.append("    %-18s %-10s %s" % (label, tool, where))
            out.append("        in %s (1 of ~%d flagged site(s))"
                       % (", ".join(sorted(blocks_)), nflag.get((label, tool), 0)))
    if amb:
        out += ["", "UNRESOLVED BY THE TOOL ITSELF (contributed to no column)",
                "  The tool reported a line it could not attribute to a file."]
        for (label, tool, where), blocks_ in sorted(amb.items()):
            out.append("    %-18s %-10s %s   [%s]"
                       % (label, tool, where, ", ".join(sorted(blocks_))))

    text = "\n".join(out)
    print(text)
    if args.out:
        open(args.out, "w").write(text + "\n")
        print("\nwrote %s" % args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())