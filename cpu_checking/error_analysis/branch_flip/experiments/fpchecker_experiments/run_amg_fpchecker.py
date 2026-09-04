#!/usr/bin/env python3
"""
run_amg_fpchecker.py -- build AMG under FPChecker branch-flip instrumentation
and run it, fp32 and fp64 separately.

    ./run_amg_fpchecker.py                          # both, default eta sweeps
    ./run_amg_fpchecker.py -p fp32 -n 5 --problem 2
    ./run_amg_fpchecker.py --bf-mode both
    ./run_amg_fpchecker.py --shadow-fallback zero

Census config is problem 2, n=5 (fp32 needs ~2x the GMRES iterations of
fp64; one oracle flip at gmres.c:573). Extra gates: sizeof(HYPRE_Real) probe
and iterations != 0. See fpc_common.py for the environment and outputs.
"""

import argparse
import re
import shutil
import sys

from fpc_common import (BENCH_BASE, FPC_INSTALL, WORK_BASE, add_common_args,
                        base_env, check_binary, check_shadow_invariance,
                        collect_manifests, etas_for, fallback_flag, instr_var,
                        make_env, one_line, print_header, result_dir,
                        run_instrumented, sh, write_results)

BENCH_NAME = "amg"
BENCH_ROOT = BENCH_BASE / "amg"
WORK_ROOT = WORK_BASE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"
EXPECTED_SITES = None

PRECISION_FLAG = {"fp32": "-DHYPRE_SINGLE", "fp64": "", "ld": "-DHYPRE_LONG_DOUBLE"}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8, "ld": 16}
ITER_RE = re.compile(r"Iterations\s*=\s*(\d+)")
RESID_RE = re.compile(r"Final Relative Residual Norm\s*=\s*([0-9.eE+-]+)")


def check_width(tree, precision, cc="clang"):
    """sizeof(HYPRE_Real) probe with a plain compiler."""
    probe = WORK_ROOT / "_width_probe.c"
    probe.write_text(
        '#include <stdio.h>\n#include "HYPRE_utilities.h"\n'
        'int main(void){ printf("%zu\\n", sizeof(HYPRE_Real)); return 0; }\n')
    out_bin = WORK_ROOT / "_width_probe"
    cmd = [cc, "-O0", "-DHYPRE_SEQUENTIAL=1"]
    if PRECISION_FLAG[precision]:
        cmd.append(PRECISION_FLAG[precision])
    cmd += [f"-I{tree}/utilities", f"-I{tree}", str(probe), "-o", str(out_bin)]
    rc, _ = sh(cmd, env=base_env())
    if rc != 0:
        return None
    rc, out = sh([str(out_bin)], env=base_env())
    try:
        return int(out.strip())
    except ValueError:
        return None


