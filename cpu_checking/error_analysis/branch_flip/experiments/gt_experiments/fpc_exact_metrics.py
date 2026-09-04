#!/usr/bin/env python3
"""fpc_exact_metrics.py -- score FPChecker against brtrace by matching
individual branch executions on the occurrence index both sides emit.

    ./fpc_exact_metrics.py                        # interval rule
    ./fpc_exact_metrics.py --rule shadow          # shadow-predicate rule
    ./fpc_exact_metrics.py --rule both
    ./fpc_exact_metrics.py -b LULESH AMG --eta 1e-6
    ./fpc_exact_metrics.py --declined silence
    ./fpc_exact_metrics.py --json out.json --latex out.tex --text out.txt

Join key is (module_id, site_id, k), window is k <= E_S, universe is
window.csv with E_S == 0 sites dropped. Path equivalence is checked per
site; disagreeing sites and n_fcmp > 1 sites are excluded from event scoring.

The interval rule can abstain (DECLINED): --declined exclude (default) scores
only executions it judged; --declined silence counts an abstention as a
stability verdict. The shadow rule (SFLIP/SFLIPNF) never abstains.

Inputs:
    <gt>/<bench>/results/O0/<pair>/{window.csv|sites.txt,flips.csv,report.txt}
    <fpc>/<bench>/results/O0/<prec>[_rule-both|_rule-shadow]/summary.json
        and the events_O0_eta<E>[.<rule>].log it names
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

RULES = {
    "interval": {"flag": frozenset({"UNSTABLE"}), "abstain": "DECLINED",
                 "nf": frozenset(), "label": "interval rule"},
    "shadow":   {"flag": frozenset({"SFLIP", "SFLIPNF"}), "abstain": None,
                 "nf": frozenset({"SFLIPNF"}), "label": "shadow rule"},
}

EVENT_RE = re.compile(r"mod=(\d+)\s+site=(\d+)\s+k=(\d+)\s+verdict=([A-Z_]+)")
COVERAGE_RE = re.compile(r"lock-step compared\s+([\d,]+)\s+events\s+"
                         r"\(([\d.]+)% of longer trace\)")
SITE_RE = re.compile(r"^#FPC_SITE\s+(\d+)\s+(-?\d+)\s+(\d+)")
OVERFLOW_RE = re.compile(r"#FPC_SITES WARNING: the site table filled up")


# ---------------------------------------------------------------- oracle side

def read_sites(path):
    """sites.txt -> {(mod, site): (executions, flips, location)} (legacy)."""
    out = {}
    with open(path) as fh:
        for line in fh:
            if line.startswith("#") or not line.strip():
                continue
            p = line.split(None, 6)
            if len(p) < 7 or p[0] not in ("TP", "TN"):
                continue
            out[(int(p[1]), int(p[2]))] = (int(p[4]), int(p[3]), p[6].strip())
    return out


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
    """Scoring universe; E_S == 0 sites are dropped as unadjudicated."""
    out = {}
    for k, (E_S, flips, loc, _n) in win.items():
        if E_S == 0:
            continue
        out[k] = (E_S, flips, loc)
    return out


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


# ---------------------------------------------------------------- tool side

def read_site_totals(path):
    """event log -> ({(mod, site): executions}, overflowed)"""
    out, overflow = {}, False
    if not os.path.isfile(path):
        return out, overflow
    with open(path, errors="replace") as fh:
        for line in fh:
            if line.startswith("#FPC_SITE"):
                m = SITE_RE.match(line)
                if m:
                    out[(int(m.group(1)), int(m.group(2)))] = int(m.group(3))
                elif OVERFLOW_RE.search(line):
                    overflow = True
    return out, overflow


def read_tool_status(path):
    if not os.path.isfile(path):
        return False
    try:
        recs = json.load(open(path))
    except Exception:
        return False
    if isinstance(recs, dict):
        recs = [recs]
    return any(isinstance(r, dict) and r.get("status") == "tool_failure"
               for r in recs)


def read_events(path):
    """event log -> {(mod, site): {verdict: set(k)}}"""
    out = defaultdict(lambda: defaultdict(set))
    if not os.path.isfile(path):
        return None
    with open(path, errors="replace") as fh:
        for line in fh:
            m = EVENT_RE.search(line)
            if m:
                out[(int(m.group(1)), int(m.group(2)))][m.group(4)].add(
                    int(m.group(3)))
    return out


def resolve_event_log(rec, rule, base_fp, eta):
    byrule = rec.get("event_log_by_rule") or {}
    cand = byrule.get(rule) or rec.get("event_log")
    if cand:
        for p in (cand, os.path.join(base_fp, os.path.basename(cand))):
            if os.path.isfile(p):
                return p
    for name in ("events_O0_eta%s.%s.log" % (eta, rule),
                 "events_O0_eta%s.log" % eta,
                 "events_O0_shadow.%s.log" % rule,
                 "events_O0_shadow.log"):
        p = os.path.join(base_fp, name)
        if os.path.isfile(p):
            return p
    return None


def fp_dir(fpc_root, cen, prec, rule, suffix):
    base = os.path.join(fpc_root, cen)
    if suffix is not None:
        return os.path.join(base, prec + suffix)
    if rule == "interval":
        return os.path.join(base, prec)
    for s in ("_rule-both", "_rule-shadow"):
        d = os.path.join(base, prec + s)
        if os.path.isdir(d):
            return d
    return os.path.join(base, prec + "_rule-both")


# ---------------------------------------------------------------- scoring

def _sets(ev, rule, E):
    R = RULES[rule]
    if not ev:
        return set(), set()
    flagged_all = set().union(*[ev.get(t, set()) for t in R["flag"]])
    decl_all = ev.get(R["abstain"], set()) if R["abstain"] else set()
    return {k for k in flagged_all if k <= E}, {k for k in decl_all if k <= E}


def check_path_equivalence(sites, totals, coverage, overflow):
    """Per-site execution counts, tool vs oracle. Strict only at 100%
    coverage. Returns the set of locations to exclude from event scoring."""
    if overflow:
        return set()
    strict = coverage is not None and coverage >= 99.999
    diffs = set()
    for key, (E, _nflip, loc) in sites.items():
        T = totals.get(key)
        if T is None:
            continue
        if T < E or (strict and T != E):
            diffs.add(loc)
    return diffs


def score(sites, flips, events, rule, declined_mode, skip):
    TP = FP = FN = TN = 0
    for key, (E, _nflip, loc) in sites.items():
        if loc in skip:
            continue
        flipped = {o for o in flips.get(key, ()) if o <= E}
        flagged, declined = _sets(events.get(key), rule, E)
        if declined_mode == "exclude":
            judged = E - len(declined)
            tp = len(flipped & flagged)
            fp = len(flagged - flipped)
            fn = len(flipped - flagged - declined)
            tn = judged - tp - fp - fn
        else:
            tp = len(flipped & flagged)
            fp = len(flagged - flipped)
            fn = len(flipped - flagged)
            tn = E - tp - fp - fn
        TP += tp
        FP += fp
        FN += fn
        TN += max(0, tn)
    return dict(TP=TP, FP=FP, FN=FN, TN=TN)


def metrics(TP, FP, FN, TN):
    p = TP / (TP + FP) if TP + FP else None
    r = TP / (TP + FN) if TP + FN else None
    f1 = (2 * p * r / (p + r)) if (p and r) else (
        0.0 if p is not None and r is not None else None)
    n = TP + FP + FN + TN
    return p, r, f1, (TP + TN) / n if n else None


def eta_key(e):
    try:
        return -float(e)
    except ValueError:
        return 0.0


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-f", "--fpchecker",
                    default=os.environ.get(
                        "FPC_RESULTS",
                        os.path.join(HERE, "..", "fpchecker_experiments")))
    ap.add_argument("-g", "--gt", default=os.environ.get("GT_RESULTS", HERE))
    ap.add_argument("-b", "--benchmarks", nargs="+")
    ap.add_argument("--eta")
    ap.add_argument("--rule", choices=("interval", "shadow", "both"),
                    default="interval")
    ap.add_argument("--dir-suffix", default=None,
                    help="results directory suffix, e.g. _rule-both")
    ap.add_argument("--declined", choices=("exclude", "silence"),
                    default="exclude")
    ap.add_argument("--text")
    ap.add_argument("--latex")
    ap.add_argument("--json")
    args = ap.parse_args()

    rules = ["interval", "shadow"] if args.rule == "both" else [args.rule]
    dir_rule = "shadow" if args.rule == "both" else args.rule

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
            return 1
        cells = [c for c in cells if c[0].upper() in want]

    rows = defaultdict(lambda: defaultdict(dict))   # [(prec,bench)][eta][rule]
    etas_seen = defaultdict(set)
    order = []

    for bench, cen in cells:
        for prec, pair in (("fp32", "fp32_vs_fp64"), ("fp64", "fp64_vs_ld")):
            base_gt = os.path.join(args.gt, cen, pair)
            sites_p = os.path.join(base_gt, "sites.txt")
            window_p = os.path.join(base_gt, "window.csv")
            report_p = os.path.join(base_gt, "report.txt")
            flips_p = os.path.join(base_gt, "flips.csv")
            base_fp = fp_dir(args.fpchecker, cen, prec, dir_rule,
                             args.dir_suffix)
            sj = os.path.join(base_fp, "summary.json")
            if not (os.path.isfile(sites_p) or os.path.isfile(window_p)):
                continue
            if not os.path.isfile(sj):
                continue

            win = read_window(window_p)
            if win:
                sites = sites_from_window(win)
            else:
                sites = read_sites(sites_p) if os.path.isfile(sites_p) else {}
            coverage = read_coverage(report_p)
            flips = read_flips(flips_p)

            if read_tool_status(sj):
                for rule in rules:
                    rows[(prec, bench)]["--"][rule] = dict(refused=True)
                etas_seen[prec].add("--")
                if (prec, bench) not in order:
                    order.append((prec, bench))
                continue
            if not sites and not win:
                for rule in rules:
                    rows[(prec, bench)]["--"][rule] = dict(vacuous=True)
                etas_seen[prec].add("--")
                if (prec, bench) not in order:
                    order.append((prec, bench))
                continue
            if not sites:
                continue

            with open(sj) as fh:
                recs = json.load(fh)
            if isinstance(recs, dict):
                recs = [recs]

            for r in recs:
                eta = str(r.get("eta", "na"))
                if args.eta and eta != args.eta:
                    continue
                mode = r.get("bf_mode", "interval")
                have = {"interval", "shadow"} if mode == "both" else {mode}
                if not set(rules) <= have:
                    continue
                evs = {}
                for rule in rules:
                    p = resolve_event_log(r, rule, base_fp, eta)
                    if p is None:
                        break
                    evs[rule] = (p, read_events(p))
                if len(evs) != len(rules):
                    continue

                totals, overflow = read_site_totals(evs[rules[0]][0])
                skip = check_path_equivalence(sites, totals, coverage, overflow)
                skip |= {v[2] for k, v in win.items() if v[3] > 1}
                for rule in rules:
                    rows[(prec, bench)][eta][rule] = score(
                        sites, flips, evs[rule][1], rule, args.declined, skip)
                etas_seen[prec].add(eta)
                if (prec, bench) not in order:
                    order.append((prec, bench))

    if not rows:
        sys.exit("no cells scored -- check -f and -g, and that the event "
                 "logs exist")

    def cell(prec, b, e, rule):
        return rows[(prec, b)].get(e, {}).get(rule)

    ROW = "%-12s%-9s%9s%9s%8s%12s%9s%9s%9s%9s"
    RULE_LINE = "-" * 91

    def mfmt(x):
        return "--" if x is None else "%.4f" % x

    L = []
    for prec in sorted({p for p, _ in rows}):
        es = sorted(etas_seen[prec], key=eta_key)
        bs = [b for p, b in order if p == prec]
        bs.sort(key=lambda b: BENCH_ORDER.index(b) if b in BENCH_ORDER else 99)
        for rule in rules:
            L.append("=" * 91)
            L.append("FPChecker -- %s -- %s" % (prec, RULES[rule]["label"]))
            L.append("=" * 91)
            L.append(ROW % ("benchmark", "eta", "TP", "FP", "FN", "TN",
                            "P", "R", "F1", "Acc"))
            L.append(RULE_LINE)
            for b in bs:
                for e in es:
                    r = cell(prec, b, e, rule)
                    if r is None:
                        continue
                    if r.get("vacuous"):
                        L.append(ROW % (b, "--", 0, 0, 0, 0, "--", "--", "--", "--"))
                        continue
                    if r.get("refused"):
                        L.append(ROW % (b, "--", "--", "--", "--", "--",
                                        "--", "--", "--", "--"))
                        continue
                    p, rc, f1, acc = metrics(r["TP"], r["FP"], r["FN"], r["TN"])
                    L.append(ROW % (b, e, r["TP"], r["FP"], r["FN"], r["TN"],
                                    mfmt(p), mfmt(rc), mfmt(f1), mfmt(acc)))
            L.append(RULE_LINE)
            L.append("")

    text = "\n".join(L)
    print(text)
    if args.text:
        open(args.text, "w").write(text + "\n")
        print("wrote %s" % args.text)

    if args.json:
        J = {"tool": "fpchecker", "declined": args.declined, "cells": []}
        for (prec, b), byeta in sorted(rows.items()):
            for e, byrule in sorted(byeta.items(), key=lambda kv: eta_key(kv[0])):
                for rule, r in byrule.items():
                    c = {"benchmark": b, "precision": prec, "rule": rule,
                         "eta": None if e == "--" else e}
                    if r.get("vacuous"):
                        c["vacuous"] = True
                    elif r.get("refused"):
                        c["refused"] = True
                    else:
                        p, rc, f1, acc = metrics(r["TP"], r["FP"], r["FN"], r["TN"])
                        c.update({"TP": r["TP"], "FP": r["FP"], "FN": r["FN"],
                                  "TN": r["TN"], "P": p, "R": rc, "F1": f1,
                                  "Acc": acc})
                    J["cells"].append(c)
        with open(args.json, "w") as fh:
            json.dump(J, fh, indent=2)
        print("wrote %s" % args.json)

    if args.latex:
        T = []
        for prec in sorted({p for p, _ in rows}):
            es = sorted(etas_seen[prec], key=eta_key)
            bs = [b for p, b in order if p == prec]
            bs.sort(key=lambda b: BENCH_ORDER.index(b) if b in BENCH_ORDER else 99)
            for rule in rules:
                T += ["\\begin{tabular}{llrrrrrrrr}", "\\toprule",
                      "Benchmark & $\\eta$ & TP & FP & FN & TN & P & R & F1 & "
                      "Acc \\\\", "\\midrule"]
                for b in bs:
                    for e in es:
                        r = cell(prec, b, e, rule)
                        if r is None or r.get("vacuous") or r.get("refused"):
                            continue
                        p, rc, f1, acc = metrics(r["TP"], r["FP"], r["FN"], r["TN"])
                        T.append("%s & %s & %d & %d & %d & %d & %s & %s & %s & "
                                 "%s \\\\" % (
                                     b, e, r["TP"], r["FP"], r["FN"], r["TN"],
                                     *[("\\textemdash" if x is None
                                        else "%.4f" % x)
                                       for x in (p, rc, f1, acc)]))
                T += ["\\bottomrule", "\\end{tabular}", ""]
        open(args.latex, "w").write("\n".join(T) + "\n")
        print("wrote %s" % args.latex)
    return 0


if __name__ == "__main__":
    sys.exit(main())
