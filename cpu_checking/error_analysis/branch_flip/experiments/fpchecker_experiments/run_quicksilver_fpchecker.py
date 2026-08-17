#!/usr/bin/env python3
"""
run_quicksilver_fpchecker.py

Build QuickSilver under FPChecker branch-instability instrumentation and run
it, keeping fp32 and fp64 results completely separate.

Place in: branch_flip/experiments/fpchecker_experiments/

    ./run_quicksilver_fpchecker.py                  # both, default eta sweeps
    ./run_quicksilver_fpchecker.py -p fp32          # fp32 only
    ./run_quicksilver_fpchecker.py --opt O2         # -O2 instead of -O0
    ./run_quicksilver_fpchecker.py -n 2000 -N 10    # different problem

Default eta sweeps (scaled to machine epsilon, ~9 orders apart):
    fp32  1e-2  1e-6  1e-10      (eps 1.19e-07)
    fp64  1e-8  1e-14 1e-16      (eps 2.22e-16)

Instrumentation variables (set for BOTH compile and link):
    fp32 -> FPC_INSTRUMENT_ERR_TRACKING=1
    fp64 -> FPC_INSTRUMENT_ERR_TRACKING_FP64=1  (base var explicitly cleared)

Layout produced:
    quicksilver/
      results/O0/{fp32,fp64}/  build.log  run_O0_eta*.log
                               summary.txt  summary.json  build_info.txt
      build/O0/{qs_fp32,qs_fp64}/

CENSUS CONFIG (measured, uninstrumented; identical on gcc 13.3 and
clang 19.1.7):

    -n 4000 -X 10 -Y 10 -Z 10 -x 5 -y 5 -z 5 -I 1 -J 1 -K 1 -N 3

  First fp32-vs-fp64 divergence at cycle 2, so 3 cycles suffice.
  Runtime 0.16 s uninstrumented.

  Particle-count scan at mesh 5^3 (N=6) -- divergence is a sharp function
  of particle count, not a smooth one:

      n     first flip
    1200    none in 6
    2000    none in 6
    2800    none in 6
    3200    cycle 4
    3600    cycle 4
    4000    cycle 2      <-- default: earliest flip, so fewest cycles
    4400    cycle 2

  Other configurations tested that do NOT diverge: 4000 particles on a 4^3
  mesh, 3000 on 4^3, 2000 on 4^3, and anything at 8^3 up to 5000 particles.
  -n 2000 -x 5 -N 10 does diverge, at cycle 8, but costs more cycles.

        cycle  absorb   scatter  fission  num_seg
   fp32    2     4422     4632      456    16740
   fp64    2     4421     4632      455    16738

  fp64 and ld are IDENTICAL in every control-flow tally at every
  configuration tested, so fp64 is oracle-stable -- consistent with NAS,
  LULESH and AMG.

  Divergence is NON-MONOTONIC in mesh size: an 8^3 mesh never flipped at
  any particle count up to 5000, while 4^3 and 5^3 both did.  For Monte
  Carlo this is expected -- whether a particle lands near a facet boundary
  is sampling luck, not a smooth function of resolution.  Below -n 2000
  nothing flipped within 20 cycles at any mesh tested.

  Alternate config worth keeping: -n 5000 -x 4 -N 7 diverges in num_seg
  ONLY (18482 vs 18481) with absorb/scatter/fission identical -- a pure
  facet-crossing-vs-collision boundary flip with no reaction-count
  contamination.

NOTE ON THE RNG: QuickSilver's generator is a 64-bit integer LCG
(MC_RNG_State.hh) and rngSpawn hashes with pseudo_des, also integer.  The
seed stream is BIT-IDENTICAL across fp32/fp64/ld, so particle histories
start from identical draws and any divergence is physics arithmetic, not
sampling.  QS_RNG_NATIVE defaults to 0, which maps the seed to (0,1) in
double and rounds once to qs_real, making that cast the generator's sole
precision boundary.
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

PRECISION_FLAG = {"fp32": "-DQS_FP32", "fp64": "", "ld": "-DQS_LD"}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8, "ld": 16}

FPC_ROOT = Path(os.environ.get("FPC_ROOT", "/usr/workspace/das9/fpchecker_bf"))
FPC_INSTALL = Path(os.environ.get("FPC_INSTALL", FPC_ROOT / "install"))
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT",
    FPC_ROOT / "cpu_checking/error_analysis/branch_flip/benchmarks/quicksilver"))
CONDA_LIB = os.environ.get("CONDA_PREFIX", "") and \
    os.path.join(os.environ["CONDA_PREFIX"], "lib")

HERE = Path(__file__).resolve().parent
BENCH_NAME = "quicksilver"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

FLIP_RE = re.compile(r"Unstable branch", re.I)
SITE_RE = re.compile(
    r"at\s+\S*?([A-Za-z0-9_.+-]+\.(?:cc|cpp|cxx|hh|h|hpp|tcc)):(\d+)"
    r"(?:\s+in\s+([A-Za-z0-9_$.]+))?")
# tally rows: cycle start source rr split absorb scatter fission produce
#             collisn escape census num_seg scalar_flux ...
TALLY_RE = re.compile(r"^\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+"
                      r"(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+\d+\s+(\d+)")

_DEMANGLE_CACHE = {}


def demangle(sym):
    """FPChecker names the compiled TU as the file, not the file the branch
    came from, so an STL header comparison shows up under a QuickSilver
    filename.  The mangled symbol is the only way to tell them apart."""
    if not sym:
        return ""
    if sym not in _DEMANGLE_CACHE:
        try:
            out = subprocess.run(["c++filt", sym], stdout=subprocess.PIPE,
                                 text=True, timeout=10).stdout.strip()
        except Exception:
            out = sym
        out = re.sub(r"\s*\(.*\)\s*$", "", out)
        _DEMANGLE_CACHE[sym] = out or sym
    return _DEMANGLE_CACHE[sym]


FOREIGN_RE = re.compile(r"\b(std|__gnu_cxx|__cxxabiv1)::")


def is_foreign(func):
    return bool(FOREIGN_RE.search(func))


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
       fp64 -> FPC_INSTRUMENT_ERR_TRACKING_FP64=1 ONLY (base var cleared).
       Both must be present at compile AND link."""
    env = base_env()
    env.pop("FPC_INSTRUMENT_ERR_TRACKING", None)
    env.pop("FPC_INSTRUMENT_ERR_TRACKING_FP64", None)
    if precision == "fp64":
        env["FPC_INSTRUMENT_ERR_TRACKING_FP64"] = "1"
    else:
        env["FPC_INSTRUMENT_ERR_TRACKING"] = "1"
    return env


