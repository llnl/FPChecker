#!/usr/bin/env python3
"""
score_table.py -- confusion matrices across tools and cells, at BOTH
site and event granularity, with the window-consistency checks that make the
numbers safe to put in a paper.

    python3 score_table.py \\
      --row "LULESH fp32/fp64" lulesh/results/O0/fp32_vs_fp64/sites.txt \\
            ../fpchecker_experiments/lulesh/results/O0/fp32/summary.txt

Repeat --row once per (cell, tool) pair. Every eta / threshold block inside a
tool summary becomes its own line. An optional 4th field gives the census
coverage percentage, which switches on the prefix-bound annotations.


WHAT CHANGED FROM THE PREVIOUS VERSION, AND WHY
===============================================

1. THE SITE TABLE IS NOW PRIMARY AND IS PRINTED FIRST.
   Both tables were always available, but the event table led and the site
   table lived in adjudicate.py, one cell at a time. That got the emphasis
   backwards. Event counts weight every site by how hot it is, so on LULESH
   fp64/ld a single std::max site is 15,000 of 407,037 events -- flagging it
   or not swings accuracy by tens of percent while the detection quality
   barely moves. The site table asks the question the tools actually answer:
   did you flag the branches that flip?

2. COUNT MODE IS OFF BY DEFAULT AND GATED BEHIND --unsound-count-mode.
   It compared the tool's warning COUNT against the oracle's flip COUNT per
   site. Those two numbers are measured over DIFFERENT WINDOWS whenever
   coverage is below 100%:

       oracle flips / executions : the adjudicated PREFIX only
                                   (LULESH fp32: 87,985 events)
       tool warnings             : the tool's ENTIRE run
                                   (LULESH fp32: 406,861 events)

   FPChecker emits no event indices, so there is no way to ask how many of
   its warnings fell inside the prefix. The old code silently clamped the
   excess (`if excess > room`) which made the totals look plausible while
   comparing incommensurate quantities. It is retained only for inspection,
   prints a banner saying so, and must not be used for a published table.

3. EVERY ROW IS CHECKED FOR WINDOW CONSISTENCY.
   The event cells must sum to the adjudicated event count, and the site
   cells to the number of executed sites. Both are derived from the same
   sites.txt the row was built from, so a mismatch means the ground truth and
   the scoring disagree about the universe. Violations abort rather than
   print, because this is exactly the failure that produces a wrong table
   that looks right.

4. FP AND FN ARE LABELLED AS BOUNDS WHEN COVERAGE < 100%.
   A site that only flips AFTER the divergence point is recorded as
   never-flipping in a prefix census, so a tool that flags it is charged an
   FP it may not deserve. With coverage supplied, FP is marked an upper bound
   and FN a lower bound, and precision/recall inherit those directions.


HOW EACH MATRIX IS DEFINED
==========================

SITE-BASED (primary). One executed site = one item.

    tool flagged, oracle saw >=1 flip   -> TP
    tool flagged, oracle saw no flip    -> FP
    not flagged,  oracle saw >=1 flip   -> FN
    not flagged,  oracle saw no flip    -> TN

DEAD sites (instrumented, never executed) are excluded: the oracle has no
verdict on them.

EVENT-BASED (secondary). One branch evaluation = one item. The tools emit
warnings at sites, not per-event predictions, so a site verdict is broadcast
to every adjudicated event at that site. For a site with `e` executions and
`f` oracle flips:

    tool flagged the site      ->  TP += f        FP += (e - f)
    tool did not flag the site ->  FN += f        TN += (e - f)

Note what the event FP column means: flagging a site that flips 795 times in
3,750 executions scores TP=795 AND FP=2,955. The tool is charged for every
execution of a site it warned about, even though it emitted one warning about
a site that genuinely does flip. That is a consequence of broadcasting a site
verdict to events; it is not the usual meaning of "false positive" for a
warning-based tool. Report the site table.

ACCURACY IS PRINTED BUT IS THE WEAKEST COLUMN in both tables. The positive
class is tiny -- 831 of 87,985 events, 5 of 71 sites on LULESH fp32 -- so a
detector is rewarded for staying silent. Use F1 where GT>0 and the false-alarm
rate where GT=0.

Requires adjudicate.py in the same directory: parsing and site matching are
shared, so both scripts resolve the std::max file-attribution mismatch the
same way.
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


# ------------------------------------------------------------------ matrices

def site_matrix(gt_rows, flagged_keys):
    """PRIMARY. One executed site = one item."""
    tp = tn = fp = fn = 0
    for r in gt_rows:
        if r["class"] == "DEAD":
            continue
        flagged = r["key"] in flagged_keys
        flips = r["flips"] > 0
        if flagged and flips:
            tp += 1
        elif flagged and not flips:
            fp += 1
        elif not flagged and flips:
            fn += 1
        else:
            tn += 1
    return tp, tn, fp, fn


def event_matrix(gt_rows, flagged_keys):
    """SECONDARY. The tool's per-site verdict applied to every event."""
    tp = tn = fp = fn = 0
    for r in gt_rows:
        if r["class"] == "DEAD":
            continue
        e, f = r["executions"], r["flips"]
        if r["key"] in flagged_keys:
            tp += f
            fp += e - f
        else:
            fn += f
            tn += e - f
    return tp, tn, fp, fn


