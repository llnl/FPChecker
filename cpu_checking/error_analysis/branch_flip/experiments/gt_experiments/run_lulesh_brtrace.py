#!/usr/bin/env python3
"""
run_lulesh_brtrace.py -- brtrace census for LULESH: build fp32, fp64 and
long-double variants with the BranchTrace pass, run each, and diff the
trace pairs (fp32 vs fp64, fp64 vs ld).

    ./run_lulesh_brtrace.py                    # -s 5 -i 20, all variants
    ./run_lulesh_brtrace.py -s 10 -i 50
    ./run_lulesh_brtrace.py --variants fp32 fp64 --pairs fp32_vs_fp64
    ./run_lulesh_brtrace.py --skip-build       # reuse binaries, rerun + diff

Each variant is one direct clang++ invocation over the same source list, so
the three builds differ only in the precision define. -O0 only: site ids are
positional and only correspond across builds when nothing is optimised.
Runs are single-threaded. See gt_common.py for outputs.
"""

import argparse
import re
import shutil
import sys

from gt_common import (BENCH_BASE, PAIRS, WORK_BASE, add_common_args,
                       arith_profile, brx_cflags, check_precisions,
                       check_site_agreement, collect_side_tables, die,
                       ensure_brtrace, parse_banners, print_pair, run_diff,
                       run_traced, sh, write_summary, RUNTIME)

BENCH_NAME = "lulesh"
BENCH_ROOT = BENCH_BASE / "lulesh"
WORK_ROOT = WORK_BASE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

SRCS = ["lulesh.cc", "lulesh-comm.cc", "lulesh-init.cc",
        "lulesh-util.cc", "lulesh-viz.cc"]
VARIANTS = {"fp32": "lulesh_fp32", "fp64": "lulesh_fp64", "ld": "lulesh_ld"}
DEFINES = {"fp32": "-DLULESH_FP32", "fp64": "", "ld": "-DLULESH_LD"}
BIN = "lulesh.brx"


def build(variant, args, outdir):
    src = BENCH_ROOT / VARIANTS[variant]
    dst = BUILD_ROOT / args.opt / VARIANTS[variant]
    if not src.is_dir():
        die(f"no source tree at {src}")
    missing = [s for s in SRCS if not (src / s).exists()]
    if missing:
        die(f"{src} is missing sources: {', '.join(missing)}")
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for junk in (list(dst.glob("*.o")) + list(dst.glob("lulesh2.0")) +
                 list(dst.glob("*.br*"))):
        junk.unlink()

    cflags = brx_cflags(args.opt, not args.all_branches)
    cmd = re.sub(r"\s+", " ",
                 f"{args.cxx} {cflags} -DUSE_MPI=0 -I. -std=c++11 "
                 f"{DEFINES[variant]} {' '.join(SRCS)} {RUNTIME} -lm -o {BIN}")
    log = outdir / "builds" / f"{variant}.build.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    print(f"  {variant}")
    rc, out = sh(cmd, cwd=dst, log=log)
    if rc != 0 or not (dst / BIN).exists():
        for line in out.strip().splitlines()[-10:]:
            print(f"    {line[:200]}")
        die(f"build failed for {variant} -- see {log}")

    banners = parse_banners(out)
    if not banners:
        die(f"no [BranchTrace] banner in the {variant} build -- see {log}")
    total = sum(b[1] for b in banners.values())
    print(f"      {total} branch sites, "
          f"{sum(b[2] for b in banners.values())} select sites across "
          f"{len(banners)} TU(s)")
    for mod in sorted(banners):
        print(f"        {mod:<20s} mod {banners[mod][0]:<11d} "
              f"{banners[mod][1]:5d} sites")
    (outdir / "builds" / f"{variant}.build_info.txt").write_text(
        f"variant     = {variant}\nopt         = -{args.opt}\n"
        f"cmd         = {cmd}\nsites_total = {total}\nsites_by_tu = "
        + ", ".join(f"{m}={banners[m][1]}" for m in sorted(banners)) + "\n")
    return dst / BIN, banners


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-s", "--size", type=int, default=5)
    ap.add_argument("-i", "--iter", type=int, default=20)
    ap.add_argument("--cxx", default="clang++")
    ap.add_argument("--variants", nargs="+", default=["fp32", "fp64", "ld"],
                    choices=list(VARIANTS))
    ap.add_argument("--pairs", nargs="+", default=list(PAIRS),
                    choices=list(PAIRS))
    ap.add_argument("--skip-build", action="store_true")
    ap.add_argument("--skip-run", action="store_true")
    ap.add_argument("--allow-site-mismatch", action="store_true")
    add_common_args(ap)
    args = ap.parse_args()

    ensure_brtrace(args.rebuild_brtrace)
    outdir = RESULT_ROOT / args.opt
    (outdir / "traces").mkdir(parents=True, exist_ok=True)
    print(f"LULESH / brtrace   -{args.opt}  s={args.size} i={args.iter}  "
          f"{'all branches' if args.all_branches else 'fp-only'}")

    binaries, banners, profiles = {}, {}, {}
    if not args.skip_build:
        print("[build]")
        for v in args.variants:
            binaries[v], banners[v] = build(v, args, outdir)
            profiles[v] = arith_profile(binaries[v])
        if not check_site_agreement(banners, "site count") \
                and not args.allow_site_mismatch:
            die("site counts differ between variants")
        if not check_precisions(profiles, "precision"):
            die("two variants were built at the same precision")
    else:
        for v in args.variants:
            binaries[v] = BUILD_ROOT / args.opt / VARIANTS[v] / BIN

    traces = {v: outdir / "traces" / f"lulesh_{v}.out" for v in args.variants}
    if not args.skip_run:
        print("[run]")
        for v in args.variants:
            if not binaries[v].exists():
                die(f"{binaries[v]} not found -- drop --skip-build")
            rc, _ = run_traced(binaries[v], ["-s", str(args.size),
                                             "-i", str(args.iter)],
                               traces[v], cwd=binaries[v].parent,
                               log=outdir / "builds" / f"{v}.run.log")
            if rc != 0:
                die(f"run failed for {v} (exit {rc})")
            print(f"  {v:<5s} {traces[v].stat().st_size // 12:,} events")

    print("[diff]")
    rec = {"title": f"LULESH brtrace census (-{args.opt}, s={args.size}, "
                    f"i={args.iter})",
           "opt": args.opt, "size": args.size, "iterations": args.iter,
           "fp_only": not args.all_branches,
           "sites_by_variant": {v: {m: b[1] for m, b in banners[v].items()}
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
        mods = BUILD_ROOT / args.opt / VARIANTS[b]
        collect_side_tables(mods, pdir / "sidetables")
        res = run_diff(traces[a], traces[b], mods, pdir, "branch")
        sel = None
        sa, sb = (str(traces[a]).replace(".out", "_sel.out"),
                  str(traces[b]).replace(".out", "_sel.out"))
        if (outdir / "traces" / f"lulesh_{a}_sel.out").exists():
            sel = run_diff(sa, sb, mods, pdir, "select")
        rec["pairs"][label] = {"a": a, "b": b, "branch": res, "select": sel}
        print_pair(label, res, sel)
    write_summary(outdir, rec)
    print(f"\nresults under {outdir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
