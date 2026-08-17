#!/usr/bin/env python3
"""
run_amg_fpchecker.py

Build AMG under FPChecker branch-instability instrumentation and run it,
keeping fp32 and fp64 results completely separate.

Place in: branch_flip/experiments/fpchecker_experiments/

    ./run_amg_fpchecker.py                          # both, default eta sweeps
    ./run_amg_fpchecker.py -p fp32                  # fp32 only
    ./run_amg_fpchecker.py --opt O2                 # -O2 instead of -O0
    ./run_amg_fpchecker.py --eta-fp64 1e-12         # override one precision
    ./run_amg_fpchecker.py -n 10                    # bigger problem

Default eta sweeps (scaled to machine epsilon, ~9 orders apart):
    fp32  1e-2  1e-6  1e-10      (eps 1.19e-07)
    fp64  1e-8  1e-14 1e-16      (eps 2.22e-16)

Instrumentation variables (set for BOTH compile and link):
    fp32 -> FPC_INSTRUMENT_ERR_TRACKING=1
    fp64 -> FPC_INSTRUMENT_ERR_TRACKING_FP64=1  (base var explicitly cleared)

Layout produced:
    amg/
      results/
        O0/  fp32/  build.log  run_O0_eta1e-2.log ...  summary.txt
             fp64/  ...        summary.json  build_info.txt
        O2/  ...
      build/
        O0/amg_fp32/  O0/amg_fp64/  O2/amg_fp32/  O2/amg_fp64/

DIFFERENCES FROM THE LULESH HARNESS
  * AMG is C, so the frontend is clang-fpchecker (not clang++-fpchecker)
    and there is no name mangling to undo.
  * Build vars are CC / INCLUDE_CFLAGS / INCLUDE_LFLAGS, and the top-level
    Makefile loops over subdirectories -- command-line assignments
    propagate to the sub-makes, so they stay authoritative.
  * Two extra verification gates, because AMG fails SILENTLY in ways
    LULESH does not:
      1. sizeof(HYPRE_Real) probe -- a clean build does not prove the
         -DHYPRE_SINGLE / -DHYPRE_LONG_DOUBLE flag reached every TU.
      2. iterations != 0 -- upstream's sequential MPI stubs have no
         FLOAT/LONG_DOUBLE case and no default, so at non-double precision
         Allreduce falls through, never writes recvbuf, and returns
         success.  Symptom: "Iterations = 0, Residual = 0.000000e+00,
         FOM = -nan" from a binary that built and ran cleanly.  The
         benchmark trees are patched for this; the gate catches a
         regression or an unpatched tree.

CENSUS CONFIG: problem 2 only.  Problem 1 (AMG-PCG on a 27-point Laplace)
shows ZERO iteration divergence between fp32 and fp64 at any size tested --
well-conditioned and AMG-preconditioned, fp32 tracks fp64 exactly.  Problem
2 is GMRES on a modified-diagonal system; Gram-Schmidt is precision
sensitive, and fp32 needs roughly twice the iterations of fp64.

Measured iteration counts, problem 2 (uninstrumented, gcc 13.3):

     n     fp64    ld     fp32    ratio   fp32 runtime
    3^3     132   132      255    1.93x     0.011 s
    4^3     143   143      252    1.76x     0.014 s
    5^3     150   150      297    1.98x     0.029 s   <-- default
    6^3     162   162      401    2.48x     0.061 s
    8^3     168   168      437    2.60x
   10^3     180   180      386    2.14x
   15^3     196   196      324    1.65x
   20^3     205   205      330    1.61x

n=5 is the default: it matches the LULESH s=5 census size, diverges by
~2x, and runs in milliseconds uninstrumented.  Even 3^3 diverges, so
there is headroom to go smaller if instrumented runs are slow.

fp64 == ld at EVERY size tested, so fp64 is oracle-stable for AMG --
consistent with NAS, LULESH and QuickSilver.  Note the divergence ratio is
non-monotonic in n (1.76x at 4^3, 2.48x at 6^3, 1.61x at 20^3), so a bigger
problem does not mean more divergence.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

# --------------------------------------------------------------------
DEFAULT_ETA = {
    "fp32": ["1e-2", "1e-6", "1e-10"],
    "fp64": ["1e-8", "1e-14", "1e-16"],
}

PRECISION_FLAG = {
    "fp32": "-DHYPRE_SINGLE",
    "fp64": "",                      # default typedef
    "ld":   "-DHYPRE_LONG_DOUBLE",
}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8, "ld": 16}

FPC_ROOT = Path(os.environ.get("FPC_ROOT", "/usr/workspace/das9/fpchecker_bf"))
FPC_INSTALL = Path(os.environ.get("FPC_INSTALL", FPC_ROOT / "install"))
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT",
    FPC_ROOT / "cpu_checking/error_analysis/branch_flip/benchmarks/amg"))
CONDA_LIB = os.environ.get("CONDA_PREFIX", "") and \
    os.path.join(os.environ["CONDA_PREFIX"], "lib")

HERE = Path(__file__).resolve().parent
BENCH_NAME = "amg"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

FLIP_RE = re.compile(r"Unstable branch", re.I)
SITE_RE = re.compile(
    r"at\s+\S*?([A-Za-z0-9_.+-]+\.(?:c|cc|cpp|h|hh)):(\d+)"
    r"(?:\s+in\s+([A-Za-z0-9_$.]+))?")
ITER_RE = re.compile(r"Iterations\s*=\s*(\d+)")
RESID_RE = re.compile(r"Final Relative Residual Norm\s*=\s*([0-9.eE+-]+)")


def sh(cmd, cwd=None, env=None, log=None):
    p = subprocess.run(cmd, cwd=cwd, env=env, shell=isinstance(cmd, str),
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True, errors="replace")
    if log:
        log.write_text(p.stdout)
    return p.returncode, p.stdout


def base_env():
    env = os.environ.copy()
    if CONDA_LIB:
        env["LD_LIBRARY_PATH"] = CONDA_LIB + ":" + env.get("LD_LIBRARY_PATH", "")
    return env


def make_env(precision):
    """fp32 -> FPC_INSTRUMENT_ERR_TRACKING=1
       fp64 -> FPC_INSTRUMENT_ERR_TRACKING_FP64=1 ONLY (base var cleared,
               so the two passes cannot both instrument and double-count).
       Both must be present at compile AND link; missing them at link gives
       a binary that builds and runs but has zero _FPC symbols."""
    env = base_env()
    env.pop("FPC_INSTRUMENT_ERR_TRACKING", None)
    env.pop("FPC_INSTRUMENT_ERR_TRACKING_FP64", None)
    if precision == "fp64":
        env["FPC_INSTRUMENT_ERR_TRACKING_FP64"] = "1"
    else:
        env["FPC_INSTRUMENT_ERR_TRACKING"] = "1"
    return env


def check_width(tree, precision, cc="clang"):
    """Compile a tiny probe against this tree's headers and report
    sizeof(HYPRE_Real).  A clean AMG build does NOT prove the precision
    flag took effect -- the failure is silent."""
    probe = WORK_ROOT / "_width_probe.c"
    probe.write_text(
        '#include <stdio.h>\n#include "HYPRE_utilities.h"\n'
        'int main(void){ printf("%zu\\n", sizeof(HYPRE_Real)); return 0; }\n')
    out_bin = WORK_ROOT / "_width_probe"
    flag = PRECISION_FLAG[precision]
    cmd = [cc, "-O0", "-DHYPRE_SEQUENTIAL=1"]
    if flag:
        cmd.append(flag)
    cmd += [f"-I{tree}/utilities", f"-I{tree}", str(probe), "-o", str(out_bin)]
    rc, _ = sh(cmd, env=base_env())
    if rc != 0:
        return None
    rc, out = sh([str(out_bin)], env=base_env())
    try:
        return int(out.strip())
    except ValueError:
        return None


def build(precision, opt, outdir, jobs=1, cc="clang"):
    src = BENCH_ROOT / f"amg_{precision}"
    dst = BUILD_ROOT / opt / f"amg_{precision}"
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

    fpcc = FPC_INSTALL / "bin" / "clang-fpchecker"      # C, not C++
    if not fpcc.exists():
        print(f"  FATAL: no clang-fpchecker at {fpcc}")
        print("         set FPC_INSTALL=/path/to/install")
        return None

    flag = PRECISION_FLAG[precision]
    env = make_env(precision)
    ivar = ("FPC_INSTRUMENT_ERR_TRACKING_FP64" if precision == "fp64"
            else "FPC_INSTRUMENT_ERR_TRACKING")
    print(f"  instrumentation var: {ivar}=1")

    # One optimisation level on compile AND link.  The benchmark trees have
    # their own Makefile.include values; command-line assignments override
    # them and propagate to the per-directory sub-makes.
    # -Wl,--allow-multiple-definition works around duplicate _FPC_STABILITY_*
    # symbols emitted per TU from Runtime_error.h.
    # No -fopenmp: upstream Makefile.include enables it by default, which
    # would make reductions non-deterministic.
    cflags = f"-g -{opt} -DHYPRE_SEQUENTIAL=1 -I../utilities {flag}".strip()
    lflags = f"-lm -Wl,--allow-multiple-definition"
    cmd = ["make", f"-j{jobs}", f"CC={fpcc}",
           f"INCLUDE_CFLAGS={cflags}", f"INCLUDE_LFLAGS={lflags}"]

    print(f"  building ({precision}, -{opt}, -j{jobs})")
    rc, _ = sh(cmd, cwd=dst, env=env, log=outdir / "build.log")

    binary = dst / "test" / "amg"
    if not binary.exists():
        print(f"  BUILD FAILED -- see {outdir/'build.log'}")
        for line in (outdir / "build.log").read_text().splitlines():
            if "error:" in line.lower():
                print("   ", line[:150])
                break
        return None

    # Gate 1: instrumentation actually present.
    _, nm_out = sh(["nm", str(binary)])
    nsym = sum(1 for l in nm_out.splitlines() if "_FPC" in l)
    print(f"  _FPC symbol count = {nsym}")

    # Gate 2: the precision flag reached the headers (AMG-specific).
    width = check_width(dst, precision, cc)
    ok_w = (width == EXPECTED_WIDTH[precision])
    print(f"  sizeof(HYPRE_Real) = {width} "
          f"({'ok' if ok_w else 'EXPECTED ' + str(EXPECTED_WIDTH[precision])})")

    # Gate 3: correct libm variant reached the object code (AMG-specific).
    _, und = sh(["nm", "-u", str(binary)])
    libm = sorted({m for m in re.findall(
        r"\b(sqrtf?l?|fabsf?l?|powf?l?|expf?l?|logf?l?)\b", und)})
    print(f"  libm symbols       = {' '.join(libm) if libm else '(none)'}")

    (outdir / "build_info.txt").write_text(
        f"precision      = {precision}\n"
        f"opt            = -{opt}   (compile AND link)\n"
        f"jobs           = -j{jobs}\n"
        f"CC             = {fpcc}\n"
        f"INCLUDE_CFLAGS = {cflags}\n"
        f"INCLUDE_LFLAGS = {lflags}\n"
        f"instr_var      = {ivar}=1\n"
        f"fpc_symbols    = {nsym}\n"
        f"HYPRE_Real     = {width} bytes (expected {EXPECTED_WIDTH[precision]})\n"
        f"libm symbols   = {' '.join(libm)}\n"
        f"openmp         = disabled (no -fopenmp)\n")

    if nsym == 0:
        print("  *** UNINSTRUMENTED BINARY -- env vars did not reach "
              "compile and/or link. Aborting this precision.")
        return None
    if not ok_w:
        print("  *** WRONG HYPRE_Real WIDTH -- precision flag did not take. "
              "Aborting this precision.")
        return None
    return binary


def run(binary, precision, eta, problem, n, maxwarn, outdir, opt):
    env = base_env()
    # FPC_STABILITY_ETA_REL, not FPC_STABILITY_ETA -- the wrong name leaves
    # every run on the default and makes an eta sweep produce identical
    # results without any error.
    env["FPC_STABILITY_ETA_REL"] = eta
    env["FPC_STABILITY_MAX_WARNINGS"] = str(maxwarn)

    log = outdir / f"run_{opt}_eta{eta}.log"
    print(f"  running eta={eta} -> {log.relative_to(HERE)}")
    args = ["-problem", str(problem), "-n", str(n), str(n), str(n),
            "-P", "1", "1", "1"]
    rc, out = sh(["stdbuf", "-i0", "-o0", "-e0", str(binary)] + args,
                 cwd=binary.parent, env=env, log=log)

    flips = [l for l in out.splitlines() if FLIP_RE.search(l)]
    sites = Counter()
    unparsed = 0
    for l in flips:
        m = SITE_RE.search(l)
        if not m:
            unparsed += 1
            continue
        key = f"{m.group(1)}:{m.group(2)}"
        if m.group(3):
            key += f"  [{m.group(3)}]"     # C: no mangling to undo
        sites[key] += 1

    it = ITER_RE.search(out)
    rs = RESID_RE.search(out)
    iters = int(it.group(1)) if it else None
    wrote = re.search(r"Successfully wrote (\d+) error entries", out)

    # Gate: iterations == 0 means the solver produced nothing (MPI-stub
    # fall-through at non-double precision).
    if iters == 0:
        print("  *** WARNING: Iterations = 0 -- solver produced no result. "
              "This is the mpistubs FLOAT/LONG_DOUBLE fall-through; the tree "
              "is unpatched or the patch regressed.")

    return {
        "precision": precision, "eta": eta, "opt": opt,
        "problem": problem, "n": n,
        "iterations": iters,
        "residual": rs.group(1) if rs else None,
        "flips": len(flips),
        "locations": len(sites),
        "sites": dict(sites.most_common()),
        "error_entries": int(wrote.group(1)) if wrote else None,
        "unparsed_lines": unparsed,
        "exit_code": rc,
        "log": str(log.relative_to(HERE)),
    }


def write_results(precision, records, outdir):
    r0 = records[0]
    lines = [f"AMG {precision} -- FPChecker branch instability",
             f"(-{r0['opt']} compile+link, serial build, "
             f"problem {r0['problem']}, n={r0['n']}^3)", ""]
    for r in records:
        lines.append(f"{r['flips']} flips @ {r['locations']} loc"
                     f"   (eta={r['eta']})"
                     f"   [iters={r['iterations']}]")
    lines.append("")
    for r in records:
        lines.append(f"--- eta={r['eta']} : {r['flips']} flips @ "
                     f"{r['locations']} loc ---")
        if not r["sites"]:
            lines.append("    (no sites parsed -- check the log)")
        for site, n in r["sites"].items():
            lines.append(f"    {n:8d}  {site}")
        lines.append(f"    iterations: {r['iterations']}   "
                     f"residual: {r['residual']}")
        if r["error_entries"] is not None:
            lines.append(f"    error entries written: {r['error_entries']}")
        if r["unparsed_lines"]:
            lines.append(f"    UNPARSED flip lines: {r['unparsed_lines']}")
        lines.append("")
    (outdir / "summary.txt").write_text("\n".join(lines))
    (outdir / "summary.json").write_text(json.dumps(records, indent=2))


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64", "ld"])
    ap.add_argument("-e", "--eta", nargs="+", default=None,
                    help="override eta for ALL precisions")
    ap.add_argument("--eta-fp32", nargs="+", default=None)
    ap.add_argument("--eta-fp64", nargs="+", default=None)
    ap.add_argument("--problem", type=int, default=2,
                    help="1 = AMG-PCG Laplace (no divergence), "
                         "2 = GMRES modified diagonal (default)")
    ap.add_argument("-n", type=int, default=5,
                    help="grid points per axis (default 5, matching the "
                         "LULESH s=5 census size; 3 also diverges)")
    ap.add_argument("--opt", default="O0", choices=["O0", "O1", "O2", "O3"],
                    help="optimisation level (default O0: -O2 collapses "
                         "function attribution under inlining)")
    ap.add_argument("--maxwarn", type=int, default=100000)
    ap.add_argument("-j", "--jobs", type=int, default=1)
    ap.add_argument("--cc", default="clang",
                    help="plain compiler for the width probe")
    args = ap.parse_args()

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)

    etas = dict(DEFAULT_ETA)
    etas.setdefault("ld", DEFAULT_ETA["fp64"])
    if args.eta:
        etas = {k: list(args.eta) for k in etas}
    if args.eta_fp32:
        etas["fp32"] = list(args.eta_fp32)
    if args.eta_fp64:
        etas["fp64"] = list(args.eta_fp64)

    print(f"AMG / FPChecker   problem {args.problem}  n={args.n}^3  "
          f"-{args.opt}")
    for k in args.precision:
        print(f"  {k} eta: {' '.join(etas[k])}")
    print(f"root:    {FPC_ROOT}")
    print(f"bench:   {BENCH_ROOT}")
    print(f"workdir: {WORK_ROOT}   (make -j{args.jobs})\n")

    if args.problem == 1:
        print("NOTE: problem 1 shows no fp32/fp64 iteration divergence at any "
              "size tested; problem 2 is the census target.\n")

    overall = {}
    for precision in args.precision:
        print(f"===== {precision} =====")
        outdir = RESULT_ROOT / args.opt / precision
        outdir.mkdir(parents=True, exist_ok=True)

        binary = build(precision, args.opt, outdir, args.jobs, args.cc)
        if binary is None:
            continue

        records = [run(binary, precision, eta, args.problem, args.n,
                       args.maxwarn, outdir, args.opt)
                   for eta in etas[precision]]
        write_results(precision, records, outdir)
        overall[precision] = records

        for r in records:
            print(f"  -> {r['flips']} flips @ {r['locations']} loc "
                  f"(eta={r['eta']})  iters={r['iterations']}")
        print()

    if not overall:
        print("No results produced.")
        return 1

    print("===== results =====")
    for precision, records in overall.items():
        for r in records:
            print(f"  {precision:5s} eta={r['eta']:<8s} "
                  f"{r['flips']:7d} flips @ {r['locations']:3d} loc   "
                  f"iters={r['iterations']}")
    print(f"\n{BENCH_NAME}/results/{args.opt}/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())