def build(precision, args, outdir):
    src = BENCH_ROOT / f"amg_{precision}"
    dst = BUILD_ROOT / args.opt / f"amg_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        return None
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for pat in ("*/*.o", "*/*.a", "test/amg"):
        for junk in dst.glob(pat):
            junk.unlink()

    fpcc = FPC_INSTALL / "bin" / "clang-fpchecker"
    if not fpcc.exists():
        print(f"  FATAL: no clang-fpchecker at {fpcc}")
        return None

    env = make_env(precision, args.opt)
    cflags = (f"-g -{args.opt} -DHYPRE_SEQUENTIAL=1 -I../utilities "
              f"{PRECISION_FLAG[precision]} "
              f"{fallback_flag(args.shadow_fallback)}").strip()
    lflags = "-lm -Wl,--allow-multiple-definition"
    cmd = ["make", f"-j{args.jobs}", f"CC={fpcc}",
           f"INCLUDE_CFLAGS={cflags}", f"INCLUDE_LFLAGS={lflags}"]
    print(f"  building ({precision}, -{args.opt}, {instr_var(precision)}=1)")
    rc, _ = sh(cmd, cwd=dst, env=env, log=outdir / "build.log")

    binary = dst / "test" / "amg"
    if not binary.exists():
        print(f"  BUILD FAILED -- see {outdir/'build.log'}")
        return None

    build_log = (outdir / "build.log").read_text()
    gate = check_binary(binary, build_log, outdir, EXPECTED_SITES)
    if gate is None:
        return None
    nsym, banners, site_total, ntab, nmode = gate
    n_man = collect_manifests(dst, outdir, recursive=True)

    width = check_width(dst, precision, args.cc)
    ok_w = (width == EXPECTED_WIDTH[precision])
    print(f"  sizeof(HYPRE_Real) = {width} "
          f"({'ok' if ok_w else 'EXPECTED ' + str(EXPECTED_WIDTH[precision])})")

    (outdir / "build_info.txt").write_text(
        f"precision      = {precision}\n"
        f"opt            = -{args.opt}\n"
        f"jobs           = -j{args.jobs}\n"
        f"CC             = {fpcc}\n"
        f"INCLUDE_CFLAGS = {cflags}\n"
        f"INCLUDE_LFLAGS = {lflags}\n"
        f"instr_var      = {instr_var(precision)}=1\n"
        f"fallback       = {args.shadow_fallback}\n"
        f"fpc_symbols    = {nsym}\n"
        f"branch_sites   = {site_total}\n"
        f"tus            = {len(banners)}\n"
        f"site_tables    = {ntab}\n"
        f"bf_mode_syms   = {nmode}\n"
        f"manifests      = {n_man}\n"
        f"HYPRE_Real     = {width} bytes (expected {EXPECTED_WIDTH[precision]})\n"
        f"openmp         = disabled\n")
    if not ok_w:
        print("  *** WRONG HYPRE_Real WIDTH -- aborting this precision")
        return None
    return binary


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64", "ld"])
    ap.add_argument("--problem", type=int, default=2,
                    help="2 = GMRES modified diagonal (default), 1 = AMG-PCG")
    ap.add_argument("-n", type=int, default=5, help="grid points per axis")
    ap.add_argument("--cc", default="clang", help="plain compiler for the probe")
    add_common_args(ap)
    args = ap.parse_args()

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)
    etas = etas_for(args, args.precision)
    print_header("AMG", args, f"problem {args.problem} n={args.n}^3")

    run_args = ["-problem", str(args.problem), "-n", str(args.n), str(args.n),
                str(args.n), "-P", "1", "1", "1"]
    overall = {}
    for precision in args.precision:
        print(f"===== {precision}  eta: {' '.join(etas[precision])} =====")
        outdir = result_dir(RESULT_ROOT, args.opt, precision, args)
        binary = build(precision, args, outdir)
        if binary is None:
            continue

        records = []
        for eta in etas[precision]:
            tag = (f"{args.opt}_eta{eta}" if args.bf_mode != "shadow"
                   else f"{args.opt}_shadow")
            print(f"  running rule={args.bf_mode} eta={eta}")
            rec, out = run_instrumented(binary, run_args, eta, args, outdir, tag)
            it = ITER_RE.search(out)
            rs = RESID_RE.search(out)
            iters = int(it.group(1)) if it else None
            rec.update({"precision": precision, "problem": args.problem,
                        "n": args.n, "iterations": iters,
                        "residual": rs.group(1) if rs else None})
            if iters == 0:
                rec["consistency"].append("Iterations = 0: solver produced nothing")
            records.append(rec)
            print(f"  -> eta={eta}  {one_line(rec)}   iters={iters}")
            for msg in rec["consistency"]:
                print(f"     *** {msg}")
        inv = check_shadow_invariance(records)
        if inv:
            print(f"     *** {inv}")
        write_results(f"AMG {precision} -- FPChecker branch flips "
                      f"(-{args.opt}, problem {args.problem}, n={args.n}^3, "
                      f"rule={args.bf_mode})", records, outdir,
                      extra_per_record=lambda r: [
                          f"  iterations: {r['iterations']}   "
                          f"residual: {r['residual']}"])
        overall[precision] = records
        print()

    if not overall:
        print("No results produced.")
        return 1

    print("===== results =====")
    for precision, records in overall.items():
        for r in records:
            for rule in ("interval", "shadow"):
                if rule in r:
                    print(f"  {precision:5s} {rule:<9s} eta={r['eta'] or 'n/a':<8s} "
                          f"{r[rule]['flips']:7d} flips @ "
                          f"{r[rule]['locations']:3d} loc   "
                          f"iters={r['iterations']}")
    print(f"\n{BENCH_NAME}/results/{args.opt}/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
