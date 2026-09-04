#!/usr/bin/env python3
"""eftsan_exact_metrics.py -- score EFTSanitizer against brtrace by matching
individual branch executions on the occurrence index both sides emit.

    ./eftsan_exact_metrics.py                       # everything it can find
    ./eftsan_exact_metrics.py -b LULESH AMG
    ./eftsan_exact_metrics.py --nonfinite exclude
    ./eftsan_exact_metrics.py --site-level          # ignore k entirely
    ./eftsan_exact_metrics.py --json out.json --latex out.tex --text out.txt

Counterpart of fpc_exact_metrics.py / nsan_exact_metrics.py. Differences are
properties of the tool: EFTSan walks one llvm-linked module, so sites are
joined on source location (file, line, column order) rather than site_id;
EFTSan is built with Clang 10, so per-site execution counts are compared
against brtrace's first and a cell whose counts drift widely is scored at
site level instead of event level; EFTSan has no abstention -- FLIP and
FLIP_NONFINITE are both detections (--nonfinite exclude drops non-finite
executions from both detections and denominator as a scoring choice).

Inputs:
    <gt>/<bench>/results/O0/<pair>/{window.csv|sites.txt,flips.csv,report.txt}
    <eft>/<bench>/results/O0/<prec>/{eftsan_sites.csv,eftsan_events.csv,
                                     eftsan_totals.csv,summary.json}
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

COVERAGE_RE = re.compile(r"lock-step compared\s+([\d,]+)\s+events\s+"
                         r"\(([\d.]+)% of longer trace\)")


# ---------------------------------------------------------------- oracle side

def parse_location(s):
    """'lulesh.cc:2079:12' -> ('lulesh.cc', 2079, 12); no column -> None."""
    s = s.strip().split()[0] if s.strip() else ""
    parts = s.rsplit(":", 2)
    if len(parts) == 3 and parts[1].isdigit() and parts[2].isdigit():
        return os.path.basename(parts[0]), int(parts[1]), int(parts[2])
    parts = s.rsplit(":", 1)
    if len(parts) == 2 and parts[1].isdigit():
        return os.path.basename(parts[0]), int(parts[1]), None
    return os.path.basename(s), None, None


def read_brtrace_sites(path):
    """sites.txt -> {(mod, site): (executions, flips, location)}"""
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


def read_window(path, kind="branch"):
    """window.csv -> {(mod, site): (E_S, flips, location, n_fcmp)}"""
    out = {}
    if not os.path.isfile(path):
        return out
    with open(path) as fh:
        for r in csv.DictReader(fh):
            if r.get("kind") != kind:
                continue
            out[(int(r["module_id"]), int(r["site_id"]))] = (
                int(r["E_S"]), int(r["flips"]), r["location"],
                int(r.get("n_fcmp", 1)))
    return out


def read_coverage(path):
    if not os.path.isfile(path):
        return None
    m = COVERAGE_RE.search(open(path, errors="replace").read())
    return float(m.group(2)) if m else None


def read_brtrace_flips(path, kind="branch"):
    """flips.csv -> {(mod, site): set(occ_index)}"""
    out = defaultdict(set)
    if not os.path.isfile(path):
        return out
    with open(path) as fh:
        for row in csv.DictReader(fh):
            if row.get("kind") not in (None, "", kind):
                continue
            try:
                out[(int(row["module_id"]), int(row["site_id"]))].add(
                    int(row["occ_index"]))
            except (KeyError, ValueError):
                continue
    return out


# ---------------------------------------------------------------- eftsan side

def read_eftsan_sites(path, kind="branch"):
    """eftsan_sites.csv -> {(mod, site): {file, line, col, function, n_fcmp}}"""
    out = {}
    with open(path) as fh:
        lines = [l.rstrip("\n") for l in fh if l.strip()]
    if not lines:
        return out
    hdr = lines[0].lstrip("# ").split(",")
    for l in lines[1:]:
        if l.startswith("#"):
            continue
        rec = dict(zip(hdr, l.split(",")))
        if rec.get("kind") != kind:
            continue
        out[(int(rec["module_id"]), int(rec["site_id"]))] = {
            "file": os.path.basename(rec["file"]),
            "line": int(rec["line"]),
            "col": int(rec["col"]),
            "function": rec["function"],
            "n_fcmp": int(rec["n_fcmp"]),
        }
    return out


def read_eftsan_events(path, kind="branch"):
    """eftsan_events.csv -> {(mod, site): {verdict: set(k)}}"""
    out = defaultdict(lambda: defaultdict(set))
    if not os.path.isfile(path):
        return None
    with open(path) as fh:
        for rec in csv.DictReader(fh):
            if rec.get("kind") != kind:
                continue
            out[(int(rec["module_id"]), int(rec["site_id"]))][
                rec["verdict"]].add(int(rec["k"]))
    return out


def read_eftsan_totals(path, kind="branch"):
    """eftsan_totals.csv -> {(mod, site): executions}"""
    out = {}
    if not os.path.isfile(path):
        return out
    with open(path) as fh:
        for rec in csv.DictReader(fh):
            if rec.get("kind") != kind:
                continue
            out[(int(rec["module_id"]), int(rec["site_id"]))] = int(
                rec["executions"])
    return out


def read_tool_status(path):
    """summary.json -> True if the harness recorded a tool failure."""
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


# ---------------------------------------------------------------- the join

def build_sitemap(brt, eft, nfcmp=None):
    """Match brtrace sites to EFTSan sites by source location.

    Tier 1: (file, line) exact, 1:1. Tier 2: (file, line) exact, n:n, paired
    by column order. Tier 3: a brtrace site with n_fcmp = k mapped to exactly
    k unmatched EFTSan sites within k lines above it. Returns (pairs, diag);
    pairs is [(brt_key, [eft_keys], (file, line))]."""
    brt_by_line = defaultdict(list)
    multi = []
    unparsed = []
    for k, (_E, _f, loc) in brt.items():
        f, ln, col = parse_location(loc)
        if ln is None:
            unparsed.append(loc)
            continue
        if nfcmp and nfcmp.get(k, 1) > 1:
            multi.append(((f, ln), [(k, col)]))
        else:
            brt_by_line[(f, ln)].append((k, col))

    eft_by_line = defaultdict(list)
    for k, si in eft.items():
        eft_by_line[(si["file"], si["line"])].append((k, si["col"]))

    pairs = []
    used_eft = set()
    diag = {"ambiguous": [], "only_brtrace": [], "only_eftsan": [],
            "multi_fcmp": [], "unparsed": unparsed}

    def record(bk, eks, loc):
        for ek in eks:
            used_eft.add(ek)
        nf = nfcmp.get(bk, 1) if nfcmp else 1
        if len(eks) > 1 or nf > 1 or eft[eks[0]]["n_fcmp"] > 1:
            diag["multi_fcmp"].append((loc, max(len(eks), nf)))
        pairs.append((bk, eks, loc))

    leftover_brt = []
    for loc, bs in sorted(brt_by_line.items()):
        es = eft_by_line.get(loc, [])
        if not es:
            leftover_brt.append((loc, bs))
            continue
        if len(bs) == 1 and len(es) == 1:
            matched = [(bs[0], [es[0]])]
        else:
            if len(bs) != len(es) or any(c is None for _k, c in bs) \
                    or any(c is None for _k, c in es):
                diag["ambiguous"].append((loc, len(bs), len(es)))
                continue
            matched = [(b, [e]) for b, e in
                       zip(sorted(bs, key=lambda x: x[1]),
                           sorted(es, key=lambda x: x[1]))]
        for (bk, _bcol), eks in matched:
            record(bk, [ek for ek, _c in eks], loc)

    for loc, bs in leftover_brt + multi:
        placed = False
        if len(bs) == 1 and nfcmp:
            bk = bs[0][0]
            k = nfcmp.get(bk, 1)
            if k > 1:
                f, ln = loc
                cand = []
                for off in range(k):
                    for ek, _c in eft_by_line.get((f, ln - off), []):
                        if ek not in used_eft:
                            cand.append(ek)
                if len(cand) == k:
                    record(bk, cand, loc)
                    placed = True
        if not placed:
            diag["only_brtrace"].append((loc, len(bs)))

    for loc, es in eft_by_line.items():
        rem = [ek for ek, _c in es if ek not in used_eft]
        if rem:
            diag["only_eftsan"].append((loc, len(rem)))

    return pairs, diag


def check_path_equivalence(pairs, brt, eft_totals, coverage):
    """Per-site execution counts, tool vs oracle. Strict only at 100%
    coverage (E_S is windowed). A disagreeing site is excluded; widespread
    disagreement fails the cell to site level."""
    strict = coverage is not None and coverage >= 99.999
    checked = 0
    diffs = []
    for bk, eks, loc in pairs:
        E = brt[bk][0]
        if not any(ek in eft_totals for ek in eks):
            continue
        T = sum(eft_totals[ek] for ek in eks if ek in eft_totals)
        checked += 1
        if T < E or (strict and T != E):
            diffs.append((loc, E, T))
    exclude = {loc for loc, _E, _T in diffs}
    widespread = checked and (len(diffs) > max(3, 0.05 * checked))
    return {"checked": checked, "n_differ": len(diffs), "exclude": exclude,
            "strict": strict, "coverage": coverage, "failed": bool(widespread)}


# ---------------------------------------------------------------- scoring

def score(pairs, brt, flips, events, nonfinite_mode, occ_base, skip):
    """Exact per-execution counts over the adjudicated region."""
    TP = FP = FN = TN = 0
    for bk, eks, loc in pairs:
        if loc in skip:
            continue
        E, _nflip, _locstr = brt[bk]
        if E == 0:
            continue
        in_win = (lambda k: k <= E) if occ_base == 1 else (lambda k: k < E)

        flipped = {o for o in flips.get(bk, ()) if in_win(o)}
        flagged_all, nf_all = set(), set()
        for ek in eks:
            ev = events.get(ek, {})
            flagged_all |= ev.get("FLIP", set()) | ev.get("FLIP_NONFINITE", set())
            nf_all |= ev.get("FLIP_NONFINITE", set()) | ev.get("NONFINITE", set())
        flagged = {k for k in flagged_all if in_win(k)}
        nonfin = {k for k in nf_all if in_win(k)}

        if nonfinite_mode == "exclude":
            flagged = flagged - nonfin
            judged = E - len(nonfin)
            tp = len(flipped & flagged)
            fp = len(flagged - flipped)
            fn = len(flipped - flagged - nonfin)
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


def score_site_level(pairs, brt, flips, events, skip):
    """Site membership only: did the site flip, did the tool report it."""
    TP = FP = FN = TN = 0
    for bk, eks, loc in pairs:
        if loc in skip:
            continue
        if brt[bk][0] == 0:
            continue
        did_flip = bool(flips.get(bk))
        reported = any(events.get(ek, {}).get("FLIP")
                       or events.get(ek, {}).get("FLIP_NONFINITE")
                       for ek in eks)
        if did_flip and reported:
            TP += 1
        elif reported:
            FP += 1
        elif did_flip:
            FN += 1
        else:
            TN += 1
    return dict(TP=TP, FP=FP, FN=FN, TN=TN)


def metrics(TP, FP, FN, TN):
    p = TP / (TP + FP) if TP + FP else None
    r = TP / (TP + FN) if TP + FN else None
    f1 = (2 * p * r / (p + r)) if (p and r) else (
        0.0 if p is not None and r is not None else None)
    n = TP + FP + FN + TN
    return p, r, f1, (TP + TN) / n if n else None


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-e", "--eftsan",
                    default=os.environ.get(
                        "EFTSAN_RESULTS",
                        os.path.join(HERE, "..", "eftsan_experiments")))
    ap.add_argument("-g", "--gt", default=os.environ.get("GT_RESULTS", HERE))
    ap.add_argument("-b", "--benchmarks", nargs="+")
    ap.add_argument("--kind", default="branch", choices=("branch", "select"))
    ap.add_argument("--nonfinite", choices=("include", "exclude"),
                    default="include")
    ap.add_argument("--occ-base", type=int, choices=(0, 1), default=1)
    ap.add_argument("--site-level", action="store_true",
                    help="ignore k; score site membership only")
    ap.add_argument("--text")
    ap.add_argument("--latex")
    ap.add_argument("--json")
    args = ap.parse_args()

    cells = [("LULESH", "lulesh", "lulesh/results/O0"),
             ("AMG", "amg", "amg/results/O0"),
             ("QuickSilver", "quicksilver", "quicksilver/results/O0")]
    for b in ("bt", "cg", "ep", "is", "lu", "mg", "sp"):
        cells.append((b.upper(), "nas/" + b, "nas/results/%s/O0" % b))

    if args.benchmarks:
        want = {x.upper() for x in args.benchmarks}
        unknown = want - {c[0].upper() for c in cells}
        if unknown:
            print("unknown benchmark(s): %s" % ", ".join(sorted(unknown)))
            print("known: %s" % ", ".join(c[0] for c in cells))
            return 1
        cells = [c for c in cells if c[0].upper() in want]

    rows = {}
    order = []

    for bench, sub, cen in cells:
        for prec, pair in (("fp32", "fp32_vs_fp64"), ("fp64", "fp64_vs_ld")):
            base_gt = os.path.join(args.gt, cen, pair)
            if args.kind == "select":
                base_gt = os.path.join(base_gt, "select")
            sites_p = os.path.join(base_gt, "sites.txt")
            window_p = os.path.join(base_gt, "window.csv")
            report_p = os.path.join(base_gt, "report.txt")
            flips_p = os.path.join(base_gt, "flips.csv")
            base_ef = os.path.join(args.eftsan, sub, "results/O0", prec)
            esites_p = os.path.join(base_ef, "eftsan_sites.csv")
            eevents_p = os.path.join(base_ef, "eftsan_events.csv")
            etotals_p = os.path.join(base_ef, "eftsan_totals.csv")

            if not (os.path.isfile(window_p) or os.path.isfile(sites_p)):
                continue
            if not os.path.isfile(esites_p):
                if read_tool_status(os.path.join(base_ef, "summary.json")):
                    rows[(prec, bench)] = dict(refused=True)
                    if (prec, bench) not in order:
                        order.append((prec, bench))
                continue

            win = read_window(window_p, args.kind)
            nfcmp = {k: v[3] for k, v in win.items()}
            if win:
                brt = {k: (v[0], v[1], v[2]) for k, v in win.items()}
            elif os.path.isfile(sites_p):
                brt = read_brtrace_sites(sites_p)
            else:
                brt = {}
            coverage = read_coverage(report_p)
            eft = read_eftsan_sites(esites_p, args.kind)

            if not brt and not eft:
                rows[(prec, bench)] = dict(vacuous=True)
                if (prec, bench) not in order:
                    order.append((prec, bench))
                continue
            if not brt or not eft:
                continue

            events = read_eftsan_events(eevents_p, args.kind)
            if events is None:
                if read_tool_status(os.path.join(base_ef, "summary.json")):
                    rows[(prec, bench)] = dict(refused=True)
                    if (prec, bench) not in order:
                        order.append((prec, bench))
                continue

            flips = read_brtrace_flips(flips_p, args.kind)
            totals = read_eftsan_totals(etotals_p, args.kind)

            pairs, diag = build_sitemap(brt, eft, nfcmp)
            if not pairs:
                continue

            pe = check_path_equivalence(pairs, brt, totals, coverage)
            site_only = args.site_level or pe["failed"]
            skip = {loc for loc, _n in diag["multi_fcmp"]} | pe["exclude"]
            if site_only:
                res = score_site_level(pairs, brt, flips, events, skip)
            else:
                res = score(pairs, brt, flips, events, args.nonfinite,
                            args.occ_base, skip)
            res["level"] = "site" if site_only else "event"
            rows[(prec, bench)] = res
            if (prec, bench) not in order:
                order.append((prec, bench))

    if not rows:
        sys.exit("no cells scored -- check -e and -g, and that "
                 "eftsan_events.csv exists")

    ROW = "%-12s%10s%10s%9s%12s%9s%9s%9s%9s"
    RULE = "-" * 89

    def mfmt(x):
        return "--" if x is None else "%.4f" % x

    L = []
    for prec in sorted({p for p, _ in rows}):
        bs = [b for p, b in order if p == prec]
        bs.sort(key=lambda b: BENCH_ORDER.index(b)
                if b in BENCH_ORDER else 99)
        L.append("=" * 89)
        L.append("EFTSanitizer -- %s   (%s)" % (prec, args.kind))
        L.append("=" * 89)
        L.append(ROW % ("benchmark", "TP", "FP", "FN", "TN", "P", "R", "F1",
                        "Acc"))
        L.append(RULE)
        for b in bs:
            r = rows[(prec, b)]
            if r.get("vacuous"):
                L.append(ROW % (b, 0, 0, 0, 0, "--", "--", "--", "--"))
                continue
            if r.get("refused"):
                L.append(ROW % (b, "--", "--", "--", "--", "--", "--", "--",
                                "--"))
                continue
            p_, rc, f1, acc = metrics(r["TP"], r["FP"], r["FN"], r["TN"])
            L.append(ROW % (b, r["TP"], r["FP"], r["FN"], r["TN"],
                            mfmt(p_), mfmt(rc), mfmt(f1), mfmt(acc)))
        L.append(RULE)
        L.append("")

    text = "\n".join(L)
    print(text)
    if args.text:
        open(args.text, "w").write(text + "\n")
        print("wrote %s" % args.text)

    if args.json:
        J = {"tool": "eftsan", "cells": []}
        for (prec, b), r in sorted(rows.items()):
            cell = {"benchmark": b, "precision": prec}
            if r.get("vacuous"):
                cell["vacuous"] = True
            elif r.get("refused"):
                cell["refused"] = True
            else:
                p, rc, f1, acc = metrics(r["TP"], r["FP"], r["FN"], r["TN"])
                cell.update({"level": r["level"],
                             "TP": r["TP"], "FP": r["FP"],
                             "FN": r["FN"], "TN": r["TN"],
                             "P": p, "R": rc, "F1": f1, "Acc": acc})
            J["cells"].append(cell)
        with open(args.json, "w") as fh:
            json.dump(J, fh, indent=2)
        print("wrote %s" % args.json)

    if args.latex:
        T = []
        for prec in sorted({p for p, _ in rows}):
            bs = [b for p, b in order if p == prec]
            bs.sort(key=lambda b: BENCH_ORDER.index(b)
                    if b in BENCH_ORDER else 99)
            T += ["\\begin{tabular}{lrrrrrrrr}", "\\toprule",
                  "Benchmark & TP & FP & FN & TN & P & R & F1 & Acc \\\\",
                  "\\midrule"]
            for b in bs:
                r = rows[(prec, b)]
                if r.get("vacuous") or r.get("refused"):
                    continue
                p, rc, f1, acc = metrics(r["TP"], r["FP"], r["FN"], r["TN"])
                T.append("%s & %d & %d & %d & %d & %s & %s & %s & %s \\\\" % (
                    b, r["TP"], r["FP"], r["FN"], r["TN"],
                    *[("\\textemdash" if x is None else "%.4f" % x)
                      for x in (p, rc, f1, acc)]))
            T += ["\\bottomrule", "\\end{tabular}", ""]
        open(args.latex, "w").write("\n".join(T) + "\n")
        print("wrote %s" % args.latex)
    return 0


if __name__ == "__main__":
    sys.exit(main())