def check_width(tree, precision, cxx="clang++"):
    """sizeof(qs_real) probe.  A clean build does not prove the -DQS_FP32 /
    -DQS_LD flag reached the header."""
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


def build(precision, opt, outdir, jobs=1, cxx="clang++"):
    src = BENCH_ROOT / f"qs_{precision}"
    dst = BUILD_ROOT / opt / f"qs_{precision}"
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

    env = make_env(precision)
    ivar = ("FPC_INSTRUMENT_ERR_TRACKING_FP64" if precision == "fp64"
            else "FPC_INSTRUMENT_ERR_TRACKING")
    print(f"  instrumentation var: {ivar}=1")

    # -include cstdint: upstream QuickSilver omits <cstdint> and does not
    # compile on modern libstdc++ at all (unrelated to precision).
    # -DUSE_MPI=0 is not a QuickSilver flag; the serial path is selected by
    # simply not defining HAVE_MPI, which the stock Makefile already does.
    cxxf = (f"-std=c++11 -g -{opt} -include cstdint "
            f"{PRECISION_FLAG[precision]}").strip()
    ldf = f"-g -{opt} -Wl,--allow-multiple-definition"
    cmd = ["make", f"-j{jobs}", f"CXX={fpcxx}",
           f"CXXFLAGS={cxxf}", "CPPFLAGS=", f"LDFLAGS={ldf}"]

    print(f"  building ({precision}, -{opt}, -j{jobs})")
    rc, _ = sh(cmd, cwd=dst, env=env, log=outdir / "build.log")

    binary = dst / "qs"
    if not binary.exists():
        print(f"  BUILD FAILED -- see {outdir/'build.log'}")
        for line in (outdir / "build.log").read_text().splitlines():
            if "error" in line.lower():
                print("   ", line[:150])
                break
        return None

    _, nm_out = sh(["nm", str(binary)])
    nsym = sum(1 for l in nm_out.splitlines() if "_FPC" in l)
    print(f"  _FPC symbol count = {nsym}")

    width = check_width(dst, precision, cxx)
    ok_w = (width == EXPECTED_WIDTH[precision])
    print(f"  sizeof(qs_real)    = {width} "
          f"({'ok' if ok_w else 'EXPECTED ' + str(EXPECTED_WIDTH[precision])})")

    (outdir / "build_info.txt").write_text(
        f"precision   = {precision}\n"
        f"opt         = -{opt}   (compile AND link)\n"
        f"jobs        = -j{jobs}\n"
        f"CXX         = {fpcxx}\n"
        f"CXXFLAGS    = {cxxf}\n"
        f"LDFLAGS     = {ldf}\n"
        f"instr_var   = {ivar}=1\n"
        f"fpc_symbols = {nsym}\n"
        f"qs_real     = {width} bytes (expected {EXPECTED_WIDTH[precision]})\n"
        f"mpi         = serial (QuickSilver's own stubs in utilsMpi.cc)\n")

    if nsym == 0:
        print("  *** UNINSTRUMENTED BINARY -- env vars did not reach "
              "compile and/or link. Aborting this precision.")
        return None
    if not ok_w:
        print("  *** WRONG qs_real WIDTH -- precision flag did not take. "
              "Aborting this precision.")
        return None
    return binary


