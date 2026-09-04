#!/usr/bin/env python3
"""
run_amg_brtrace.py -- brtrace census for AMG: build fp32, fp64 and long-double
variants with the BranchTrace pass, run each, and diff the trace pairs.

    ./run_amg_brtrace.py                       # problem 2, n=5
    ./run_amg_brtrace.py -n 10 --problem 1
    ./run_amg_brtrace.py --skip-build

Built through AMG's Makefile (CC / INCLUDE_CFLAGS / INCLUDE_LFLAGS) with the
plugin on the compile line and the runtime object on the link line. Extra
gate: sizeof(HYPRE_Real) probe per variant. -O0 only, single-threaded,
HYPRE_SEQUENTIAL. See gt_common.py for outputs.
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

BENCH_NAME = "amg"
BENCH_ROOT = BENCH_BASE / "amg"
WORK_ROOT = WORK_BASE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

VARIANTS = {"fp32": "amg_fp32", "fp64": "amg_fp64", "ld": "amg_ld"}
PRECISION_FLAG = {"fp32": "-DHYPRE_SINGLE", "fp64": "",
                  "ld": "-DHYPRE_LONG_DOUBLE"}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8, "ld": 16}
BIN_REL = Path("test") / "amg"


def check_width(tree, variant, cc):
    probe = tree / "_width_probe.c"
    probe.write_text('#include <stdio.h>\n#include "HYPRE_utilities.h"\n'
                     'int main(void){ printf("%zu\\n", sizeof(HYPRE_Real)); '
                     'return 0; }\n')
    out_bin = tree / "_width_probe"
    cmd = (f"{cc} -O0 -DHYPRE_SEQUENTIAL=1 {PRECISION_FLAG[variant]} "
           f"-I{tree}/utilities -I{tree} {probe} -o {out_bin}")
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
    for pat in ("**/*.o", "**/*.a", "**/*.br*", "test/amg"):
        for junk in dst.glob(pat):
            if junk.is_file():
                junk.unlink()

    cflags = re.sub(r"\s+", " ",
                    f"{brx_cflags(args.opt, not args.all_branches)} "
                    f"-DHYPRE_SEQUENTIAL=1 -I../utilities "
                    f"{PRECISION_FLAG[variant]}").strip()
    lflags = f"-lm {RUNTIME}"
    cmd = (f"make -j{args.jobs} CC={shlex.quote(args.cc)} "
           f"INCLUDE_CFLAGS={shlex.quote(cflags)} "
           f"INCLUDE_LFLAGS={shlex.quote(lflags)}")
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
    print(f"      {total} branch sites across {len(banners)} TU(s)")

    width = check_width(dst, variant, args.probe_cc)
    ok_w = width == EXPECTED_WIDTH[variant]
    print(f"      sizeof(HYPRE_Real) = {width} "
          f"({'ok' if ok_w else 'EXPECTED ' + str(EXPECTED_WIDTH[variant])})")
    (outdir / "builds" / f"{variant}.build_info.txt").write_text(
        f"variant     = {variant}\nopt         = -{args.opt}\n"
        f"cmd         = {cmd}\nHYPRE_Real  = {width}\n"
        f"sites_total = {total}\ntus         = {len(banners)}\n")
    if not ok_w:
        die(f"wrong HYPRE_Real width for {variant}")
    return binary, banners


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--problem", type=int, default=2)
    ap.add_argument("-n", type=int, default=5, help="grid points per axis")
    ap.add_argument("--cc", default="clang")
    ap.add_argument("--probe-cc", default="clang")
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
    print(f"AMG / brtrace   -{args.opt}  problem {args.problem} n={args.n}^3  "
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
            binaries[v] = BUILD_ROOT / args.opt / VARIANTS[v] / BIN_REL

    run_args = ["-problem", str(args.problem), "-n", str(args.n), str(args.n),
                str(args.n), "-P", "1", "1", "1"]
    traces = {v: outdir / "traces" / f"amg_{v}.out" for v in args.variants}
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
    rec = {"title": f"AMG brtrace census (-{args.opt}, problem {args.problem}, "
                    f"n={args.n}^3)",
           "opt": args.opt, "problem": args.problem, "n": args.n,
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
        if (outdir / "traces" / f"amg_{a}_sel.out").exists():
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