def count_matrix(gt_rows, warn_by_key):
    """UNSOUND below 100% coverage -- see item 2 in the module docstring.

    Matches the tool's warning COUNT against the oracle's flip COUNT per site.
    The tool's warnings span its whole run; the oracle's counts span the
    adjudicated prefix. Where the excess exceeds the room available in the
    prefix it is clamped, which conceals the mismatch rather than fixing it.
    `over` counts how often that happened.
    """
    tp = tn = fp = fn = 0
    over = 0
    for r in gt_rows:
        if r["class"] == "DEAD":
            continue
        e, a = r["executions"], r["flips"]
        b = warn_by_key.get(r["key"], 0)
        hit = min(a, b)
        tp += hit
        fn += a - hit
        excess = b - hit
        room = e - a
        if excess > room:
            over += 1
            excess = room
        fp += excess
        tn += room - excess
    return tp, tn, fp, fn, over


# ------------------------------------------------------------------- metrics

def far(fp, total):
    """False alarms per million items.

    On GT=0 cells -- where the oracle found no flips at all -- precision,
    recall and F1 are every one of them 0/0 and the row would be all dashes.
    The false-alarm rate is well defined there and is the only thing those
    cells can say about a tool.
    """
    return fp * 1e6 / total if total else float("nan")


def prf(tp, tn, fp, fn):
    P = tp / (tp + fp) if tp + fp else float("nan")
    R = tp / (tp + fn) if tp + fn else float("nan")
    F = 2 * tp / (2 * tp + fp + fn) if (2 * tp + fp + fn) else float("nan")
    tot = tp + tn + fp + fn
    Acc = (tp + tn) / tot if tot else float("nan")
    return P, R, F, Acc


def fmt(x):
    return "—" if x != x else "%.4f" % x          # NaN check


def micro(rows, idx):
    """Micro-averaged P/R/F1 over a set of rows: pool the cells, then divide.

    Micro-averaging is the defensible suite-level number here. Macro-averaging
    over cells would give a benchmark with one flipping site the same weight
    as one with hundreds, and the GT=0 cells contribute an undefined F1 that
    has to be dropped, which quietly changes the denominator.
    """
    tp = sum(r[idx] for r in rows)
    tn = sum(r[idx + 1] for r in rows)
    fp = sum(r[idx + 2] for r in rows)
    fn = sum(r[idx + 3] for r in rows)
    return (tp, tn, fp, fn) + prf(tp, tn, fp, fn)


# -------------------------------------------------------------------- tables