def run(binary, precision, eta, opts, maxwarn, outdir, opt):
    env = base_env()
    # FPC_STABILITY_ETA_REL, not FPC_STABILITY_ETA.
    env["FPC_STABILITY_ETA_REL"] = eta
    env["FPC_STABILITY_MAX_WARNINGS"] = str(maxwarn)

    log = outdir / f"run_{opt}_eta{eta}.log"
    print(f"  running eta={eta} -> {log.relative_to(HERE)}")
    args = ["-n", str(opts["n"]),
            "-X", "10", "-Y", "10", "-Z", "10",
            "-x", str(opts["x"]), "-y", str(opts["x"]), "-z", str(opts["x"]),
            "-I", "1", "-J", "1", "-K", "1",
            "-N", str(opts["N"])]
    rc, out = sh(["stdbuf", "-i0", "-o0", "-e0", str(binary)] + args,
                 cwd=binary.parent, env=env, log=log)

    flips = [l for l in out.splitlines() if FLIP_RE.search(l)]
    sites, foreign = Counter(), Counter()
    unparsed = 0
    for l in flips:
        m = SITE_RE.search(l)
        if not m:
            unparsed += 1
            continue
        fn = demangle(m.group(3))
        key = f"{m.group(1)}:{m.group(2)}"
        if fn:
            key += f"  [{fn}]"
        sites[key] += 1
        if is_foreign(fn):
            foreign[key] += 1

    # Control-flow observables: integer tallies per cycle.
    tallies = []
    for l in out.splitlines():
        m = TALLY_RE.match(l)
        if m:
            tallies.append({"cycle": int(m.group(1)),
                            "absorb": int(m.group(3)),
                            "scatter": int(m.group(4)),
                            "fission": int(m.group(5)),
                            "num_seg": int(m.group(6))})

    wrote = re.search(r"Successfully wrote (\d+) error entries", out)

    return {
        "precision": precision, "eta": eta, "opt": opt,
        "n": opts["n"], "mesh": opts["x"], "cycles": opts["N"],
        "flips": len(flips),
        "locations": len(sites),
        "sites": dict(sites.most_common()),
        "foreign_flips": sum(foreign.values()),
        "tallies": tallies,
        "error_entries": int(wrote.group(1)) if wrote else None,
        "unparsed_lines": unparsed,
        "exit_code": rc,
        "log": str(log.relative_to(HERE)),
    }


