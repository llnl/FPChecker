#!/usr/bin/env python3
"""
run_lulesh_fpchecker.py

Build LULESH under FPChecker branch-instability instrumentation and run it,
keeping fp32 and fp64 results completely separate.

Place in: branch_flip/experiments/fpchecker_experiments/

    ./run_lulesh_fpchecker.py                        # both, default eta sweeps
    ./run_lulesh_fpchecker.py -p fp32                # fp32 only
    ./run_lulesh_fpchecker.py --eta-fp64 1e-12       # override one precision
    ./run_lulesh_fpchecker.py -e 1e-6                # same eta for both

Default eta sweeps (scaled to machine epsilon, which differs by ~9 orders):
    fp32  1e-2  1e-6  1e-10      (eps 1.19e-07)
    fp64  1e-8  1e-14 1e-16      (eps 2.22e-16)
    ./run_lulesh_fpchecker.py -s 10 -i 50            # the census size
    ./run_lulesh_fpchecker.py --opt O2               # event counts only, see below

Instrumentation variables (set for BOTH compile and link):
    fp32 -> FPC_INSTRUMENT_ERR_TRACKING=1
    fp64 -> FPC_INSTRUMENT_ERR_TRACKING_FP64=1  (base var explicitly cleared)

Layout produced (one subtree per optimisation level, so -O0 and -O2 runs
never overwrite each other):
    lulesh/
      results/
        O0/  fp32/  build.log  run_O0_eta1e-2.log ...  summary.txt  summary.json
             fp64/  ...
        O2/  fp32/  ...
             fp64/  ...
      build/
        O0/lulesh_fp32/  O0/lulesh_fp64/  O2/lulesh_fp32/  O2/lulesh_fp64/

WHY -O0 IS THE DEFAULT
----------------------
FPChecker reads DILocation.line without walking getInlinedAt(), so from -O1
upward an inlined branch is attributed to the line that inlined it, not the
line it came from.  That is the origin of the known lulesh.cc:263 vs
stl_algobase.h:263 collision -- same line number, different file.  At -O2 the
site count is inflated by that misattribution, and the sites cannot be joined
against the brtrace census, which is generated at -O0.  brtrace does not share
the failure mode: it records locString at instrumentation time, before any
inlining, so its attribution is stable across levels.

So: -O0 for anything that will be scored per site.  -O2 remains available for
comparing EVENT counts against the earlier sweep, but its site lists are not
adjudicable.  Adjudicate sites by file+line, never line alone.
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
# Defaults -- override on the command line or via environment.
# The tree moved fpchecker_new -> fpchecker_bf, so verify the install path.
# --------------------------------------------------------------------
# Eta sweep per precision.  Eta is a window classifier, not a threshold:
# larger eta flags more.  The useful range tracks machine epsilon, which
# differs by ~9 orders between the two precisions, so one shared list would
# be meaningless for both.
#   fp32 eps = 1.19e-07     fp64 eps = 2.22e-16
DEFAULT_ETA = {
    "fp32": ["1e-2", "1e-6", "1e-10"],
    "fp64": ["1e-8", "1e-14", "1e-16"],
}

FPC_ROOT = Path(os.environ.get("FPC_ROOT", "/usr/workspace/das9/fpchecker_bf"))
FPC_INSTALL = Path(os.environ.get("FPC_INSTALL", FPC_ROOT / "install"))
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT",
    FPC_ROOT / "cpu_checking/error_analysis/branch_flip/benchmarks/lulesh"))
CONDA_LIB = os.environ.get("CONDA_PREFIX", "") and \
    os.path.join(os.environ["CONDA_PREFIX"], "lib")

HERE = Path(__file__).resolve().parent
# Everything for this benchmark lives under <script dir>/lulesh/ so that
# other benchmarks (quicksilver, amg) can sit alongside it later.
BENCH_NAME = "lulesh"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

# FPChecker emits one line per unstable branch; capture file:line from it.
FLIP_RE = re.compile(r"Unstable branch", re.I)
# FPCHECKER: Unstable branch at /abs/path/lulesh.cc:263 in _ZSt3maxIdERKT_S2_S2_: ...
SITE_RE = re.compile(
    r"at\s+\S*?([A-Za-z0-9_.+-]+\.(?:cc|cpp|cxx|h|hh|hpp|tcc)):(\d+)"
    r"(?:\s+in\s+([A-Za-z0-9_$.]+))?")

_DEMANGLE_CACHE = {}


def demangle(sym):
    """FPChecker reports the *compiled TU* as the file, not the file the
    branch really came from.  std::max lives at stl_algobase.h:263 but is
    reported as lulesh.cc:263 -- correct line, wrong file.  The mangled
    function name is the only way to tell them apart, so key sites on it."""
    if not sym:
        return ""
    if sym not in _DEMANGLE_CACHE:
        try:
            out = subprocess.run(["c++filt", sym], stdout=subprocess.PIPE,
                                 text=True, timeout=10).stdout.strip()
        except Exception:
            out = sym
        # trim template noise to something table-friendly
        out = re.sub(r"\s*\(.*\)\s*$", "", out)
        _DEMANGLE_CACHE[sym] = out or sym
    return _DEMANGLE_CACHE[sym]


# The demangled name carries a return type, e.g.
#   "float const& std::max<float>"
# so a startswith() test on the namespace misses.  Match the qualified name
# wherever it appears.
FOREIGN_RE = re.compile(r"\b(std|__gnu_cxx|__cxxabiv1)::")


def is_foreign(func):
    """True if the branch really belongs to a system/STL header rather than
    the TU FPChecker named."""
    return bool(FOREIGN_RE.search(func))


def sh(cmd, cwd=None, env=None, log=None):
    """Run a command, tee output to `log`, return (rc, text)."""
    p = subprocess.run(cmd, cwd=cwd, env=env, shell=isinstance(cmd, str),
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True, errors="replace")
    if log:
        log.write_text(p.stdout)
    return p.returncode, p.stdout


def make_env(precision):
    """Instrumentation vars must be present for BOTH compile and link.
    Missing them at link produces a binary that builds and prints normally
    but contains zero _FPC symbols and is silent -- the most common
    failure mode.

    fp32 -> FPC_INSTRUMENT_ERR_TRACKING=1        (default pass)
    fp64 -> FPC_INSTRUMENT_ERR_TRACKING_FP64=1   ONLY; the base variable is
            explicitly cleared so the two passes cannot both instrument and
            double-count."""
    env = os.environ.copy()
    if CONDA_LIB:
        env["LD_LIBRARY_PATH"] = CONDA_LIB + ":" + env.get("LD_LIBRARY_PATH", "")
    env.pop("FPC_INSTRUMENT_ERR_TRACKING", None)
    env.pop("FPC_INSTRUMENT_ERR_TRACKING_FP64", None)
    if precision == "fp64":
        env["FPC_INSTRUMENT_ERR_TRACKING_FP64"] = "1"
    else:
        env["FPC_INSTRUMENT_ERR_TRACKING"] = "1"
    return env


def build(precision, opt, outdir, jobs=1):
    src = BENCH_ROOT / f"lulesh_{precision}"
    dst = BUILD_ROOT / opt / f"lulesh_{precision}"
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
        print("         set FPC_INSTALL=/path/to/install")
        return None

    flag = "-DLULESH_FP32" if precision == "fp32" else ""
    env = make_env(precision)
    ivar = ("FPC_INSTRUMENT_ERR_TRACKING_FP64" if precision == "fp64"
            else "FPC_INSTRUMENT_ERR_TRACKING")
    print(f"  instrumentation var: {ivar}=1")

    # Two-step compile-then-link (the Makefile already does this).
    # -Wl,--allow-multiple-definition works around the duplicate
    # _FPC_STABILITY_* symbols emitted per TU from Runtime_error.h.
    # Serial build by default.  Parallel compilation (-j8) was observed to
    # give different instrumentation between builds of identical source:
    # flip counts drifted across rebuilds while back-to-back rebuilds in
    # quick succession matched, and "error entries written" moved too
    # (fp64 857 -> 808).  LULESH has no OpenMP here (CXXFLAGS omits
    # -fopenmp, verified: zero GOMP symbols, "Num processors: 1"), so the
    # run itself is single-threaded and the only parallelism was the
    # compile.  -j1 removes that variable.  The build takes seconds.
    # One optimisation level, applied to BOTH compile and link.  The
    # benchmark trees carry their own CXXFLAGS/LDFLAGS in their Makefiles;
    # command-line assignments override those, so these are authoritative
    # and the tree settings are irrelevant.
    cxx = f"{fpcxx} -DUSE_MPI=0 {flag}".strip()
    cxxflags = f"-g -{opt} -I. -std=c++11"
    ldflags = f"-g -{opt} -Wl,--allow-multiple-definition"
    cmd = [
        "make", f"-j{jobs}",
        f"CXX={cxx}",
        f"CXXFLAGS={cxxflags}",
        f"LDFLAGS={ldflags}",
    ]
    print(f"  building ({precision}, -{opt}, -j{jobs})")
    rc, _ = sh(cmd, cwd=dst, env=env, log=outdir / "build.log")

    binary = dst / "lulesh2.0"
    if rc != 0 or not binary.exists():
        print(f"  BUILD FAILED -- see {outdir/'build.log'}")
        for line in (outdir / "build.log").read_text().splitlines():
            if "error" in line.lower():
                print("   ", line[:140])
                break
        return None

    # Authoritative check: a clean build proves nothing about instrumentation.
    _, nm_out = sh(["nm", str(binary)])
    nsym = sum(1 for l in nm_out.splitlines() if "_FPC" in l)
    print(f"  _FPC symbol count = {nsym}")
    # Full provenance so a result set is self-describing.
    (outdir / "build_info.txt").write_text(
        f"precision   = {precision}\n"
        f"opt         = -{opt}   (compile AND link)\n"
        f"jobs        = -j{jobs}\n"
        f"CXX         = {cxx}\n"
        f"CXXFLAGS    = {cxxflags}\n"
        f"LDFLAGS     = {ldflags}\n"
        f"instr_var   = {ivar}=1\n"
        f"fpc_symbols = {nsym}\n"
        f"openmp      = disabled (no -fopenmp; LULESH guards on _OPENMP)\n")
    if nsym == 0:
        print("  *** UNINSTRUMENTED BINARY -- env vars did not reach "
              "compile and/or link. Aborting this precision.")
        return None
    return binary


def run(binary, precision, eta, size, iters, maxwarn, outdir, opt):
    env = os.environ.copy()
    if CONDA_LIB:
        env["LD_LIBRARY_PATH"] = CONDA_LIB + ":" + env.get("LD_LIBRARY_PATH", "")
    # It is FPC_STABILITY_ETA_REL, not FPC_STABILITY_ETA.  Setting the
    # wrong one leaves every run on the default and makes an eta sweep
    # silently produce identical results.
    env["FPC_STABILITY_ETA_REL"] = eta
    env["FPC_STABILITY_MAX_WARNINGS"] = str(maxwarn)

    log = outdir / f"run_{opt}_eta{eta}.log"
    print(f"  running eta={eta} -> {log.relative_to(HERE)}")
    rc, out = sh(["stdbuf", "-i0", "-o0", "-e0", str(binary),
                  "-s", str(size), "-i", str(iters), "-p"],
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

    wrote = re.search(r"Successfully wrote (\d+) error entries", out)

    return {
        "precision": precision,
        "eta": eta,
        "opt": opt,
        "foreign_sites": dict(foreign),
        "foreign_flips": sum(foreign.values()),
        "unparsed_lines": unparsed,
        "size": size,
        "iterations": iters,
        "flips": len(flips),
        "locations": len(sites),
        "sites": dict(sites.most_common()),
        "error_entries": int(wrote.group(1)) if wrote else None,
        "exit_code": rc,
        "log": str(log.relative_to(HERE)),
    }


def write_results(precision, records, outdir):
    lines = [f"LULESH {precision} -- FPChecker branch instability",
             f"(-{records[0]['opt']} compile+link, serial build, "
             f"s={records[0]['size']}, i={records[0]['iterations']})", ""]
    for r in records:
        own = r["flips"] - r["foreign_flips"]
        lines.append(f"{r['flips']} flips @ {r['locations']} loc"
                     f"   (eta={r['eta']})"
                     f"   [in-TU {own}, STL {r['foreign_flips']}]")
    lines.append("")
    lines.append("NOTE: FPChecker names the compiled TU as the file, not the")
    lines.append("      file the branch came from.  Sites marked [std::...]")
    lines.append("      are STL headers (e.g. std::max is stl_algobase.h:263,")
    lines.append("      reported as lulesh.cc:263 -- right line, wrong file).")
    lines.append("")
    for r in records:
        lines.append(f"--- eta={r['eta']} : {r['flips']} flips @ "
                     f"{r['locations']} loc ---")
        if not r["sites"]:
            lines.append("    (no sites parsed -- check the log; if the run "
                         "produced output, the flip-line pattern may differ)")
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
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("-e", "--eta", nargs="+", default=None,
                    help="override eta for ALL precisions "
                         "(default: fp32 %s, fp64 %s)"
                         % (",".join(DEFAULT_ETA["fp32"]),
                            ",".join(DEFAULT_ETA["fp64"])))
    ap.add_argument("--eta-fp32", nargs="+", default=None,
                    help="override eta for fp32 only")
    ap.add_argument("--eta-fp64", nargs="+", default=None,
                    help="override eta for fp64 only")
    ap.add_argument("-s", "--size", type=int, default=5)
    ap.add_argument("-i", "--iter", type=int, default=20)
    ap.add_argument("--opt", default="O0", choices=["O0", "O1", "O2", "O3"],
                    help="optimisation level (default O0). Site attribution "
                         "is only reliable at -O0: FPChecker reads "
                         "DILocation.line without walking getInlinedAt(), so "
                         "from -O1 up an inlined branch is reported against "
                         "the inlining line. Use -O2 for event-count "
                         "comparisons only, never for site-level results.")
    ap.add_argument("--maxwarn", type=int, default=100000)
    ap.add_argument("-j", "--jobs", type=int, default=1,
                    help="make parallelism (default 1; parallel builds were "
                         "observed to change instrumentation between builds)")
    args = ap.parse_args()

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)

    etas = dict(DEFAULT_ETA)
    if args.eta:
        etas = {k: list(args.eta) for k in etas}
    if args.eta_fp32:
        etas["fp32"] = list(args.eta_fp32)
    if args.eta_fp64:
        etas["fp64"] = list(args.eta_fp64)

    print(f"LULESH / FPChecker   s={args.size} i={args.iter} -{args.opt}")
    for k in args.precision:
        print(f"  {k} eta: {' '.join(etas[k])}")
    print(f"root:    {FPC_ROOT}")
    print(f"install: {FPC_INSTALL}")
    print(f"bench:   {BENCH_ROOT}\n")

    overall = {}
    for precision in args.precision:
        print(f"===== {precision} =====")
        outdir = RESULT_ROOT / args.opt / precision
        outdir.mkdir(parents=True, exist_ok=True)

        binary = build(precision, args.opt, outdir, args.jobs)
        if binary is None:
            continue

        records = [run(binary, precision, eta, args.size, args.iter,
                       args.maxwarn, outdir, args.opt)
                   for eta in etas[precision]]
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
                  f"{r['flips']:6d} flips @ {r['locations']} loc")
    print(f"\n{BENCH_NAME}/results/{args.opt}/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())