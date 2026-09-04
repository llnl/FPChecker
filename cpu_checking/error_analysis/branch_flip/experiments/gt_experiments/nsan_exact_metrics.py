#!/usr/bin/env python3
"""nsan_exact_metrics.py -- score NSan against brtrace by matching individual
branch executions on the occurrence index both sides emit.

    ./nsan_exact_metrics.py                       # everything it can find
    ./nsan_exact_metrics.py -b LULESH AMG
    ./nsan_exact_metrics.py --json out.json --latex out.tex --text out.txt

Counterpart of fpc_exact_metrics.py: same join, same window (k < E_S), same
universe (window.csv), same path-equivalence gate. Differences are properties
of the tool: NSan has no DECLINED verdict (its check is an exact boolean
disagreement, so every verdict is FLIP); policy variants (resume-discard,
eq-truncated) replace the eta sweep and appear as separate rows; select and
oos events are not scored against the branch census.

Occurrence indices are 0-based on both sides. A cell with k >= E_S at full
coverage is refused, since that is the signature of a one-based tick.

Inputs:
    <gt>/<bench>/results/O0/<pair>/{window.csv,flips.csv,report.txt}
    <nsan>/<bench>/results/O0/<prec>[_<policy>]/events_O0.log
"""

import argparse
import csv
import json
import os
import re
import sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))

BENCH_ORDER = ["LULESH", "AMG", "QuickSilver", "BT", "CG", "EP", "IS", "LU",
               "MG", "SP"]

EVENT_RE = re.compile(
    r"^#NSAN_EVENT\s+mod=(\d+)\s+site=(-?\d+)\s+k=(\d+)\s+kind=(\w+)\s+"
    r"verdict=(\w+)")
SITE_RE = re.compile(
    r"^#NSAN_SITE\s+(\d+)\s+(-?\d+)\s+(\w+)\s+(\d+)\s+(\d+)")
TOTALS_RE = re.compile(
    r"^#NSAN_TOTALS\s+events=(\d+)\s+out_of_scope=(\d+)\s+unticked=(\d+)\s+"
    r"overflow=(\d+)")
COVERAGE_RE = re.compile(r"lock-step compared\s+([\d,]+)\s+events\s+"
                         r"\(([\d.]+)% of longer trace\)")


# ---------------------------------------------------------------- oracle side

def read_window(path):
    """window.csv -> {(mod, site): (E_S, flips, location, n_fcmp)}"""
    out = {}
    if not os.path.isfile(path):
        return out
    with open(path) as fh:
        for r in csv.DictReader(fh):
            if r.get("kind") != "branch":
                continue
            out[(int(r["module_id"]), int(r["site_id"]))] = (
                int(r["E_S"]), int(r["flips"]), r["location"],
                int(r.get("n_fcmp", 1)))
    return out


def sites_from_window(win):
    """Scoring universe {(mod, site): (E_S, flips, loc)}; E_S == 0 sites are
    dropped as unadjudicated, as in the FPChecker and EFTSan scorers."""
    out, dead = {}, 0
    for k, (E_S, flips, loc, _n) in win.items():
        if E_S == 0:
            dead += 1
            continue
        out[k] = (E_S, flips, loc)
    return out, dead


def read_coverage(path):
    if not os.path.isfile(path):
        return None
    m = COVERAGE_RE.search(open(path, errors="replace").read())
    return float(m.group(2)) if m else None


def read_flips(path):
    """flips.csv -> {(mod, site): set(occ_index)}"""
    out = defaultdict(set)
    if not os.path.isfile(path):
        return out
    with open(path) as fh:
        for row in csv.DictReader(fh):
            if row.get("kind") not in (None, "", "branch"):
                continue
            try:
                out[(int(row["module_id"]), int(row["site_id"]))].add(
                    int(row["occ_index"]))
            except (KeyError, ValueError):
                continue
    return out


# ------------------------------------------------------------------ nsan side