def write_results(precision, records, outdir):
    r0 = records[0]
    lines = [f"QuickSilver {precision} -- FPChecker branch instability",
             f"(-{r0['opt']} compile+link, serial build, "
             f"n={r0['n']} mesh={r0['mesh']}^3 N={r0['cycles']})", ""]
    for r in records:
        own = r["flips"] - r["foreign_flips"]
        lines.append(f"{r['flips']} flips @ {r['locations']} loc"
                     f"   (eta={r['eta']})"
                     f"   [in-TU {own}, STL {r['foreign_flips']}]")
    lines.append("")
    lines.append("NOTE: FPChecker names the compiled TU as the file, not the")
    lines.append("      file the branch came from.  Sites marked [std::...]")
    lines.append("      are STL headers reported under a QuickSilver name.")
    lines.append("")
    if r0["tallies"]:
        lines.append("control-flow tallies (should match the uninstrumented run):")
        lines.append("  cycle   absorb  scatter  fission  num_seg")
        for t in r0["tallies"]:
            lines.append(f"  {t['cycle']:5d} {t['absorb']:8d} {t['scatter']:8d}"
                         f" {t['fission']:8d} {t['num_seg']:8d}")
        lines.append("")
    for r in records:
        lines.append(f"--- eta={r['eta']} : {r['flips']} flips @ "
                     f"{r['locations']} loc ---")
        if not r["sites"]:
            lines.append("    (no sites parsed -- check the log)")
        for site, n in r["sites"].items():
            lines.append(f"    {n:8d}  {site}")
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
    ap.add_argument("-e", "--eta", nargs="+", default=None)
    ap.add_argument("--eta-fp32", nargs="+", default=None)
    ap.add_argument("--eta-fp64", nargs="+", default=None)
    ap.add_argument("-n", type=int, default=4000, help="particles (default 4000)")
    ap.add_argument("-x", type=int, default=5, help="mesh cells per axis")
    ap.add_argument("-N", type=int, default=3,
                    help="cycles (default 3; first divergence is at cycle 2)")
    ap.add_argument("--opt", default="O0", choices=["O0", "O1", "O2", "O3"])
    ap.add_argument("--maxwarn", type=int, default=100000)
    ap.add_argument("-j", "--jobs", type=int, default=1)
    ap.add_argument("--cxx", default="clang++",
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

    opts = {"n": args.n, "x": args.x, "N": args.N}
    print(f"QuickSilver / FPChecker   n={args.n} mesh={args.x}^3 "
          f"N={args.N}  -{args.opt}")
    for k in args.precision:
        print(f"  {k} eta: {' '.join(etas[k])}")
    print(f"bench:   {BENCH_ROOT}")
    print(f"workdir: {WORK_ROOT}   (make -j{args.jobs})\n")

    overall = {}
    for precision in args.precision:
        print(f"===== {precision} =====")
        outdir = RESULT_ROOT / args.opt / precision
        outdir.mkdir(parents=True, exist_ok=True)

        binary = build(precision, args.opt, outdir, args.jobs, args.cxx)
        if binary is None:
            continue

        records = [run(binary, precision, eta, opts, args.maxwarn,
                       outdir, args.opt) for eta in etas[precision]]
        write_results(precision, records, outdir)
        overall[precision] = records

        for r in records:
            own = r["flips"] - r["foreign_flips"]
            print(f"  -> {r['flips']} flips @ {r['locations']} loc "
                  f"(eta={r['eta']})  [in-TU {own}, STL {r['foreign_flips']}]")
        print()

    if not overall:
        print("No results produced.")
        return 1

    print("===== results =====")
    for precision, records in overall.items():
        for r in records:
            print(f"  {precision:5s} eta={r['eta']:<8s} "
                  f"{r['flips']:7d} flips @ {r['locations']:3d} loc")

    # Cross-precision tally check: divergence should appear at cycle 2.
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
                print("  none -- tallies identical; try more cycles or "
                      "a different mesh (divergence is non-monotonic in mesh)")

    print(f"\n{BENCH_NAME}/results/{args.opt}/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
