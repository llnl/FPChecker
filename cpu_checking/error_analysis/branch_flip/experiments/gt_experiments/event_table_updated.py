#!/usr/bin/env python3
"""
event_table_updated.py -- event-based confusion matrices only, with the
window made explicit and enforced.

    python3 event_table_updated.py \\
      --row "LULESH fp32/fp64" lulesh/results/O0/fp32_vs_fp64/sites.txt \\
            ../fpchecker_experiments/lulesh/results/O0/fp32/summary.txt 21.62

Repeat --row once per (cell, tool) pair. Every eta / threshold block inside a
tool summary becomes its own line. The 4th field is the census coverage
percentage and should be supplied for every row.


THE WINDOW RULE
===============

COVERAGE = adjudicated events / max(len A, len B).

  coverage == 100%   Lock-step held to the end of both traces. Every event in
                     the run was adjudicated, so the four cells describe the
                     WHOLE TRACE and mean what they say.

  coverage <  100%   Lock-step ended at the first structural flip. Events after
                     that point are UNADJUDICATED -- not TN, not anything. The
                     four cells describe the PREFIX ONLY, and every cell must
                     be drawn from that same prefix.

All four event cells here come from sites.txt (`executions` and `flips`); the
tool contributes only a binary per-site verdict. So the cells cannot mix
windows with each other -- they are prefix-clean by construction, and the
script asserts that they sum to the adjudicated event count.

What the prefix DOES cost you is interpretation, not arithmetic. A site that
first flips after the divergence point is recorded as never-flipping, so:

    FP is an UPPER bound        precision is a LOWER bound
    FN is a LOWER bound         recall    is an UPPER bound

Rows below 100% are marked `*` and those directions are restated under the
table. Coverage must appear in any caption built from these rows.


WHY THERE IS NO COUNT MODE
==========================

Warning-count matching compared the tool's warning COUNT against the oracle's
flip COUNT per site. Those are measured over different windows the moment
coverage drops below 100%:

    oracle flips / executions : the adjudicated PREFIX   (LULESH fp32:  87,985)
    tool warnings             : the tool's ENTIRE run    (LULESH fp32: 406,861)

FPChecker emits no event indices, so there is no way to ask how many of its
warnings landed inside the prefix. The old implementation clamped the excess
(`if excess > room`), which made the totals look plausible while comparing
incommensurate quantities. It is removed rather than gated: on a prefix cell
there is no sound version of it to fall back to.


HOW THE EVENT MATRIX IS DEFINED
===============================

One adjudicated branch evaluation = one item. The tools emit warnings at
sites, not per-event predictions, so a site verdict is broadcast to every
adjudicated event at that site. For an executed site with `e` executions and
`f` oracle flips:

    tool flagged the site      ->  TP += f        FP += (e - f)
    tool did not flag the site ->  FN += f        TN += (e - f)

DEAD sites (instrumented, never executed) are excluded -- the oracle has no
verdict on them.

Note what the event FP column means: flagging a site that flips 795 times in
3,750 executions scores TP=795 AND FP=2,955. The tool is charged for every
execution of a site it warned about, even though it emitted one warning about
a site that genuinely does flip. That is a consequence of broadcasting a site
verdict to events, and it is not the usual meaning of "false positive" for a
warning-based tool.

Events are also weighted by how hot each site is. On LULESH fp64/ld a single
std::max site is 15,000 of 407,037 events, so flagging it or not swings
accuracy by tens of percent while detection quality barely moves.

ACCURACY IS THE WEAKEST COLUMN. The positive class is tiny -- 831 of 87,985
events on LULESH fp32 -- so a silent detector scores well. Use F1 where GT>0
and FP/M where GT=0.

Requires adjudicate.py in the same directory: parsing and site matching are
shared, so this and adjudicate.py resolve the std::max file-attribution
mismatch the same way.
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


# ------------------------------------------------------------------- matrix

def event_matrix(gt_rows, flagged_keys):
    """One adjudicated branch evaluation = one item."""
    tp = tn = fp = fn = 0
    for r in gt_rows:
        if r["class"] == "DEAD":
            continue                      # never executed: no events to score
        e, f = r["executions"], r["flips"]
        if r["key"] in flagged_keys:
            tp += f
            fp += e - f
        else:
            fn += f
            tn += e - f
    return tp, tn, fp, fn


# ------------------------------------------------------------------ metrics

def far(fp, total):
    """False alarms per million adjudicated events.

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