def render(title, note, rows, base, w_label, w_tool, w_cfg, coverage_by_row):
    """Render one confusion table. `base` indexes the first of TP,TN,FP,FN."""
    hdr = ("%-*s  %-*s  %-*s  %11s %11s %11s %9s  %8s %8s %8s %9s %11s"
           % (w_label, "cell", w_tool, "tool", w_cfg, "config",
              "TP", "TN", "FP", "FN", "prec", "recall", "F1", "accuracy",
              "FP/M"))
    out = [title, note, "", hdr, "-" * len(hdr)]
    last = None
    for r in rows:
        if last is not None and r[0] != last:
            out.append("")
        last = r[0]
        tp, tn, fp, fn = r[base:base + 4]
        P, R, F, Acc = prf(tp, tn, fp, fn)
        cov = coverage_by_row.get((r[0], r[1], r[2]))
        mark = "" if cov is None or cov >= 99.995 else " *"
        out.append("%-*s  %-*s  %-*s  %11s %11s %11s %9s  %8s %8s %8s %9s %11s%s"
                   % (w_label, r[0], w_tool, r[1], w_cfg, r[2],
                      "{:,}".format(tp), "{:,}".format(tn),
                      "{:,}".format(fp), "{:,}".format(fn),
                      fmt(P), fmt(R), fmt(F), fmt(Acc),
                      ("%.0f" % far(fp, tp + tn + fp + fn))
                      if (tp + tn + fp + fn) else "—",
                      mark))
    mtp, mtn, mfp, mfn, mP, mR, mF, mAcc = micro(rows, base)
    out.append("-" * len(hdr))
    out.append("%-*s  %-*s  %-*s  %11s %11s %11s %9s  %8s %8s %8s %9s %11s"
               % (w_label, "MICRO (all rows pooled)", w_tool, "", w_cfg, "",
                  "{:,}".format(mtp), "{:,}".format(mtn),
                  "{:,}".format(mfp), "{:,}".format(mfn),
                  fmt(mP), fmt(mR), fmt(mF), fmt(mAcc),
                  ("%.0f" % far(mfp, mtp + mtn + mfp + mfn))
                  if (mtp + mtn + mfp + mfn) else "—"))
    return out


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--row", nargs="+", action="append",
                    metavar="LABEL SITES_TXT TOOL_SUMMARY [COVERAGE]",
                    help="one (cell, ground truth, tool) triple, optionally "
                         "followed by the census coverage percentage; "
                         "repeatable")
    ap.add_argument("--fallback", choices=["none", "function"], default="none",
                    help="none (default): a flagged site whose file:line is "
                         "not in the census stays unresolved. function: fold "
                         "it onto the single census site in the same function, "
                         "if there is exactly one. This LOOSENS the matching "
                         "criterion and can inflate a tool's score, since the "
                         "two may be different branches -- every such match is "
                         "listed so it can be audited.")
    ap.add_argument("--unsound-count-mode", action="store_true",
                    help="ALSO print the warning-count matrix. Compares the "
                         "tool's full-run warning counts against the oracle's "
                         "prefix flip counts, which are different windows "
                         "whenever coverage < 100%%. For inspection only; do "
                         "not publish it.")
    ap.add_argument("--rows-from", metavar="FILE",
                    help="read rows from a file instead of (or in addition "
                         "to) --row. One row per line, TAB-separated: LABEL, "
                         "sites.txt, tool summary, and optionally COVERAGE. "
                         "Blank lines and lines starting with # are ignored. "
                         "Intended for sweeps like the seven NAS benchmarks, "
                         "where writing 28 --row flags by hand is "
                         "unmanageable.")
    ap.add_argument("--out", help="also write the tables to this .txt file")
    ap.add_argument("--sort", action="store_true",
                    help="within each cell, order rows by site F1 descending")
    ap.add_argument("--no-check", action="store_true",
                    help="do not abort on a window-consistency failure. Only "
                         "for debugging a malformed sites.txt -- a failure "
                         "means the table is wrong.")
    args = ap.parse_args()

    rows = []
    for r in (args.row or []):
        if len(r) not in (3, 4):
            sys.exit("--row takes 3 or 4 values (LABEL SITES_TXT TOOL_SUMMARY "
                     "[COVERAGE]), got %d: %r" % (len(r), r))
        rows.append(list(r))
    if args.rows_from:
        for ln, line in enumerate(open(args.rows_from), 1):
            line = line.rstrip("\n")
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            parts = [p.strip() for p in line.split("\t") if p.strip() != ""]
            if len(parts) not in (3, 4):
                sys.exit("%s line %d: expected 3 or 4 TAB-separated fields, "
                         "got %d\n  %r" % (args.rows_from, ln, len(parts),
                                           line))
            rows.append(parts)
    if not rows:
        sys.exit("no rows: pass --row or --rows-from")

    lines, failed, warn_over, checks = [], [], [], []
    fuzzy, unres, amb, nflag = {}, {}, {}, {}
    coverage_by_row = {}

    for row in rows:
        label, gt_path, tool_path = row[0], row[1], row[2]
        cov = None
        if len(row) == 4:
            try:
                cov = float(row[3].rstrip("%"))
            except ValueError:
                sys.exit("coverage must be a number, got %r" % row[3])

        gt = A.load_gt(gt_path)
        if not gt:
            sys.exit("no site rows parsed from %s" % gt_path)

        # Universe totals, derived from the SAME file the row is scored
        # against. These are what the matrices must reproduce.
        exp_events = sum(r["executions"] for r in gt if r["class"] != "DEAD")
        exp_sites = sum(1 for r in gt if r["class"] != "DEAD")

        blocks = A.load_tool(tool_path)

        tool = "tool"
        for part in os.path.abspath(tool_path).split(os.sep):
            if "eftsan" in part.lower():
                tool = "EFTSan"
            elif "fpchecker" in part.lower():
                tool = "FPChecker"

        by_func = {r["key"] for r in gt}
        by_file = {}
        for r in gt:
            by_file.setdefault(r["fkey"], []).append(r["key"])
        gt_keys = {r["key"] for r in gt}
        by_fname = {}
        for r in gt:
            if r["func"]:
                by_fname.setdefault(r["func"], set()).add(r["key"])

        if not blocks:
            # A summary with no flip block is a tool that did not produce a
            # result -- EFTSanitizer refuses QuickSilver's merged module over
            # vector-typed loads, for instance. That is a capability finding,
            # not a detection of nothing, so it gets a row that says so rather
            # than aborting the whole table or being silently omitted.
            failed.append((label, tool, tool_path))
            continue

        for block, sites in blocks.items():
            flagged, dropped, tool_amb = set(), [], []
            extra_warn = {}
            for s in sites:
                keys = [k for k in A.resolve(s, by_func, by_file)
                        if k in gt_keys]
                if keys:
                    flagged.update(keys)
                elif s.get("tool_ambiguous"):
                    tool_amb.append(s)
                elif (args.fallback == "function" and s["func"]
                      and len(by_fname.get(s["func"], ())) == 1):
                    k = next(iter(by_fname[s["func"]]))
                    flagged.add(k)
                    fuzzy.setdefault((label, tool,
                                      "%s:%d [%s]  ->  line %d"
                                      % (s["file"], s["line"], s["func"],
                                         k[1])), set()).add(block)
                    extra_warn[k] = extra_warn.get(k, 0) + s["flips"]
                else:
                    dropped.append(s)

            # A flagged site that resolves to nothing in the census
            # contributes to no column. Silently dropping it makes a
            # resolution failure look like a tool that found nothing.
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

            s_tp, s_tn, s_fp, s_fn = site_matrix(gt, flagged)
            e_tp, e_tn, e_fp, e_fn = event_matrix(gt, flagged)

            # WINDOW CONSISTENCY. Both matrices partition the same universe,
            # so their cells must sum to the totals derived from sites.txt.
            # A mismatch means the ground truth and the scoring disagree
            # about what is being scored -- the failure that yields a wrong
            # table that looks right.
            if s_tp + s_tn + s_fp + s_fn != exp_sites:
                checks.append("%s / %s / %s: site cells sum to %d, expected "
                              "%d executed site(s)"
                              % (label, tool, block,
                                 s_tp + s_tn + s_fp + s_fn, exp_sites))
            if e_tp + e_tn + e_fp + e_fn != exp_events:
                checks.append("%s / %s / %s: event cells sum to %s, expected "
                              "%s adjudicated event(s)"
                              % (label, tool, block,
                                 "{:,}".format(e_tp + e_tn + e_fp + e_fn),
                                 "{:,}".format(exp_events)))

            c_tp = c_tn = c_fp = c_fn = 0
            if args.unsound_count_mode:
                warn_by_key = {}
                for s2 in sites:
                    for k in A.resolve(s2, by_func, by_file):
                        if k in gt_keys:
                            warn_by_key[k] = warn_by_key.get(k, 0) + s2["flips"]
                            break
                for k, v in extra_warn.items():
                    warn_by_key[k] = warn_by_key.get(k, 0) + v
                c_tp, c_tn, c_fp, c_fn, over = count_matrix(gt, warn_by_key)
                if over:
                    warn_over.append(
                        "  %s / %s / %s: %d site(s) where the tool emitted "
                        "more warnings than the site had adjudicated "
                        "executions" % (label, tool, block, over))

            coverage_by_row[(label, tool, block)] = cov
            lines.append([label, tool, block,
                          s_tp, s_tn, s_fp, s_fn,
                          e_tp, e_tn, e_fp, e_fn,
                          c_tp, c_tn, c_fp, c_fn])

    if checks and not args.no_check:
        sys.stderr.write("WINDOW CONSISTENCY FAILURE -- refusing to print a "
                         "table that would be wrong:\n")
        for c in checks:
            sys.stderr.write("  %s\n" % c)
        sys.stderr.write(
            "\nThe cells must partition the universe described by sites.txt.\n"
            "Most likely the sites.txt was produced by a different run than\n"
            "the one the tool summary came from, or it was concatenated from\n"
            "more than one diff. Regenerate it with:\n"
            "  brtrace_diff_mtu.py A.out B.out --mods <dir> --sites-txt <f>\n"
            "Pass --no-check to print anyway (the table will be wrong).\n")
        return 2

    if not lines:
        print("no scorable rows")
        if failed:
            for label, tool, path in failed:
                print("  NO RESULT  %-18s %-10s %s" % (label, tool, path))
        return 0

    order = list(dict.fromkeys(r[0] for r in lines))
    grouped = []
    for cell in order:
        sub = [r for r in lines if r[0] == cell]
        if args.sort:
            def f1_of(r):
                F = prf(r[3], r[4], r[5], r[6])[2]
                return -(F if F == F else -1)
            sub.sort(key=f1_of)
        grouped.extend(sub)

    w_label = max([len(r[0]) for r in grouped] + [len("MICRO (all rows pooled)")])
    w_tool = max([len(r[1]) for r in grouped] + [4])
    w_cfg = max([len(r[2]) for r in grouped] + [6])

    out = render(
        "SITE-BASED  (PRIMARY -- one executed branch site = one item)",
        "  TP flagged & flips   FP flagged & never flips   "
        "FN flips & not flagged   TN neither",
        grouped, 3, w_label, w_tool, w_cfg, coverage_by_row)

    out += ["", ""]
    out += render(
        "EVENT-BASED  (SECONDARY -- site verdict broadcast to every "
        "adjudicated event)",
        "  Weighted by how hot each site is; one hot site can move accuracy "
        "by tens of percent.",
        grouped, 7, w_label, w_tool, w_cfg, coverage_by_row)

    if args.unsound_count_mode:
        out += ["", ""]
        out += render(
            "WARNING-COUNT  (UNSOUND below 100% coverage -- DO NOT PUBLISH)",
            "  Tool warning counts span its WHOLE run; oracle counts span "
            "the adjudicated PREFIX.",
            grouped, 11, w_label, w_tool, w_cfg, coverage_by_row)
        out += ["",
                "  The two columns are measured over different windows. "
                "FPChecker emits no",
                "  event indices, so its warnings cannot be restricted to the "
                "prefix. Excess",
                "  beyond the prefix is clamped, which hides the mismatch "
                "rather than fixing",
                "  it. Read as volume agreement only."]
        if warn_over:
            out += ["", "  Sites where warnings exceeded adjudicated "
                    "executions (FP clamped):"] + warn_over

    if any(v is not None and v < 99.995 for v in coverage_by_row.values()):
        out += ["", "* PREFIX CENSUS (coverage < 100%)",
                "  Lock-step ended at a divergence, so the census describes "
                "only the prefix.",
                "  A site that first flips AFTER that point is recorded as "
                "never-flipping, so",
                "  a tool that flags it is charged an FP it may not deserve:",
                "      FP is an UPPER bound, FN is a LOWER bound",
                "      precision is a LOWER bound, recall is an UPPER bound",
                "  Coverage must appear in any caption built from these rows."]
        for (lab, tl, blk), cov in sorted(coverage_by_row.items()):
            if cov is not None and cov < 99.995:
                out.append("    %-18s %-10s %-8s  coverage %.2f%%"
                           % (lab, tl, blk, cov))

    out += ["",
            "FP/M = false alarms per million items; the only defined metric "
            "on GT=0 cells,",
            "       where precision, recall and F1 are all 0/0.",
            "MICRO pools the cells across every row, then divides. Preferred "
            "over macro-",
            "       averaging, which would weight a one-site benchmark the "
            "same as a",
            "       hundred-site one and has to drop the undefined GT=0 cells.",
            "",
            "Accuracy is the weakest column in both tables: the positive class "
            "is a tiny",
            "fraction of the universe, so a silent detector scores well. Use "
            "F1 where GT>0",
            "and FP/M where GT=0."]

    if failed:
        out += ["", "NO RESULT (tool produced no flip data)",
                "  The tool did not run to completion on this benchmark, so "
                "there is nothing",
                "  to score. This is a capability finding and is NOT the same "
                "as detecting",
                "  nothing -- do not read it as recall 0."]
        for label, tool, path in failed:
            out.append("    %-18s %-10s %s" % (label, tool, path))

    if fuzzy:
        out += ["", "MATCHED BY FUNCTION, NOT BY LINE (--fallback function)",
                "  The tool reported a line that is not in the census, so it "
                "was folded onto",
                "  the only census site in the same function. These may be "
                "DIFFERENT branches:",
                "  a loosened criterion that inflates the tool's counts if "
                "they are. Audit",
                "  each one before relying on the row."]
        for (label, tool, where), blocks_ in sorted(fuzzy.items()):
            out.append("    %-18s %-10s %s   [%s]"
                       % (label, tool, where, ", ".join(sorted(blocks_))))

    if unres:
        out += ["", "OUTSIDE THE CENSUS UNIVERSE (contributed to no column)",
                "  Sites the tool flagged that brtrace never instrumented, so "
                "the oracle has",
                "  no verdict on them. Counting them as FP would punish the "
                "tool for looking",
                "  where the oracle did not. The fcmp->select class lands here "
                "when brtrace",
                "  was built without select instrumentation."]
        for (label, tool, where), blocks_ in sorted(unres.items()):
            n = nflag.get((label, tool), 0)
            out.append("    %-18s %-10s %s" % (label, tool, where))
            out.append("        in %s (1 of ~%d flagged site(s))"
                       % (", ".join(sorted(blocks_)), n))

    if amb:
        out += ["", "UNRESOLVED BY THE TOOL ITSELF (contributed to no column)",
                "  The tool reported a line number it could not attribute to a "
                "file, so it",
                "  cannot be matched. Not a scoring failure -- the tool's own "
                "output is",
                "  ambiguous. Its events are absent from FP, so the false-alarm "
                "figures are",
                "  slight UNDERcounts."]
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