def read_events(path):
    """event log -> ({(mod, site): set(k)}, kind_counts, max_k); branch only."""
    out = defaultdict(set)
    kinds = defaultdict(int)
    max_k = defaultdict(int)
    if not os.path.isfile(path):
        return None, kinds, max_k
    with open(path, errors="replace") as fh:
        for line in fh:
            m = EVENT_RE.match(line)
            if not m:
                continue
            mod, site, k, kind = (int(m.group(1)), int(m.group(2)),
                                  int(m.group(3)), m.group(4))
            kinds[kind] += 1
            if kind != "branch":
                continue
            out[(mod, site)].add(k)
            if k > max_k[(mod, site)]:
                max_k[(mod, site)] = k
    return out, kinds, max_k


def read_site_totals(path):
    """event log -> ({(mod, site): executions}, totals dict)"""
    out, totals = {}, None
    if not os.path.isfile(path):
        return out, totals
    with open(path, errors="replace") as fh:
        for line in fh:
            m = SITE_RE.match(line)
            if m:
                if m.group(3) == "branch":
                    out[(int(m.group(1)), int(m.group(2)))] = int(m.group(4))
                continue
            m = TOTALS_RE.match(line)
            if m:
                totals = {"events": int(m.group(1)),
                          "out_of_scope": int(m.group(2)),
                          "unticked": int(m.group(3)),
                          "overflow": int(m.group(4))}
    return out, totals


# -------------------------------------------------------------------- scoring

def score(sites, flips, events, skip=frozenset()):
    """Exact per-execution counts over the adjudicated region."""
    TP = FP = FN = TN = 0
    out_window = 0
    unadj_sites = unadj_events = 0
    per_branch = []

    for key, (E, _nflip, loc) in sites.items():
        if loc in skip:
            continue
        flipped = {o for o in flips.get(key, ()) if o < E}
        flagged_all = events.get(key, set())
        flagged = {k for k in flagged_all if k < E}
        out_window += len(flagged_all) - len(flagged)

        tp = len(flipped & flagged)
        fp = len(flagged - flipped)
        fn = len(flipped - flagged)
        tn = E - tp - fp - fn

        TP += tp
        FP += fp
        FN += fn
        TN += max(0, tn)
        if flipped or flagged:
            per_branch.append((loc, E, len(flipped), len(flagged),
                               tp, fp, fn))

    for key, ks in events.items():
        if key in sites or not ks:
            continue
        unadj_sites += 1
        unadj_events += len(ks)

    return dict(TP=TP, FP=FP, FN=FN, TN=TN, out_window=out_window,
                unadj_sites=unadj_sites, unadj_events=unadj_events,
                branches=per_branch)


def check_path_equivalence(sites, totals, coverage, overflow):
    """Per-site execution counts, tool vs oracle. Strict only at 100%
    coverage (E_S is windowed). A disagreeing site is excluded; widespread
    disagreement fails the cell."""
    if overflow:
        return {"checked": 0, "n_differ": 0, "worst": [], "exclude": set(),
                "strict": False, "coverage": coverage, "failed": True,
                "overflow": True}
    strict = coverage is not None and coverage >= 99.999
    checked, diffs = 0, []
    for key, (E, _nflip, loc) in sites.items():
        T = totals.get(key)
        if T is None:
            continue
        checked += 1
        if T < E or (strict and T != E):
            diffs.append((loc, E, T))
    diffs.sort(key=lambda x: abs(x[1] - x[2]), reverse=True)
    widespread = checked and (len(diffs) > max(3, 0.05 * checked))
    return {"checked": checked, "n_differ": len(diffs), "worst": diffs[:8],
            "exclude": {loc for loc, _E, _T in diffs}, "strict": strict,
            "coverage": coverage, "failed": bool(widespread),
            "overflow": False}


def metrics(TP, FP, FN, TN):
    p = TP / (TP + FP) if TP + FP else None
    r = TP / (TP + FN) if TP + FN else None
    f1 = (2 * p * r / (p + r)) if (p and r) else (
        0.0 if p is not None and r is not None else None)
    n = TP + FP + FN + TN
    return p, r, f1, (TP + TN) / n if n else None