def micro(rows):
    """Micro-averaged cells over a set of rows: pool, then divide.

    Micro-averaging is the defensible suite-level number here. Macro-averaging
    over cells would give a benchmark with one flipping site the same weight as
    one with hundreds, and the GT=0 cells contribute an undefined F1 that has
    to be dropped, which quietly changes the denominator.

    Pooling across rows with DIFFERENT coverage pools different windows. That
    is unavoidable if a suite-level number is wanted at all, but the MICRO row
    is marked when any pooled row is a prefix.
    """
    tp = sum(r["tp"] for r in rows)
    tn = sum(r["tn"] for r in rows)
    fp = sum(r["fp"] for r in rows)
    fn = sum(r["fn"] for r in rows)
    return (tp, tn, fp, fn) + prf(tp, tn, fp, fn)


# ------------------------------------------------------------------- parsing

def parse_rows(args):
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
    return rows


def tool_name(path):
    name = "tool"
    for part in os.path.abspath(path).split(os.sep):
        if "eftsan" in part.lower():
            name = "EFTSan"
        elif "fpchecker" in part.lower():
            name = "FPChecker"
    return name


# -------------------------------------------------------------------- render

def render(rows, any_prefix):
    w_label = max([len(r["label"]) for r in rows]
                  + [len("MICRO (all rows pooled)")])
    w_tool = max([len(r["tool"]) for r in rows] + [4])
    w_cfg = max([len(r["block"]) for r in rows] + [6])

    hdr = ("%-*s  %-*s  %-*s  %11s %11s %11s %9s  %8s %8s %8s %9s %9s %13s"
           % (w_label, "cell", w_tool, "tool", w_cfg, "config",
              "TP", "TN", "FP", "FN", "prec", "recall", "F1", "accuracy",
              "FP/Mevt", "adjudicated"))
    out = ["EVENT-BASED  (site verdict broadcast to every adjudicated event)",
           "  TP oracle flips at flagged sites      FP non-flips at flagged "
           "sites",
           "  FN oracle flips at unflagged sites    TN non-flips at unflagged "
           "sites",
           "", hdr, "-" * len(hdr)]

    last = None
    for r in rows:
        if last is not None and r["label"] != last:
            out.append("")
        last = r["label"]
        tp, tn, fp, fn = r["tp"], r["tn"], r["fp"], r["fn"]
        P, R, F, Acc = prf(tp, tn, fp, fn)
        tot = tp + tn + fp + fn
        cov = r["cov"]
        mark = "" if cov is None or cov >= 99.995 else " *"
        out.append("%-*s  %-*s  %-*s  %11s %11s %11s %9s  %8s %8s %8s %9s "
                   "%9s %13s%s"
                   % (w_label, r["label"], w_tool, r["tool"], w_cfg, r["block"],
                      "{:,}".format(tp), "{:,}".format(tn),
                      "{:,}".format(fp), "{:,}".format(fn),
                      fmt(P), fmt(R), fmt(F), fmt(Acc),
                      ("%.0f" % far(fp, tot)) if tot else "—",
                      "{:,}".format(tot), mark))

    mtp, mtn, mfp, mfn, mP, mR, mF, mAcc = micro(rows)
    mtot = mtp + mtn + mfp + mfn
    out.append("-" * len(hdr))
    out.append("%-*s  %-*s  %-*s  %11s %11s %11s %9s  %8s %8s %8s %9s %9s %13s%s"
               % (w_label, "MICRO (all rows pooled)", w_tool, "", w_cfg, "",
                  "{:,}".format(mtp), "{:,}".format(mtn),
                  "{:,}".format(mfp), "{:,}".format(mfn),
                  fmt(mP), fmt(mR), fmt(mF), fmt(mAcc),
                  ("%.0f" % far(mfp, mtot)) if mtot else "—",
                  "{:,}".format(mtot), " *" if any_prefix else ""))
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
    ap.add_argument("--rows-from", metavar="FILE",
                    help="read rows from a file instead of (or in addition "
                         "to) --row. One row per line, TAB-separated: LABEL, "
                         "sites.txt, tool summary, and optionally COVERAGE. "
                         "Blank lines and lines starting with # are ignored.")
    ap.add_argument("--require-coverage", action="store_true",
                    help="abort if any row omits the coverage field. Use this "
                         "for anything headed into the paper: an unmarked row "
                         "is indistinguishable from a 100%% one in the output.")
    ap.add_argument("--out", help="also write the table to this .txt file")
    ap.add_argument("--sort", action="store_true",
                    help="within each cell, order rows by F1 descending")
    ap.add_argument("--no-check", action="store_true",
                    help="do not abort on a window-consistency failure. Only "
                         "for debugging a malformed sites.txt -- a failure "
                         "means the table is wrong.")
    args = ap.parse_args()

    rows = parse_rows(args)

    scored, failed, checks = [], [], []
    fuzzy, unres, amb, nflag = {}, {}, {}, {}

    for row in rows:
        label, gt_path, tool_path = row[0], row[1], row[2]
        cov = None
        if len(row) == 4:
            try:
                cov = float(row[3].rstrip("%"))
            except ValueError:
                sys.exit("coverage must be a number, got %r" % row[3])
        elif args.require_coverage:
            sys.exit("--require-coverage: row %r has no coverage field" % label)

        gt = A.load_gt(gt_path)
        if not gt:
            sys.exit("no site rows parsed from %s" % gt_path)

        # The adjudicated universe, derived from the SAME file the row is
        # scored against.
        exp_events = sum(r["executions"] for r in gt if r["class"] != "DEAD")

        # PER-SITE WINDOW INVARIANT. The sum check below is structural -- in
        # site mode the cells and exp_events are both sum(executions), so they
        # agree no matter how corrupt sites.txt is. THIS is the check that
        # bites: a site cannot flip more often than it executed. If the census
        # wrote FULL-RUN flips against PREFIX executions, that is where it
        # shows up.
        for r in gt:
            if r["class"] == "DEAD":
                continue
            if r["executions"] < 0 or r["flips"] < 0:
                checks.append("%s: %s:%d has a negative count "
                              "(executions=%d flips=%d)"
                              % (gt_path, r["key"][0], r["key"][1],
                                 r["executions"], r["flips"]))
            if r["flips"] > r["executions"]:
                checks.append("%s: %s:%d flips (%s) EXCEED executions (%s) -- "
                              "flips and executions are being measured over "
                              "different windows"
                              % (gt_path, r["key"][0], r["key"][1],
                                 "{:,}".format(r["flips"]),
                                 "{:,}".format(r["executions"])))

        blocks = A.load_tool(tool_path)
        tool = tool_name(tool_path)

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

            tp, tn, fp, fn = event_matrix(gt, flagged)

            # Structural backstop. In site mode this cannot fail for a
            # well-formed census -- the cells and exp_events are both
            # sum(executions) over non-DEAD rows. It is retained to catch a
            # loader change that drops or double-counts rows. The check that
            # detects a mixed-window census is the per-site one above.
            if tp + tn + fp + fn != exp_events:
                checks.append("%s / %s / %s: event cells sum to %s, expected "
                              "%s adjudicated event(s)"
                              % (label, tool, block,
                                 "{:,}".format(tp + tn + fp + fn),
                                 "{:,}".format(exp_events)))

            scored.append({"label": label, "tool": tool, "block": block,
                           "tp": tp, "tn": tn, "fp": fp, "fn": fn,
                           "cov": cov, "gt_path": gt_path,
                           "adjudicated": exp_events})

    if checks and not args.no_check:
        sys.stderr.write("WINDOW CONSISTENCY FAILURE -- refusing to print a "
                         "table that would be wrong:\n")
        for c in checks:
            sys.stderr.write("  %s\n" % c)
        sys.stderr.write(
            "\nThe cells must partition the adjudicated universe described by\n"
            "sites.txt. Most likely the sites.txt came from a different run\n"
            "than the tool summary, or it was concatenated from more than one\n"
            "diff. Regenerate it with:\n"
            "  brtrace_diff_mtu.py A.out B.out --mods <dir> --sites-txt <f>\n"
            "Pass --no-check to print anyway (the table will be wrong).\n")
        return 2

    if not scored:
        print("no scorable rows")
        for label, tool, path in failed:
            print("  NO RESULT  %-18s %-10s %s" % (label, tool, path))
        return 0

    order = list(dict.fromkeys(r["label"] for r in scored))
    grouped = []
    for cell in order:
        sub = [r for r in scored if r["label"] == cell]
        if args.sort:
            def f1_of(r):
                F = prf(r["tp"], r["tn"], r["fp"], r["fn"])[2]
                return -(F if F == F else -1)
            sub.sort(key=f1_of)
        grouped.extend(sub)

    any_prefix = any(r["cov"] is not None and r["cov"] < 99.995
                     for r in grouped)
    out = render(grouped, any_prefix)

    # Cross-check the census window against the declared coverage. If
    # sites.txt carries FULL-RUN executions while its flips are prefix-only,
    # the implied trace length collapses toward the adjudicated count and the
    # tail goes to roughly zero -- which is the signature of that bug.
    seen = {}
    for r in grouped:
        if r["cov"] is None or r["gt_path"] in seen:
            continue
        seen[r["gt_path"]] = True
        if r["cov"] < 99.995 and r["cov"] > 0:
            implied = r["adjudicated"] / (r["cov"] / 100.0)
            if not out or out[-1] != "":
                out.append("")
            out.append("  window check  %-18s adjudicated %s / coverage "
                       "%.2f%%  ->  implied full run %s, unadjudicated tail %s"
                       % (r["label"], "{:,}".format(r["adjudicated"]),
                          r["cov"], "{:,.0f}".format(implied),
                          "{:,.0f}".format(implied - r["adjudicated"])))

    if any_prefix:
        out += ["", "* PREFIX CENSUS (coverage < 100%)",
                "  Lock-step ended at the first structural flip, so the census "
                "describes only",
                "  the prefix and every cell above is a prefix count. A site "
                "that first flips",
                "  AFTER that point is recorded as never-flipping, so a tool "
                "that flags it is",
                "  charged an FP it may not deserve:",
                "      FP is an UPPER bound, FN is a LOWER bound",
                "      precision is a LOWER bound, recall is an UPPER bound",
                "  Coverage must appear in any caption built from these rows. "
                "The MICRO row",
                "  pools rows measured over different windows and inherits the "
                "same bounds.",
                ""]
        for r in grouped:
            if r["cov"] is not None and r["cov"] < 99.995:
                out.append("    %-18s %-10s %-8s  coverage %6.2f%%  "
                           "adjudicated %s"
                           % (r["label"], r["tool"], r["block"], r["cov"],
                              "{:,}".format(r["adjudicated"])))

    missing = [r for r in grouped if r["cov"] is None]
    if missing:
        out += ["", "NO COVERAGE SUPPLIED (window unknown)",
                "  These rows print without a prefix marker, which is "
                "indistinguishable from",
                "  100% coverage. Supply the 4th field, or run with "
                "--require-coverage."]
        for r in missing:
            out.append("    %-18s %-10s %-8s  %s"
                       % (r["label"], r["tool"], r["block"], r["gt_path"]))

    out += ["",
            "FP/Mevt = false alarms per million adjudicated events; the only "
            "defined metric",
            "          on GT=0 cells, where precision, recall and F1 are all "
            "0/0.",
            "MICRO pools the cells across every row, then divides. Preferred "
            "over macro-",
            "      averaging, which would weight a one-site benchmark the same "
            "as a",
            "      hundred-site one and has to drop the undefined GT=0 cells.",
            "",
            "Events are weighted by execution count, so one hot site can move "
            "accuracy by",
            "tens of percent while detection quality barely changes. Accuracy "
            "is also",
            "inflated by silence, since the positive class is a tiny fraction "
            "of events.",
            "Use F1 where GT>0 and FP/Mevt where GT=0."]

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
