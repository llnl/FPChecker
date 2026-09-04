#!/usr/bin/env python3
"""
run_nas_brtrace.py -- brtrace census for the NAS benchmarks: build fp32, fp64
and long-double variants with the BranchTrace pass, run each, and diff the
trace pairs.

    ./run_nas_brtrace.py                       # all 7
    ./run_nas_brtrace.py -b lu sp
    ./run_nas_brtrace.py -b cg --timeout 7200

Precision selection differs per tree: BT/CG/LU/SP take -DNAS_FP32/-DNAS_LD,
EP takes -DWORKING_T=..., IS/MG bake it into the source. Each variant is one
direct clang invocation. Problem size is compiled in. Results go to
nas/results/<bench>/<opt>/<pair>/. See gt_common.py for outputs.
"""

import argparse
import os
import re
import shutil
import sys

from gt_common import (BENCH_BASE, PAIRS, WORK_BASE, add_common_args,
                       arith_profile, brx_cflags, check_precisions,
                       check_site_agreement, collect_side_tables, die,
                       ensure_brtrace, parse_banners, print_pair, run_diff,
                       run_traced, sh, write_summary, RUNTIME)

BENCHES = ["bt", "cg", "ep", "is", "lu", "mg", "sp"]
BENCH_ROOT = BENCH_BASE / "nas"
WORK_ROOT = WORK_BASE / "nas"
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

SRCS = {
    "bt": ["bt.c"], "cg": ["cg.c"], "lu": ["lu.c"], "mg": ["mg.c"],
    "sp": ["sp.c"],
    "ep": ["ep.c", "c_randdp.c", "c_print_results.c", "c_timers.c", "wtime.c"],
    "is": ["is.c", "c_print_results.c", "c_timers.c", "wtime.c"],
}
_SWITCH = {"fp32": "-DNAS_FP32", "fp64": "", "ld": "-DNAS_LD"}
_NONE = {"fp32": "", "fp64": "", "ld": ""}
DEFINES = {
    "bt": _SWITCH, "cg": _SWITCH, "lu": _SWITCH, "sp": _SWITCH,
    "is": _NONE, "mg": _NONE,
    "ep": {"fp32": "-DWORKING_T=float -DWORKING_T_IS_FLOAT=1",
           "fp64": "-DWORKING_T=double",
           "ld": "-DWORKING_T=long\\ double -DWORKING_T_IS_LD=1"},
}
VARIANT_DIR = {"fp32": "{b}_fp32", "fp64": "{b}_fp64", "ld": "{b}_ld"}
VERIFY_RE = re.compile(r"\b(SUCCESSFUL|UNSUCCESSFUL)\b", re.I)


def build(bench, variant, args, outdir):
    src = BENCH_ROOT / bench / VARIANT_DIR[variant].format(b=bench)
    dst = BUILD_ROOT / bench / args.opt / VARIANT_DIR[variant].format(b=bench)
    if not src.is_dir():
        die(f"no source tree at {src}")
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for pat in ("*.o", "*.br*"):
        for j in dst.glob(pat):
            j.unlink()
    for j in dst.iterdir():
        if j.is_file() and os.access(j, os.X_OK) and j.suffix == "":
            j.unlink()

    binname = f"{bench}.brx"
    cmd = re.sub(r"\s+", " ",
                 f"{args.cc} {brx_cflags(args.opt, not args.all_branches)} "
                 f"{args.ccflags} {DEFINES[bench][variant]} "
                 f"{' '.join(SRCS[bench])} {RUNTIME} -lm -o {binname}")
    log = outdir / "builds" / f"{variant}.build.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    rc, out = sh(cmd, cwd=dst, log=log)
    if rc != 0 or not (dst / binname).exists():
        for line in out.strip().splitlines()[-8:]:
            print(f"      {line[:200]}")
        die(f"build failed for {bench} {variant} -- see {log}")
    banners = parse_banners(out)
    total = sum(b[1] for b in banners.values())
    print(f"    {variant:<5s} {total:4d} branch sites, "
          f"{sum(b[2] for b in banners.values())} select sites")
    (outdir / "builds" / f"{variant}.build_info.txt").write_text(
        f"benchmark   = {bench}\nvariant     = {variant}\n"
        f"opt         = -{args.opt}\ncmd         = {cmd}\n"
        f"sites_total = {total}\n")
    return dst / binname, banners


