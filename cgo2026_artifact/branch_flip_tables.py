#!/usr/bin/env python3
"""branch_flip_tables.py -- build the branch-flip tables from the scorer JSONs
and check them against the expected results.

    ./branch_flip_tables.py --main                  # paper Table 4 (FP32, EFTSan + FPChecker eta=1e-6)
    ./branch_flip_tables.py --full                  # appendix Tables 11/12 (FP32 + FP64, all tools)
    ./branch_flip_tables.py --main --full --pdf
    ./branch_flip_tables.py --score --main          # run the three scorers first

Inputs (--results, default ./results): fpc_metrics.json, eftsan_metrics.json,
nsan_metrics.json as written by *_exact_metrics.py --json. Expected values
(--expected, default <repo>/cpu_checking/error_analysis/branch_flip/expected)
have the same layout.

Outputs in --out (default --results): table_main.{txt,tex,json},
table_full.{txt,tex,json}, compare.txt, and table_*.pdf with --pdf.
Exit status 1 if any cell outside the tolerance set differs from expected.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
FPC_SRC = os.environ.get("FPC_SRC", os.path.abspath(os.path.join(HERE, "..")))
BF = os.path.join(FPC_SRC, "cpu_checking", "error_analysis", "branch_flip")

BENCH_ORDER = ["BT", "CG", "EP", "LU", "MG", "SP", "IS", "LULESH", "QuickSilver", "AMG"]
LABEL = {b: ("NAS " + b if len(b) == 2 else b) for b in BENCH_ORDER}
ETAS = {"fp32": ["1e-2", "1e-6", "1e-10"], "fp64": ["1e-8", "1e-14", "1e-16"]}
MAIN_ETA = {"fp32": "1e-6", "fp64": "1e-14"}
COUNTS = ["TP", "FP", "TN", "FN"]
METRICS = ["P", "R", "F1", "Acc"]
TOLERATED = {"LULESH"}


# ---------------------------------------------------------------- loading

def load(path):
    if not os.path.isfile(path):
        return {}
    d = json.load(open(path))
    return {cell_key(c): c for c in d.get("cells", [])}


def cell_key(c):
    return (c["benchmark"], c["precision"], c.get("rule", ""),
            "" if c.get("eta") in (None, "--") else str(c.get("eta")),
            c.get("policy", "") if c.get("policy", "") not in ("measured",) else "")


def get(cells, bench, prec, rule="", eta=""):
    c = cells.get((bench, prec, rule, eta, ""))
    if c is None and eta:
        # vacuous / refused cells carry no eta
        c = cells.get((bench, prec, rule, "", ""))
        if c is not None and not (c.get("vacuous") or c.get("refused")):
            c = None
    return c


def load_all(d):
    return {"eftsan": load(os.path.join(d, "eftsan_metrics.json")),
            "nsan": load(os.path.join(d, "nsan_metrics.json")),
            "fpc": load(os.path.join(d, "fpc_metrics.json"))}


# ---------------------------------------------------------------- rows

def crashed(c):
    return c is not None and c.get("refused")


def vacuous(c):
    return c is not None and c.get("vacuous")


def row_values(c):
    """[TP, FP, TN, FN, P, R, F1, Acc] or None when absent/crashed."""
    if c is None or crashed(c):
        return None
    if vacuous(c):
        return [0, 0, 0, 0, None, None, None, None]
    return [c.get(k) for k in COUNTS] + [c.get(k) for k in METRICS]


def build_main(res, prec):
    """Table 4 layout: one EFTSan row and one FPChecker (interval, main eta)
    row per benchmark."""
    rows = []
    for b in BENCH_ORDER:
        rows.append({"benchmark": b, "tool": "EFTSan", "rule": "", "eta": "",
                     "cell": get(res["eftsan"], b, prec)})
        rows.append({"benchmark": b, "tool": "FPChecker", "rule": "interval",
                     "eta": MAIN_ETA[prec],
                     "cell": get(res["fpc"], b, prec, "interval", MAIN_ETA[prec])})
    return rows


def build_full(res, prec):
    """Tables 11/12 layout: per benchmark, EFTSan and NSan once, FPChecker
    per rule x eta."""
    rows = []
    for b in BENCH_ORDER:
        rows.append({"benchmark": b, "tool": "EFTSan", "rule": "", "eta": "",
                     "cell": get(res["eftsan"], b, prec)})
        rows.append({"benchmark": b, "tool": "NSan", "rule": "", "eta": "",
                     "cell": get(res["nsan"], b, prec)})
        for rule in ("interval", "shadow"):
            for eta in ETAS[prec]:
                rows.append({"benchmark": b, "tool": "FPChecker", "rule": rule,
                             "eta": eta,
                             "cell": get(res["fpc"], b, prec, rule, eta)})
    return rows


# ---------------------------------------------------------------- comparison

def tool_of(row):
    return {"EFTSan": "eftsan", "NSan": "nsan", "FPChecker": "fpc"}[row["tool"]]


def compare(rows, exp, prec):
    """Attach status to each row: ok / delta / DIFF / missing / n/a."""
    fails = 0
    for r in rows:
        e = get(exp[tool_of(r)], r["benchmark"], prec, r["rule"], r["eta"])
        c = r["cell"]
        if e is None:
            r["status"] = "n/a"
            continue
        if c is None:
            r["status"] = "missing"
            fails += 1
            continue
        if crashed(e) or vacuous(e) or crashed(c) or vacuous(c):
            same = (crashed(e) == crashed(c) and vacuous(e) == vacuous(c))
            r["status"] = "ok" if same else "DIFF"
            fails += 0 if same else 1
            continue
        same = all(e.get(k) == c.get(k) for k in COUNTS)
        if same:
            r["status"] = "ok"
        elif r["benchmark"] in TOLERATED:
            r["status"] = "delta"
        else:
            r["status"] = "DIFF"
            fails += 1
        if not same:
            r["expected"] = [e.get(k) for k in COUNTS]
    return fails


# ---------------------------------------------------------------- rendering

def fnum(x):
    if x is None:
        return "–"
    return f"{x:,}" if isinstance(x, int) else f"{x:.4f}"


def render_text(rows, title, prec):
    hdr = f"{'Benchmark':<12}{'Tool':<11}{'Rule':<9}{'eta':<7}" \
          f"{'TP':>10}{'FP':>11}{'TN':>13}{'FN':>9}" \
          f"{'P':>8}{'R':>8}{'F1':>8}{'Acc.':>8}  status"
    L = ["=" * len(hdr), f"{title}  ({prec})", "=" * len(hdr), hdr, "-" * len(hdr)]
    last = None
    for r in rows:
        b = LABEL[r["benchmark"]] if r["benchmark"] != last else ""
        last = r["benchmark"]
        v = row_values(r["cell"])
        st = r.get("status", "")
        if r["cell"] is None:
            body = f"{'(no result)':>10}"
        elif crashed(r["cell"]):
            body = f"{'Crashed':>10}"
        else:
            body = (f"{fnum(v[0]):>10}{fnum(v[1]):>11}{fnum(v[2]):>13}{fnum(v[3]):>9}"
                    f"{fnum(v[4]):>8}{fnum(v[5]):>8}{fnum(v[6]):>8}{fnum(v[7]):>8}")
        tail = f"  {st}"
        if r.get("expected"):
            tail += "  (expected " + " ".join(fnum(x) for x in r["expected"]) + ")"
        L.append(f"{b:<12}{r['tool']:<11}{r['rule']:<9}{r['eta']:<7}{body}{tail}")
    L.append("-" * len(hdr))
    return "\n".join(L)


def tex_num(x):
    return "--" if x is None else (f"{x:,}" if isinstance(x, int) else f"{x:.4f}")


def render_tex_main(rows, prec):
    T = ["\\begin{tabular}{llrrrrrrrrr}", "\\toprule",
         "Benchmark & Tool & $\\eta_{rel}$ & TP & FP & TN & FN & P & R & F1 & Acc. \\\\",
         "\\midrule"]
    last = None
    for r in rows:
        b = LABEL[r["benchmark"]] if r["benchmark"] != last else ""
        last = r["benchmark"]
        eta = r["eta"] or "--"
        v = row_values(r["cell"])
        if r["cell"] is None or crashed(r["cell"]):
            T.append(f"{b} & {r['tool']} & {eta} & \\multicolumn{{8}}{{c}}{{Crashed}} \\\\")
        else:
            T.append(f"{b} & {r['tool']} & {eta} & " +
                     " & ".join(tex_num(x) for x in v) + " \\\\")
    T += ["\\bottomrule", "\\end{tabular}"]
    return "\n".join(T)


def render_tex_full(rows, prec):
    """One block per benchmark: EFTSan and NSan on the first line, FPChecker
    rule x eta lines after."""
    T = ["\\begin{tabular}{l" + "rrrrrrrr" + "|" + "rrrrrrrr" + "|llrrrrrrrr}",
         "\\toprule",
         "& \\multicolumn{8}{c|}{EFTSan} & \\multicolumn{8}{c|}{NSan} & \\multicolumn{10}{c}{FPChecker} \\\\",
         "Benchmark & TP & FP & TN & FN & P & R & F1 & Acc. & TP & FP & TN & FN & P & R & F1 & Acc. "
         "& Rule & $\\eta_{rel}$ & TP & FP & TN & FN & P & R & F1 & Acc. \\\\",
         "\\midrule"]
    by_bench = {}
    for r in rows:
        by_bench.setdefault(r["benchmark"], []).append(r)
    for b in BENCH_ORDER:
        rs = by_bench.get(b, [])
        eft = next((x for x in rs if x["tool"] == "EFTSan"), None)
        ns = next((x for x in rs if x["tool"] == "NSan"), None)
        fpc = [x for x in rs if x["tool"] == "FPChecker"]

        def block(r):
            if r is None or r["cell"] is None or crashed(r["cell"]):
                return "\\multicolumn{8}{c|}{Crashed}"
            return " & ".join(tex_num(x) for x in row_values(r["cell"]))
        first = True
        for r in fpc:
            lead = (f"{LABEL[b]} & {block(eft)} & {block(ns)}" if first
                    else " & " + " & ".join([""] * 15))
            first = False
            v = row_values(r["cell"])
            body = (" & ".join(tex_num(x) for x in v) if v is not None
                    else "\\multicolumn{8}{c}{--}")
            T.append(f"{lead} & {r['rule'].capitalize()} & {r['eta']} & {body} \\\\")
        T.append("\\midrule")
    T[-1] = "\\bottomrule"
    T.append("\\end{tabular}")
    return "\n".join(T)


def write_pdf(tex_path):
    if not shutil.which("pdflatex"):
        print("  (pdflatex not found; PDF skipped)")
        return
    doc = ("\\documentclass{article}\\usepackage[landscape,margin=0.5in]{geometry}"
           "\\usepackage{booktabs}\\begin{document}\\tiny\n" +
           open(tex_path).read() + "\n\\end{document}\n")
    tmp = tex_path.replace(".tex", "_doc.tex")
    open(tmp, "w").write(doc)
    subprocess.run(["pdflatex", "-interaction=batchmode", "-output-directory",
                    os.path.dirname(tex_path) or ".", tmp],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for ext in (".aux", ".log", ".tex"):
        try:
            os.remove(tmp.replace(".tex", ext))
        except OSError:
            pass
    pdf = tmp.replace(".tex", ".pdf")
    if os.path.exists(pdf):
        os.replace(pdf, tex_path.replace(".tex", ".pdf"))
        print(f"  wrote {tex_path.replace('.tex', '.pdf')}")


def to_json(rows, prec):
    out = []
    for r in rows:
        v = row_values(r["cell"])
        d = {"benchmark": r["benchmark"], "precision": prec, "tool": r["tool"],
             "rule": r["rule"] or None, "eta": r["eta"] or None,
             "status": r.get("status")}
        if r["cell"] is None:
            d["missing"] = True
        elif crashed(r["cell"]):
            d["crashed"] = True
        else:
            d.update(dict(zip(COUNTS + METRICS, v)))
        if r.get("expected"):
            d["expected"] = dict(zip(COUNTS, r["expected"]))
        out.append(d)
    return out


# ---------------------------------------------------------------- scorers

def run_scorers(results, gt):
    for tool, script in (("fpc", "fpc_exact_metrics.py"),
                         ("eftsan", "eftsan_exact_metrics.py"),
                         ("nsan", "nsan_exact_metrics.py")):
        cmd = [sys.executable, os.path.join(gt, script),
               "--json", os.path.join(results, f"{tool}_metrics.json")]
        if tool == "fpc":
            cmd += ["--rule", "both"]
        print("$ " + " ".join(cmd))
        p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True)
        if p.returncode != 0:
            print(p.stdout[-2000:])
            print(f"  {script} failed (rc={p.returncode}); continuing")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--main", action="store_true", help="paper Table 4")
    ap.add_argument("--full", action="store_true", help="appendix Tables 11/12")
    ap.add_argument("--results", default=os.path.join(HERE, "results"))
    ap.add_argument("--expected", default=os.path.join(BF, "expected"))
    ap.add_argument("--out", default=None)
    ap.add_argument("--score", action="store_true",
                    help="run the three *_exact_metrics.py scorers first")
    ap.add_argument("--gt", default=os.path.join(BF, "experiments", "gt_experiments"),
                    help="gt_experiments dir (for --score)")
    ap.add_argument("--pdf", action="store_true")
    ap.add_argument("--no-compare", action="store_true")
    args = ap.parse_args()
    if not (args.main or args.full):
        args.main = True
    out = args.out or args.results
    os.makedirs(out, exist_ok=True)

    if args.score:
        run_scorers(args.results, args.gt)

    res = load_all(args.results)
    exp = load_all(args.expected) if not args.no_compare else \
        {"eftsan": {}, "nsan": {}, "fpc": {}}
    if not any(res.values()):
        sys.exit(f"no *_metrics.json under {args.results}")

    total_fail = 0
    compare_lines = []

    def emit(name, builder, precs, tex_renderer):
        nonlocal total_fail
        texts, texs, js = [], [], []
        for prec in precs:
            rows = builder(res, prec)
            fails = 0 if args.no_compare else compare(rows, exp, prec)
            total_fail += fails
            title = {"main": "Table 4 -- FP32 branch-flip detection",
                     "full": "Branch-flip detection accuracy across benchmarks"}[name]
            texts.append(render_text(rows, title, prec))
            texs.append(f"% {title} ({prec})\n" + tex_renderer(rows, prec))
            js += to_json(rows, prec)
            n = len([r for r in rows if r.get("status") not in (None, "n/a")])
            ok = len([r for r in rows if r.get("status") == "ok"])
            dl = len([r for r in rows if r.get("status") == "delta"])
            compare_lines.append(f"{name} {prec}: {ok}/{n} exact, {dl} within "
                                 f"tolerance, {fails} mismatched")
        text = "\n\n".join(texts)
        print(text + "\n")
        open(os.path.join(out, f"table_{name}.txt"), "w").write(text + "\n")
        open(os.path.join(out, f"table_{name}.tex"), "w").write("\n\n".join(texs) + "\n")
        json.dump({"table": name, "rows": js},
                  open(os.path.join(out, f"table_{name}.json"), "w"), indent=2)
        print(f"  wrote {out}/table_{name}.{{txt,tex,json}}")
        if args.pdf:
            write_pdf(os.path.join(out, f"table_{name}.tex"))

    if args.main:
        emit("main", build_main, ["fp32"], render_tex_main)
    if args.full:
        emit("full", build_full, ["fp32", "fp64"], render_tex_full)

    if not args.no_compare:
        summary = "\n".join(compare_lines) + "\nRESULT: " + \
            ("OK" if total_fail == 0 else f"{total_fail} MISMATCH(ES)")
        print("\n" + summary)
        open(os.path.join(out, "compare.txt"), "w").write(summary + "\n")
        return 1 if total_fail else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
