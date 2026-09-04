#!/usr/bin/env python3
"""
run_quicksilver_fpchecker.py -- build QuickSilver under FPChecker branch-flip
instrumentation and run it, fp32 and fp64 separately.

    ./run_quicksilver_fpchecker.py                  # both, default eta sweeps
    ./run_quicksilver_fpchecker.py -p fp32 -n 4000 -x 5 -N 3
    ./run_quicksilver_fpchecker.py --bf-mode both
    ./run_quicksilver_fpchecker.py --shadow-fallback zero

Census config: -n 4000 mesh 5^3 N=3 (first fp32/fp64 divergence at cycle 2).
Extra gate: sizeof(qs_real) probe. Per-cycle tallies are recorded and the
fp32/fp64 divergence printed. See fpc_common.py for the environment and
outputs.
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

BENCH_NAME = "quicksilver"
BENCH_ROOT = BENCH_BASE / "quicksilver"
WORK_ROOT = WORK_BASE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"
EXPECTED_SITES = None

PRECISION_FLAG = {"fp32": "-DQS_FP32", "fp64": "", "ld": "-DQS_LD"}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8, "ld": 16}
TALLY_RE = re.compile(r"^\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+"
                      r"(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+\d+\s+(\d+)")


def check_width(tree, precision, cxx="clang++"):
    """sizeof(qs_real) probe with a plain compiler."""
    probe = WORK_ROOT / "_width_probe.cc"
    probe.write_text('#include "QS_Precision.hh"\n#include <cstdio>\n'
                     'int main(){ printf("%zu\\n", sizeof(qs_real)); '
                     'return 0; }\n')
    out_bin = WORK_ROOT / "_width_probe"
    cmd = [cxx, "-O0", "-std=c++11", "-include", "cstdint"]
    if PRECISION_FLAG[precision]:
        cmd.append(PRECISION_FLAG[precision])
    cmd += [f"-I{tree}", str(probe), "-o", str(out_bin)]
    rc, _ = sh(cmd, env=base_env())
    if rc != 0:
        return None
    rc, out = sh([str(out_bin)], env=base_env())
    try:
        return int(out.strip())
    except ValueError:
        return None


def build(precision, args, outdir):
    src = BENCH_ROOT / f"qs_{precision}"
    dst = BUILD_ROOT / args.opt / f"qs_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        return None
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for junk in list(dst.glob("*.o")) + list(dst.glob("qs")):
        junk.unlink()

    fpcxx = FPC_INSTALL / "bin" / "clang++-fpchecker"
    if not fpcxx.exists():
        print(f"  FATAL: no clang++-fpchecker at {fpcxx}")
        return None

    env = make_env(precision, args.opt)
    cxxf = (f"-std=c++11 -g -{args.opt} -include cstdint "
            f"{PRECISION_FLAG[precision]} "
            f"{fallback_flag(args.shadow_fallback)}").strip()
    ldf = f"-g -{args.opt} -Wl,--allow-multiple-definition"
    cmd = ["make", f"-j{args.jobs}", f"CXX={fpcxx}",
           f"CXXFLAGS={cxxf}", "CPPFLAGS=", f"LDFLAGS={ldf}"]
    print(f"  building ({precision}, -{args.opt}, {instr_var(precision)}=1)")
    rc, _ = sh(cmd, cwd=dst, env=env, log=outdir / "build.log")

    binary = dst / "qs"
    if not binary.exists():
        print(f"  BUILD FAILED -- see {outdir/'build.log'}")
        return None

    build_log = (outdir / "build.log").read_text()
    gate = check_binary(binary, build_log, outdir, EXPECTED_SITES)
    if gate is None:
        return None
    nsym, banners, site_total, ntab, nmode = gate
    if site_total > (1 << 13):
        print(f"  *** {site_total} sites is over half the site-table capacity")
    n_man = collect_manifests(dst, outdir, recursive=True)

    width = check_width(dst, precision, args.cxx)
    ok_w = (width == EXPECTED_WIDTH[precision])
    print(f"  sizeof(qs_real) = {width} "
          f"({'ok' if ok_w else 'EXPECTED ' + str(EXPECTED_WIDTH[precision])})")

    (outdir / "build_info.txt").write_text(
        f"precision   = {precision}\n"
        f"opt         = -{args.opt}\n"
        f"jobs        = -j{args.jobs}\n"
        f"CXX         = {fpcxx}\n"
        f"CXXFLAGS    = {cxxf}\n"
        f"LDFLAGS     = {ldf}\n"
        f"instr_var   = {instr_var(precision)}=1\n"
        f"fallback    = {args.shadow_fallback}\n"
        f"fpc_symbols = {nsym}\n"
        f"branch_sites= {site_total}\n"
        f"tus         = {len(banners)}\n"
        f"site_tables = {ntab}\n"
        f"bf_mode_syms= {nmode}\n"
        f"manifests   = {n_man}\n"
        f"qs_real     = {width} bytes (expected {EXPECTED_WIDTH[precision]})\n"
        f"mpi         = serial\n")
    if not ok_w:
        print("  *** WRONG qs_real WIDTH -- aborting this precision")
        return None
    return binary


def parse_tallies(out):
    tallies = []
    for l in out.splitlines():
        m = TALLY_RE.match(l)
        if m:
            tallies.append({"cycle": int(m.group(1)),
                            "absorb": int(m.group(3)),
                            "scatter": int(m.group(4)),
                            "fission": int(m.group(5)),
                            "num_seg": int(m.group(6))})
    return tallies


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64", "ld"])
    ap.add_argument("-n", type=int, default=4000, help="particles")
    ap.add_argument("-x", type=int, default=5, help="mesh cells per axis")
    ap.add_argument("-N", type=int, default=3, help="cycles")
    ap.add_argument("--cxx", default="clang++",
                    help="plain compiler for the probe")
    add_common_args(ap)
    args = ap.parse_args()

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)
    etas = etas_for(args, args.precision)
    print_header("QuickSilver", args, f"n={args.n} mesh={args.x}^3 N={args.N}")

    run_args = ["-n", str(args.n), "-X", "10", "-Y", "10", "-Z", "10",
                "-x", str(args.x), "-y", str(args.x), "-z", str(args.x),
                "-I", "1", "-J", "1", "-K", "1", "-N", str(args.N)]
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
            rec, out = run_instrumented(binary, run_args, eta, args, outdir,
                                        tag, cxx=True)
            rec.update({"precision": precision, "n": args.n, "mesh": args.x,
                        "cycles": args.N, "tallies": parse_tallies(out)})
            records.append(rec)
            print(f"  -> eta={eta}  {one_line(rec, cxx=True)}")
            for msg in rec["consistency"]:
                print(f"     *** {msg}")
        inv = check_shadow_invariance(records)
        if inv:
            print(f"     *** {inv}")

        def tally_lines(r):
            L = []
            if r["tallies"]:
                L.append("  cycle   absorb  scatter  fission  num_seg")
                for t in r["tallies"]:
                    L.append(f"  {t['cycle']:5d} {t['absorb']:8d} "
                             f"{t['scatter']:8d} {t['fission']:8d} "
                             f"{t['num_seg']:8d}")
            return L
        write_results(f"QuickSilver {precision} -- FPChecker branch flips "
                      f"(-{args.opt}, n={args.n} mesh={args.x}^3 N={args.N}, "
                      f"rule={args.bf_mode})", records, outdir,
                      extra_per_record=tally_lines)
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
                          f"{r[rule]['flips']:8d} flips @ "
                          f"{r[rule]['locations']:3d} loc")

    if "fp32" in overall and "fp64" in overall:
        t32 = overall["fp32"][0]["tallies"]
        t64 = overall["fp64"][0]["tallies"]
        if t32 and t64:
            print("\n===== control-flow divergence (fp32 vs fp64) =====")
            diverged = False
            for a, b in zip(t32, t64):
                if a != b:
                    diverged = True
                    print(f"  cycle {a['cycle']}: "
                          f"absorb {a['absorb']}/{b['absorb']}  "
                          f"scatter {a['scatter']}/{b['scatter']}  "
                          f"fission {a['fission']}/{b['fission']}  "
                          f"num_seg {a['num_seg']}/{b['num_seg']}")
            if not diverged:
                print("  none -- tallies identical")

    print(f"\n{BENCH_NAME}/results/{args.opt}/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