def run_bench(bench, args):
    outdir = RESULT_ROOT / bench / args.opt
    (outdir / "traces").mkdir(parents=True, exist_ok=True)
    print(f"===== {bench.upper()} =====")

    binaries, banners, profiles = {}, {}, {}
    if not args.skip_build:
        for v in args.variants:
            binaries[v], banners[v] = build(bench, v, args, outdir)
            profiles[v] = arith_profile(binaries[v])
        if not check_site_agreement(banners, bench) \
                and not args.allow_site_mismatch:
            die(f"{bench}: site counts differ between variants")
        if not check_precisions(profiles, bench):
            die(f"{bench}: two variants were built at the same precision")
    else:
        for v in args.variants:
            binaries[v] = (BUILD_ROOT / bench / args.opt /
                           VARIANT_DIR[v].format(b=bench) / f"{bench}.brx")

    traces = {v: outdir / "traces" / f"{bench}_{v}.out" for v in args.variants}
    verify = {}
    if not args.skip_run:
        for v in args.variants:
            if not binaries[v].exists():
                die(f"{binaries[v]} not found -- drop --skip-build")
            rc, out = run_traced(binaries[v], [], traces[v],
                                 cwd=binaries[v].parent,
                                 log=outdir / "builds" / f"{v}.run.log",
                                 timeout=args.timeout)
            m = VERIFY_RE.search(out)
            verify[v] = m.group(1).upper() if m else "?"
            ok = rc in (0, 26) if bench == "is" else rc == 0
            if not ok:
                die(f"{bench} {v}: run failed (exit {rc})")
            print(f"    {v:<5s} {traces[v].stat().st_size // 12:,} events   "
                  f"verification={verify[v]}")

    rec = {"title": f"NAS {bench.upper()} brtrace census (-{args.opt})",
           "benchmark": bench, "opt": args.opt,
           "fp_only": not args.all_branches, "verification": verify,
           "sites_by_variant": {v: sum(b[1] for b in banners[v].values())
                                for v in banners},
           "pairs": {}}
    for label in args.pairs:
        a, b = PAIRS[label]
        if a not in args.variants or b not in args.variants:
            continue
        for t in (traces[a], traces[b]):
            if not t.exists():
                die(f"missing trace {t}")
        pdir = outdir / label
        pdir.mkdir(parents=True, exist_ok=True)
        mods = BUILD_ROOT / bench / args.opt / VARIANT_DIR[b].format(b=bench)
        collect_side_tables(mods, pdir / "sidetables")
        res = run_diff(traces[a], traces[b], mods, pdir, "branch")
        sel = None
        if (outdir / "traces" / f"{bench}_{a}_sel.out").exists():
            sel = run_diff(str(traces[a]).replace(".out", "_sel.out"),
                           str(traces[b]).replace(".out", "_sel.out"),
                           mods, pdir, "select")
        rec["pairs"][label] = {"a": a, "b": b, "branch": res, "select": sel}
        print_pair(label, res, sel)
    write_summary(outdir, rec)
    print()
    return rec


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-b", "--benchmarks", nargs="+", default=BENCHES,
                    choices=BENCHES)
    ap.add_argument("--cc", default="clang")
    ap.add_argument("--ccflags", default="-I. -std=c99 -w")
    ap.add_argument("--variants", nargs="+", default=["fp32", "fp64", "ld"],
                    choices=list(VARIANT_DIR))
    ap.add_argument("--pairs", nargs="+", default=list(PAIRS),
                    choices=list(PAIRS))
    ap.add_argument("--timeout", type=int, default=3600)
    ap.add_argument("--skip-build", action="store_true")
    ap.add_argument("--skip-run", action="store_true")
    ap.add_argument("--allow-site-mismatch", action="store_true")
    add_common_args(ap)
    args = ap.parse_args()

    ensure_brtrace(args.rebuild_brtrace)
    print(f"NAS / brtrace   -{args.opt}  benchmarks: {' '.join(args.benchmarks)}"
          f"  {'all branches' if args.all_branches else 'fp-only'}\n")
    recs = [run_bench(b, args) for b in args.benchmarks]

    print("=" * 70)
    for r in recs:
        for label, p in r["pairs"].items():
            b = p["branch"]
            cov = f"{b['coverage']:.2f}%" if b["coverage"] is not None else "n/a"
            print(f"  {r['benchmark']:<3s} {label:<13s} {b['flip_events']:8d} "
                  f"flips @ {b['flip_sites']:3d} sites   coverage {cov}")
    print(f"\nresults under {RESULT_ROOT}/<bench>/{args.opt}/<pair>/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
