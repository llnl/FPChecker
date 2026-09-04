#!/usr/bin/env python3
"""
run_lulesh_fpchecker.py -- build LULESH under FPChecker branch-flip
instrumentation and run it, fp32 and fp64 separately.

    ./run_lulesh_fpchecker.py                        # both, default eta sweeps
    ./run_lulesh_fpchecker.py -p fp32 -s 10 -i 50
    ./run_lulesh_fpchecker.py --bf-mode both         # interval + shadow rule
    ./run_lulesh_fpchecker.py --shadow-fallback zero

Default eta sweeps: fp32 1e-2 1e-6 1e-10; fp64 1e-8 1e-14 1e-16.
See fpc_common.py for the environment, outputs and knobs.
"""

import argparse
import shutil
import sys

from fpc_common import (BENCH_BASE, DEFAULT_ETA, FPC_INSTALL, HERE, WORK_BASE,
                        add_common_args, check_binary, check_shadow_invariance,
                        collect_manifests, etas_for, fallback_flag, instr_var,
                        make_env, one_line, print_header, result_dir,
                        run_instrumented, sh, write_results)

BENCH_NAME = "lulesh"
BENCH_ROOT = BENCH_BASE / "lulesh"
WORK_ROOT = WORK_BASE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"
EXPECTED_SITES = 75


def build(precision, args, outdir):
    src = BENCH_ROOT / f"lulesh_{precision}"
    dst = BUILD_ROOT / args.opt / f"lulesh_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        return None
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for junk in list(dst.glob("*.o")) + list(dst.glob("lulesh2.0")):
        junk.unlink()

    fpcxx = FPC_INSTALL / "bin" / "clang++-fpchecker"
    if not fpcxx.exists():
        print(f"  FATAL: no clang++-fpchecker at {fpcxx}")
        return None

    flag = "-DLULESH_FP32" if precision == "fp32" else ""
    env = make_env(precision, args.opt)
    cxx = f"{fpcxx} -DUSE_MPI=0 {flag} {fallback_flag(args.shadow_fallback)}".strip()
    cxxflags = f"-g -{args.opt} -I. -std=c++11"
    ldflags = f"-g -{args.opt} -Wl,--allow-multiple-definition"
    cmd = ["make", f"-j{args.jobs}", f"CXX={cxx}",
           f"CXXFLAGS={cxxflags}", f"LDFLAGS={ldflags}"]
    print(f"  building ({precision}, -{args.opt}, {instr_var(precision)}=1)")
    rc, _ = sh(cmd, cwd=dst, env=env, log=outdir / "build.log")

    binary = dst / "lulesh2.0"
    if rc != 0 or not binary.exists():
        print(f"  BUILD FAILED -- see {outdir/'build.log'}")
        return None

    build_log = (outdir / "build.log").read_text()
    gate = check_binary(binary, build_log, outdir, EXPECTED_SITES)
    if gate is None:
        return None
    nsym, banners, site_total, ntab, nmode = gate
    for mod in sorted(banners):
        print(f"      {mod:<20s} mod {banners[mod][0]:<11d} "
              f"{banners[mod][1]:5d} sites")
    n_man = collect_manifests(dst, outdir)

    (outdir / "build_info.txt").write_text(
        f"precision   = {precision}\n"
        f"opt         = -{args.opt}\n"
        f"jobs        = -j{args.jobs}\n"
        f"CXX         = {cxx}\n"
        f"CXXFLAGS    = {cxxflags}\n"
        f"LDFLAGS     = {ldflags}\n"
        f"instr_var   = {instr_var(precision)}=1\n"
        f"fallback    = {args.shadow_fallback}\n"
        f"fpc_symbols = {nsym}\n"
        f"branch_sites= {site_total}\n"
        f"sites_by_tu = "
        + ", ".join(f"{m}={banners[m][1]}" for m in sorted(banners)) + "\n"
        f"module_ids  = "
        + ", ".join(f"{m}={banners[m][0]}" for m in sorted(banners)) + "\n"
        f"site_tables = {ntab}\n"
        f"bf_mode_syms= {nmode}\n"
        f"manifests   = {n_man}\n")
    return binary


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("-s", "--size", type=int, default=5)
    ap.add_argument("-i", "--iter", type=int, default=20)
    add_common_args(ap)
    args = ap.parse_args()

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)
    etas = etas_for(args, args.precision)
    print_header("LULESH", args, f"s={args.size} i={args.iter}")

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
            rec, _ = run_instrumented(
                binary, ["-s", str(args.size), "-i", str(args.iter), "-p"],
                eta, args, outdir, tag, cxx=True)
            rec.update({"precision": precision, "size": args.size,
                        "iterations": args.iter})
            records.append(rec)
            print(f"  -> eta={eta}  {one_line(rec, cxx=True)}")
            for msg in rec["consistency"]:
                print(f"     *** {msg}")
        inv = check_shadow_invariance(records)
        if inv:
            print(f"     *** {inv}")
        write_results(f"LULESH {precision} -- FPChecker branch flips "
                      f"(-{args.opt}, s={args.size}, i={args.iter}, "
                      f"rule={args.bf_mode})", records, outdir)
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
                          f"{r[rule]['locations']} loc")
    print(f"\n{BENCH_NAME}/results/{args.opt}/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