def policy_label(dirname, prec):
    """fp32 -> 'measured'; fp32_resume-discard -> 'resume-discard'"""
    if dirname == prec:
        return "measured"
    return dirname[len(prec) + 1:] or "measured"


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-n", "--nsan",
                    default=os.environ.get(
                        "NSAN_RESULTS",
                        os.path.join(HERE, "..", "nsan_experiments")))
    ap.add_argument("-g", "--gt", default=os.environ.get("GT_RESULTS", HERE))
    ap.add_argument("-b", "--benchmarks", nargs="+")
    ap.add_argument("--text")
    ap.add_argument("--latex")
    ap.add_argument("--json")
    args = ap.parse_args()

    cells = [("LULESH", "lulesh/results/O0"),
             ("AMG", "amg/results/O0"),
             ("QuickSilver", "quicksilver/results/O0")]
    for b in ("bt", "cg", "ep", "is", "lu", "mg", "sp"):
        cells.append((b.upper(), "nas/results/%s/O0" % b))

    if args.benchmarks:
        want = {x.upper() for x in args.benchmarks}
        unknown = want - {c[0].upper() for c in cells}
        if unknown:
            print("unknown benchmark(s): %s" % ", ".join(sorted(unknown)))
            print("known: %s" % ", ".join(c[0] for c in cells))
            return 1
        cells = [c for c in cells if c[0].upper() in want]

    rows = defaultdict(dict)
    pols_seen = defaultdict(set)
    order = []

    for bench, cen in cells:
        for prec, pair in (("fp32", "fp32_vs_fp64"), ("fp64", "fp64_vs_ld")):
            base_gt = os.path.join(args.gt, cen, pair)
            window_p = os.path.join(base_gt, "window.csv")
            report_p = os.path.join(base_gt, "report.txt")
            flips_p = os.path.join(base_gt, "flips.csv")
            if not os.path.isfile(window_p):
                continue

            win = read_window(window_p)
            sites, n_dead = sites_from_window(win)
            coverage = read_coverage(report_p)
            flips = read_flips(flips_p)

            parent = os.path.join(args.nsan, cen)
            if not os.path.isdir(parent):
                continue
            variants = sorted(d for d in os.listdir(parent)
                              if d == prec or d.startswith(prec + "_"))
            if not variants:
                continue

            if not sites:
                rows[(prec, bench)]["--"] = dict(vacuous=True)
                pols_seen[prec].add("--")
                if (prec, bench) not in order:
                    order.append((prec, bench))
                continue

            for vd in variants:
                base = os.path.join(parent, vd)
                pol = policy_label(vd, prec)
                ev_path = os.path.join(base, "events_O0.log")
                if not os.path.isfile(ev_path):
                    sj = os.path.join(base, "summary.json")
                    if os.path.isfile(sj):
                        try:
                            r = json.load(open(sj))
                            cand = r.get("event_log")
                            if cand:
                                cand = os.path.join(
                                    args.nsan, cen.split("/results/")[0],
                                    cand)
                                if os.path.isfile(cand):
                                    ev_path = cand
                        except Exception:
                            pass
                if not os.path.isfile(ev_path):
                    continue

                events, kinds, max_k = read_events(ev_path)
                totals, tot = read_site_totals(ev_path)

                # unscoreable: unticked flips, or a one-based k at full coverage
                if tot and tot.get("unticked"):
                    continue
                if coverage is not None and coverage >= 99.999:
                    if any(key in sites and mk >= sites[key][0]
                           for key, mk in max_k.items()):
                        continue

                overflow = bool(tot and tot.get("overflow"))
                pe = check_path_equivalence(sites, totals, coverage, overflow)
                skip = pe["exclude"] | {
                    v[2] for k, v in win.items() if v[3] > 1}

                res = score(sites, flips, events, skip)
                rows[(prec, bench)][pol] = res
                pols_seen[prec].add(pol)
                if (prec, bench) not in order:
                    order.append((prec, bench))

    if not rows:
        sys.exit("no cells scored -- check -n and -g, and that the event "
                 "logs exist")

    ROW = "%-12s%10s%10s%9s%12s%9s%9s%9s%9s"
    RULE = "-" * 89

    def mfmt(x):
        return "--" if x is None else "%.4f" % x

    L = []
    for prec in sorted({p for p, _ in rows}):
        ps = sorted(pols_seen[prec])
        bs = [b for p, b in order if p == prec]
        bs.sort(key=lambda b: BENCH_ORDER.index(b)
                if b in BENCH_ORDER else 99)
        multi = len([x for x in ps if x != "--"]) > 1

        L.append("=" * 89)
        L.append("NSan -- %s   (branch)" % prec)
        L.append("=" * 89)
        L.append(ROW % ("benchmark", "TP", "FP", "FN", "TN", "P", "R", "F1",
                        "Acc"))
        L.append(RULE)
        for b in bs:
            for e in ps:
                r = rows[(prec, b)].get(e)
                if r is None:
                    continue
                label = ("%s %s" % (b, e))[:12] if multi else b
                if r.get("vacuous"):
                    L.append(ROW % (label, 0, 0, 0, 0, "--", "--", "--", "--"))
                    continue
                p_, rc, f1, acc = metrics(r["TP"], r["FP"], r["FN"], r["TN"])
                L.append(ROW % (label, r["TP"], r["FP"], r["FN"], r["TN"],
                                mfmt(p_), mfmt(rc), mfmt(f1), mfmt(acc)))
        L.append(RULE)
        L.append("")

    text = "\n".join(L)
    print(text)
    if args.text:
        open(args.text, "w").write(text + "\n")
        print("wrote %s" % args.text)

    if args.json:
        J = {"tool": "nsan", "cells": []}
        for (prec, b), bypol in sorted(rows.items()):
            for e, r in sorted(bypol.items()):
                cell = {"benchmark": b, "precision": prec, "policy": e}
                if r.get("vacuous"):
                    cell["vacuous"] = True
                else:
                    p, rc, f1, acc = metrics(r["TP"], r["FP"], r["FN"],
                                             r["TN"])
                    cell.update({"TP": r["TP"], "FP": r["FP"],
                                 "FN": r["FN"], "TN": r["TN"],
                                 "P": p, "R": rc, "F1": f1, "Acc": acc})
                J["cells"].append(cell)
        with open(args.json, "w") as fh:
            json.dump(J, fh, indent=2)
        print("wrote %s" % args.json)

    if args.latex:
        T = []
        for prec in sorted({p for p, _ in rows}):
            ps = sorted(pols_seen[prec])
            bs = [b for p, b in order if p == prec]
            bs.sort(key=lambda b: BENCH_ORDER.index(b)
                    if b in BENCH_ORDER else 99)
            T += ["\\begin{tabular}{llrrrrrrrr}", "\\toprule",
                  "Benchmark & Policy & TP & FP & FN & TN & P & R & F1 & "
                  "Acc \\\\", "\\midrule"]
            for b in bs:
                for e in ps:
                    r = rows[(prec, b)].get(e)
                    if r is None or r.get("vacuous"):
                        continue
                    p, rc, f1, acc = metrics(r["TP"], r["FP"], r["FN"],
                                             r["TN"])
                    T.append("%s & %s & %d & %d & %d & %d & %s & %s & %s & "
                             "%s \\\\" % (
                                 b, e.replace("_", "\\_"),
                                 r["TP"], r["FP"], r["FN"], r["TN"],
                                 *[("\\textemdash" if x is None
                                    else "%.4f" % x)
                                   for x in (p, rc, f1, acc)]))
            T += ["\\bottomrule", "\\end{tabular}", ""]
        open(args.latex, "w").write("\n".join(T) + "\n")
        print("wrote %s" % args.latex)
    return 0


if __name__ == "__main__":
    sys.exit(main())