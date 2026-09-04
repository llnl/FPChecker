#!/usr/bin/env python3
"""
run_quicksilver_brtrace.py -- brtrace census for QuickSilver: build fp32,
fp64 and long-double variants with the BranchTrace pass, run each, and diff
the trace pairs.

    ./run_quicksilver_brtrace.py                    # -n 4000 mesh 5^3 N=3
    ./run_quicksilver_brtrace.py -n 2000 -x 4 -N 2
    ./run_quicksilver_brtrace.py --skip-build

Built through QuickSilver's Makefile with CPPFLAGS cleared (no OpenMP, no
MPI) and the runtime object on the link line. Extra gate: sizeof(qs_real)
probe. See gt_common.py for outputs.
"""

import argparse
import re
import shlex
import shutil
import sys
from pathlib import Path

from gt_common import (BENCH_BASE, PAIRS, WORK_BASE, add_common_args,
                       arith_profile, brx_cflags, check_precisions,
                       check_site_agreement, collect_side_tables, die,
                       ensure_brtrace, parse_banners, print_pair, run_diff,
                       run_traced, sh, write_summary, RUNTIME)

BENCH_NAME = "quicksilver"
BENCH_ROOT = BENCH_BASE / "quicksilver"
WORK_ROOT = WORK_BASE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

VARIANTS = {"fp32": "qs_fp32", "fp64": "qs_fp64", "ld": "qs_ld"}
PRECISION_FLAG = {"fp32": "-DQS_FP32", "fp64": "", "ld": "-DQS_LD"}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8, "ld": 16}
BIN_REL = Path("qs")


def check_width(tree, variant, cxx):
    probe = tree / "_width_probe.cc"
    probe.write_text('#include "QS_Precision.hh"\n#include <cstdio>\n'
                     'int main(){ printf("%zu\\n", sizeof(qs_real)); '
                     'return 0; }\n')
    out_bin = tree / "_width_probe"
    cmd = (f"{cxx} -O0 -std=c++11 -include cstdint {PRECISION_FLAG[variant]} "
           f"-I{tree} -I{tree}/src {probe} -o {out_bin}")
    rc, _ = sh(cmd)
    if rc != 0:
        return None
    rc, out = sh(str(out_bin))
    for f in (probe, out_bin):
        f.unlink(missing_ok=True)
    try:
        return int(out.strip())
    except ValueError:
        return None


def build(variant, args, outdir):
    src = BENCH_ROOT / VARIANTS[variant]
    dst = BUILD_ROOT / args.opt / VARIANTS[variant]
    if not src.is_dir():
        die(f"no source tree at {src}")
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for pat in ("**/*.o", "**/*.br*", "qs", "src/qs"):
        for junk in dst.glob(pat):
            if junk.is_file():
                junk.unlink()

    cflags = re.sub(r"\s+", " ",
                    f"{brx_cflags(args.opt, not args.all_branches)} "
                    f"-std=c++11 -include cstdint "
                    f"{PRECISION_FLAG[variant]}").strip()
    lflags = f"-g -{args.opt} {RUNTIME} -lm"
    cmd = (f"make -j{args.jobs} CXX={shlex.quote(args.cxx)} "
           f"CXXFLAGS={shlex.quote(cflags)} CPPFLAGS= "
           f"LDFLAGS={shlex.quote(lflags)}")
    log = outdir / "builds" / f"{variant}.build.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    print(f"  {variant}")
    rc, out = sh(cmd, cwd=dst, log=log)
    binary = dst / BIN_REL
    if not binary.exists():
        for line in out.splitlines():
            if "error:" in line.lower():
                print(f"    {line[:180]}")
        die(f"build failed for {variant} -- see {log}")

    banners = parse_banners(out)
    if not banners:
        die(f"no [BranchTrace] banner in the {variant} build -- see {log}")
    total = sum(b[1] for b in banners.values())
    print(f"      {total} branch sites, "
          f"{sum(b[2] for b in banners.values())} select sites across "
          f"{len(banners)} TU(s)")

    width = check_width(dst, variant, args.probe_cxx)
    ok_w = width == EXPECTED_WIDTH[variant]
    print(f"      sizeof(qs_real) = {width} "
          f"({'ok' if ok_w else 'EXPECTED ' + str(EXPECTED_WIDTH[variant])})")
    (outdir / "builds" / f"{variant}.build_info.txt").write_text(
        f"variant     = {variant}\nopt         = -{args.opt}\n"
        f"cmd         = {cmd}\nqs_real     = {width}\n"
        f"sites_total = {total}\ntus         = {len(banners)}\n")
    if not ok_w:
        die(f"wrong qs_real width for {variant}")
    return binary, banners


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-n", type=int, default=4000, help="particles")
    ap.add_argument("-x", type=int, default=5, help="mesh cells per axis")
    ap.add_argument("-N", type=int, default=3, help="cycles")
    ap.add_argument("--cxx", default="clang++")
    ap.add_argument("--probe-cxx", default="clang++")
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
    print(f"QuickSilver / brtrace   -{args.opt}  n={args.n} mesh={args.x}^3 "
          f"N={args.N}  {'all branches' if args.all_branches else 'fp-only'}")

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
            binaries[v] = BUILD_ROOT / args.opt / VARIANTS[v] / BIN_REL

    run_args = ["-n", str(args.n), "-X", "10", "-Y", "10", "-Z", "10",
                "-x", str(args.x), "-y", str(args.x), "-z", str(args.x),
                "-I", "1", "-J", "1", "-K", "1", "-N", str(args.N)]
    traces = {v: outdir / "traces" / f"qs_{v}.out" for v in args.variants}
    if not args.skip_run:
        print("[run]")
        for v in args.variants:
            if not binaries[v].exists():
                die(f"{binaries[v]} not found -- drop --skip-build")
            rc, _ = run_traced(binaries[v], run_args, traces[v],
                               cwd=binaries[v].parent,
                               log=outdir / "builds" / f"{v}.run.log")
            if rc != 0:
                die(f"run failed for {v} (exit {rc})")
            print(f"  {v:<5s} {traces[v].stat().st_size // 12:,} events")

    print("[diff]")
    rec = {"title": f"QuickSilver brtrace census (-{args.opt}, n={args.n} "
                    f"mesh={args.x}^3 N={args.N})",
           "opt": args.opt, "n": args.n, "mesh": args.x, "cycles": args.N,
           "fp_only": not args.all_branches,
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
        mods = BUILD_ROOT / args.opt / VARIANTS[b]
        collect_side_tables(mods, pdir / "sidetables", recursive=True)
        res = run_diff(traces[a], traces[b], mods, pdir, "branch")
        sel = None
        if (outdir / "traces" / f"qs_{a}_sel.out").exists():
            sel = run_diff(str(traces[a]).replace(".out", "_sel.out"),
                           str(traces[b]).replace(".out", "_sel.out"),
                           mods, pdir, "select")
        rec["pairs"][label] = {"a": a, "b": b, "branch": res, "select": sel}
        print_pair(label, res, sel)
    write_summary(outdir, rec)
    print(f"\nresults under {outdir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
