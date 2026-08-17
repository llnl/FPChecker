#!/usr/bin/env python3
"""
run_nas_fpchecker.py

Build the NAS benchmarks under FPChecker branch-instability instrumentation
and run them, keeping fp32 and fp64 results completely separate.

Place in: branch_flip/experiments/fpchecker_experiments/

    ./run_nas_fpchecker.py                     # all 7, both precisions, -O0
    ./run_nas_fpchecker.py -b lu sp bt ep      # the ones with non-zero GT
    ./run_nas_fpchecker.py -p fp32
    ./run_nas_fpchecker.py --opt O2
    ./run_nas_fpchecker.py --eta-fp64 1e-12

Default eta sweeps (scaled to machine epsilon, ~9 orders apart):
    fp32  1e-2  1e-6  1e-10      (eps 1.19e-07)
    fp64  1e-8  1e-14 1e-16      (eps 2.22e-16)

Instrumentation variables (set for BOTH compile and link):
    fp32 -> FPC_INSTRUMENT_ERR_TRACKING=1
    fp64 -> FPC_INSTRUMENT_ERR_TRACKING_FP64=1  (base var explicitly cleared)

Layout produced:
    nas/
      results/<bench>/<opt>/<precision>/
          build.log  build_info.txt  run_<opt>_eta<e>.log
          summary.txt  summary.json
      build/<bench>/<opt>/<bench>_<precision>/

DIFFERENCES FROM THE LULESH / AMG / QUICKSILVER HARNESSES
--------------------------------------------------------
  * NAS is C, so the frontend is clang-fpchecker, not clang++-fpchecker,
    and there is no name mangling to undo.
  * SEVEN benchmarks. Each is built and run independently; one failing does
    not stop the others.
  * MIXED PROVENANCE -- the trees do not all select precision the same way:
      BT, CG, LU, SP   compile-time switch   -DNAS_FP32 / (none)
      EP               WORKING_T macros      -DWORKING_T=float ...
      IS, MG           baked into the source, no define
    Getting this wrong does not fail the build; it silently produces a
    binary at the wrong precision. The sizeof probe below is what catches
    that, and it aborts rather than reporting plausible numbers.
  * NO RUN ARGUMENTS. Problem size is compiled in via npbparams.h. Class is
    S everywhere except CG, which is W.
  * Compiled with a single direct clang-fpchecker invocation rather than
    through each tree's Makefile, because those Makefiles differ in more
    than precision (LU fp32/fp64 use -Wall -Wno-unused where its ld sibling
    uses -w; EP uses -O2 where BT uses -O3). One flag set for all variants
    keeps the comparison honest.

-O0 IS THE DEFAULT HERE, unlike the LULESH harness. At -O2 FPChecker reads
DILocation.line without walking getInlinedAt(), so inlined sites are
attributed to the inlining line -- the origin of the lulesh.cc:263 vs
stl_algobase.h:263 collision. On LULESH that also cost 40-72% of sites.

GROUND TRUTH (brtrace, fp32 vs fp64, -O0, site level)
    LU=3  SP=2  BT=2  EP=1  CG=0  MG=0  IS=0   sites
    LU=11 SP=10 BT=10 EP=1                     events
Of those sites, all but ep.c:207 are verify() epsilon gates -- they flip
because fp32 failed verification, not because a computation branch was
unstable. ep.c:207 (the Marsaglia acceptance test) is the only genuine
algorithmic branch in the NAS ground truth. Worth remembering when a tool
"detects" BT/LU/SP: it may only be detecting that verification failed.
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

DEFAULT_ETA = {
    "fp32": ["1e-2", "1e-6", "1e-10"],
    "fp64": ["1e-8", "1e-14", "1e-16"],
}

BENCHES = ["bt", "cg", "ep", "is", "lu", "mg", "sp"]

SRCS = {
    "bt": ["bt.c"], "cg": ["cg.c"], "lu": ["lu.c"], "mg": ["mg.c"],
    "sp": ["sp.c"],
    "ep": ["ep.c", "c_randdp.c", "c_print_results.c", "c_timers.c",
           "wtime.c"],
    "is": ["is.c", "c_print_results.c", "c_timers.c", "wtime.c"],
}

# benchmark -> precision -> extra defines (see the header note)
_SWITCH = {"fp32": "-DNAS_FP32", "fp64": ""}
_NONE = {"fp32": "", "fp64": ""}
DEFINES = {
    "bt": _SWITCH, "cg": _SWITCH, "lu": _SWITCH, "sp": _SWITCH,
    "is": _NONE, "mg": _NONE,
    "ep": {"fp32": "-DWORKING_T=float -DWORKING_T_IS_FLOAT=1",
           "fp64": "-DWORKING_T=double"},
}

# The type whose width the sizeof probe checks, per benchmark.
PROBE_TYPE = {
    "bt": "nas_real", "cg": "nas_real", "lu": "nas_real", "sp": "nas_real",
    "ep": "working_t",
    # IS/MG bake precision in; there is no switchable typedef to probe, so
    # they fall back to the objdump check only.
    "is": None, "mg": None,
}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8}

# Instruction mnemonics revealing the arithmetic precision actually emitted.
PRECISION_MARKERS = [("single", r"\b(mulss|addss|subss|divss)\b"),
                     ("double", r"\b(mulsd|addsd|subsd|divsd)\b")]

FPC_ROOT = Path(os.environ.get("FPC_ROOT", "/usr/workspace/das9/fpchecker_bf"))
FPC_INSTALL = Path(os.environ.get("FPC_INSTALL", FPC_ROOT / "install"))
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT",
    FPC_ROOT / "cpu_checking/error_analysis/branch_flip/benchmarks/nas"))
CONDA_LIB = os.environ.get("CONDA_PREFIX", "") and \
    os.path.join(os.environ["CONDA_PREFIX"], "lib")

HERE = Path(__file__).resolve().parent
BENCH_NAME = "nas"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

FLIP_RE = re.compile(r"Unstable branch", re.I)
SITE_RE = re.compile(
    r"at\s+\S*?([A-Za-z0-9_.+-]+\.(?:c|h)):(\d+)"
    r"(?:\s+in\s+([A-Za-z0-9_$.]+))?")
VERIFY_RE = re.compile(r"\b(SUCCESSFUL|UNSUCCESSFUL)\b", re.I)


def sh(cmd, cwd=None, env=None, log=None, timeout=None):
    try:
        p = subprocess.run(cmd, cwd=cwd, env=env,
                           shell=isinstance(cmd, str),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True, errors="replace", timeout=timeout)
        rc, out = p.returncode, p.stdout
    except subprocess.TimeoutExpired as e:
        rc, out = 124, (e.output or "") + "\n[TIMEOUT]"
    if log:
        Path(log).write_text(out)
    return rc, out


def base_env():
    env = os.environ.copy()
    if CONDA_LIB:
        env["LD_LIBRARY_PATH"] = CONDA_LIB + ":" + env.get("LD_LIBRARY_PATH", "")
    return env


def make_env(precision):
    """fp32 -> FPC_INSTRUMENT_ERR_TRACKING=1
       fp64 -> FPC_INSTRUMENT_ERR_TRACKING_FP64=1 ONLY; the base variable is
               cleared so the two passes cannot both instrument and
               double-count. Both must be set for compile AND link -- missing
               them at link yields a binary that builds and runs but has zero
               _FPC symbols and is silent."""
    env = base_env()
    env.pop("FPC_INSTRUMENT_ERR_TRACKING", None)
    env.pop("FPC_INSTRUMENT_ERR_TRACKING_FP64", None)
    if precision == "fp64":
        env["FPC_INSTRUMENT_ERR_TRACKING_FP64"] = "1"
    else:
        env["FPC_INSTRUMENT_ERR_TRACKING"] = "1"
    return env


def check_width(tree, bench, precision, cc="clang"):
    """sizeof(<precision type>) probe.

    A clean build does NOT prove the precision flag reached the source: the
    trees select precision three different ways, and a missed define
    produces a working binary at the wrong precision.
    """
    typ = PROBE_TYPE[bench]
    if not typ:
        return None
    probe = WORK_ROOT / "_probe.c"
    hdr = ('#include "NAS_Precision.h"' if typ == "nas_real"
           else "typedef WORKING_T working_t;")
    probe.write_text('#include <stdio.h>\n%s\n'
                     'int main(void){ printf("%%zu\\n", sizeof(%s)); '
                     'return 0; }\n' % (hdr, typ))
    out_bin = WORK_ROOT / "_probe"
    cmd = [cc, "-O0", "-std=c99", "-w", "-I%s" % tree]
    cmd += [d for d in DEFINES[bench][precision].split() if d]
    cmd += [str(probe), "-o", str(out_bin), "-lm"]
    rc, _ = sh(cmd, env=base_env())
    if rc != 0:
        return None
    rc, out = sh([str(out_bin)], env=base_env())
    try:
        return int(out.strip())
    except ValueError:
        return None


def arith_profile(binary):
    if not shutil.which("objdump"):
        return None
    _, out = sh("objdump -d %s" % binary, env=base_env())
    return tuple(len(re.findall(p, out)) for _, p in PRECISION_MARKERS)


def build(bench, precision, opt, outdir, jobs=1, cc=None):
    src = BENCH_ROOT / bench / ("%s_%s" % (bench, precision))
    dst = BUILD_ROOT / bench / opt / ("%s_%s" % (bench, precision))
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print("  FATAL: no source tree at %s" % src)
        return None
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    # Drop prebuilt binaries and stale brtrace side tables that ship in the
    # hand-converted trees.
    for pat in ("*.o", "*.brsites", "*.brmods", "*.brx"):
        for j in dst.glob(pat):
            j.unlink()
    for j in dst.iterdir():
        if j.is_file() and os.access(j, os.X_OK) and j.suffix == "":
            j.unlink()

    fpcc = FPC_INSTALL / "bin" / "clang-fpchecker"      # C, not C++
    if not fpcc.exists():
        print("  FATAL: no clang-fpchecker at %s" % fpcc)
        return None

    env = make_env(precision)
    ivar = ("FPC_INSTRUMENT_ERR_TRACKING_FP64" if precision == "fp64"
            else "FPC_INSTRUMENT_ERR_TRACKING")
    defines = DEFINES[bench][precision]
    cflags = "-g -%s -I. -std=c99 -w %s" % (opt, defines)
    ldflags = "-lm -Wl,--allow-multiple-definition"
    print("  %-4s %-5s  %s=1  %s" % (bench, precision, ivar,
                                     defines or "(no define)"))

    # TWO-STEP COMPILE-THEN-LINK IS MANDATORY.
    #
    # A single clang-fpchecker invocation that compiles and links in one go
    # produces a binary with ZERO _FPC symbols and no instrumentation
    # banner -- it builds and runs cleanly and is simply silent. Verified on
    # EP fp32: one-shot gave 0 symbols, per-TU compile then link gave 218,
    # with "#FPCHECKER: Instrumented N @ <file>" printed for each TU.
    #
    # The instrumentation vars must be exported for BOTH steps; missing them
    # at link is the other way to get an uninstrumented binary.
    objs, log_parts, rc = [], [], 0
    for srcf in SRCS[bench]:
        obj = srcf[:-2] + ".o"
        c = re.sub(r"\s+", " ",
                   "%s %s -c %s -o %s" % (fpcc, cflags, srcf, obj))
        rc, out = sh(c, cwd=str(dst), env=env)
        log_parts.append("$ %s\n%s" % (c, out))
        if rc != 0:
            break
        objs.append(obj)
    if rc == 0:
        c = re.sub(r"\s+", " ",
                   "%s -g -%s %s %s -o %s.fpc"
                   % (fpcc, opt, " ".join(objs), ldflags, bench))
        rc, out = sh(c, cwd=str(dst), env=env)
        log_parts.append("$ %s\n%s" % (c, out))
    out = "\n".join(log_parts)
    (outdir / "build.log").write_text(out)

    # The per-TU banner is the authoritative signal that the pass ran.
    inst = re.findall(r"#FPCHECKER: Instrumented (\d+) @ (\S+)", out)
    if inst:
        print("    instrumented %d site(s) across %d TU(s)"
              % (sum(int(n) for n, _ in inst), len(inst)))

    binary = dst / ("%s.fpc" % bench)
    if rc != 0 or not binary.exists():
        print("    BUILD FAILED -- see %s" % (outdir / "build.log"))
        for line in out.splitlines():
            if "error" in line.lower():
                print("      %s" % line[:140])
                break
        return None

    _, nm_out = sh(["nm", str(binary)])
    nsym = sum(1 for l in nm_out.splitlines() if "_FPC" in l)
    width = check_width(dst, bench, precision, cc or "clang")
    prof = arith_profile(binary)
    print("    _FPC symbols=%d  sizeof(%s)=%s  arith(single,double)=%s"
          % (nsym, PROBE_TYPE[bench] or "-", width if width else "n/a", prof))

    (outdir / "build_info.txt").write_text(
        "benchmark   = %s\nprecision   = %s\nopt         = -%s   "
        "(compile AND link)\nCC          = %s\nCFLAGS      = %s\n"
        "LDFLAGS     = %s\ndefines     = %s\ninstr_var   = %s=1\n"
        "fpc_symbols = %d\nwidth       = %s (expected %s)\n"
        "arith       = single=%s double=%s\n"
        % (bench, precision, opt, fpcc, cflags, ldflags,
           defines or "(none)", ivar, nsym, width,
           EXPECTED_WIDTH[precision], prof[0] if prof else "?",
           prof[1] if prof else "?"))

    if nsym == 0:
        print("    *** UNINSTRUMENTED -- env vars did not reach compile "
              "and/or link. Skipping.")
        return None
    if width is not None and width != EXPECTED_WIDTH[precision]:
        print("    *** WRONG WIDTH (%s, expected %s) -- the precision define "
              "did not take. Skipping." % (width, EXPECTED_WIDTH[precision]))
        return None
    # IS is the Integer Sort benchmark: its work is bucket-sorting
    # integers, and the only FP is the double-typed timing and reporting
    # code, which does not follow the precision switch. So "fp32 emits no
    # single-precision arithmetic" is the CORRECT state there, not a
    # missed define. PROBE_TYPE is None for exactly the benchmarks with no
    # switchable FP type, so use that as the guard.
    if (prof and precision == "fp32" and prof[0] == 0
            and PROBE_TYPE[bench] is not None):
        print("    *** fp32 emits NO single-precision arithmetic -- the "
              "precision define did not take. Skipping.")
        return None
    return binary


def run(binary, bench, precision, eta, maxwarn, outdir, opt, timeout):
    env = base_env()
    # FPC_STABILITY_ETA_REL, not FPC_STABILITY_ETA -- the wrong name leaves
    # every run on the default and makes an eta sweep silently produce
    # identical results.
    env["FPC_STABILITY_ETA_REL"] = eta
    env["FPC_STABILITY_MAX_WARNINGS"] = str(maxwarn)

    log = outdir / ("run_%s_eta%s.log" % (opt, eta))
    rc, out = sh(["stdbuf", "-i0", "-o0", "-e0", str(binary)],
                 cwd=str(binary.parent), env=env, log=log, timeout=timeout)

    flips = [l for l in out.splitlines() if FLIP_RE.search(l)]
    sites = Counter()
    unparsed = 0
    for l in flips:
        m = SITE_RE.search(l)
        if not m:
            unparsed += 1
            continue
        key = "%s:%s" % (m.group(1), m.group(2))
        if m.group(3):
            key += "  [%s]" % m.group(3)     # C: no mangling to undo
        sites[key] += 1

    wrote = re.search(r"Successfully wrote (\d+) error entries", out)
    ver = VERIFY_RE.search(out)
    capped = len(flips) >= maxwarn
    if capped:
        print("      *** TRUNCATED: hit FPC_STABILITY_MAX_WARNINGS (%d); "
              "counts and site list incomplete" % maxwarn)

    return {"benchmark": bench, "precision": precision, "eta": eta,
            "opt": opt, "flips": len(flips), "locations": len(sites),
            "sites": dict(sites.most_common()),
            "verification": ver.group(1).upper() if ver else "?",
            "error_entries": int(wrote.group(1)) if wrote else None,
            "unparsed_lines": unparsed, "capped": capped,
            "maxwarn": maxwarn, "exit_code": rc,
            "log": str(log.relative_to(HERE))}


def write_results(bench, precision, records, outdir):
    r0 = records[0]
    lines = ["NAS %s %s -- FPChecker branch instability"
             % (bench.upper(), precision),
             "(-%s compile+link, serial build)" % r0["opt"], ""]
    for r in records:
        lines.append("%d flips @ %d loc   (eta=%s)   [verification=%s]"
                     % (r["flips"], r["locations"], r["eta"],
                        r["verification"]))
    lines.append("")
    for r in records:
        lines.append("--- eta=%s : %d flips @ %d loc ---"
                     % (r["eta"], r["flips"], r["locations"]))
        if not r["sites"]:
            lines.append("    (no sites)")
        for s, n in r["sites"].items():
            lines.append("    %8d  %s" % (n, s))
        if r["error_entries"] is not None:
            lines.append("    error entries written: %d" % r["error_entries"])
        if r["capped"]:
            lines.append("    *** TRUNCATED at maxwarn=%d" % r["maxwarn"])
        if r["unparsed_lines"]:
            lines.append("    UNPARSED flip lines: %d" % r["unparsed_lines"])
        lines.append("")
    (outdir / "summary.txt").write_text("\n".join(lines))
    (outdir / "summary.json").write_text(json.dumps(records, indent=2))


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-b", "--benchmarks", nargs="+", default=BENCHES,
                    choices=BENCHES)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("-e", "--eta", nargs="+", default=None)
    ap.add_argument("--eta-fp32", nargs="+", default=None)
    ap.add_argument("--eta-fp64", nargs="+", default=None)
    ap.add_argument("--opt", default="O0", choices=["O0", "O1", "O2", "O3"],
                    help="default O0: at -O2 FPChecker misattributes inlined "
                         "sites and loses most of them")
    ap.add_argument("--maxwarn", type=int, default=100_000_000)
    ap.add_argument("--timeout", type=int, default=None,
                    help="per-run timeout in seconds (default: none -- runs "
                         "to completion however long it takes)")
    ap.add_argument("-j", "--jobs", type=int, default=1)
    ap.add_argument("--cc", default="clang",
                    help="plain compiler for the width probe")
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

    print("NAS / FPChecker   benchmarks: %s   -%s"
          % (" ".join(args.benchmarks), args.opt))
    for k in args.precision:
        print("  %s eta: %s" % (k, " ".join(etas[k])))
    print("  bench: %s" % BENCH_ROOT)
    print("  work:  %s" % WORK_ROOT)
    print("  NOTE:  BT/CG/LU/SP take -DNAS_FP32; EP takes -DWORKING_T=...;")
    print("         IS/MG bake precision into the source and take no define.")
    print()

    overall = []
    for bench in args.benchmarks:
        print("===== %s =====" % bench.upper())
        for precision in args.precision:
            outdir = RESULT_ROOT / bench / args.opt / precision
            outdir.mkdir(parents=True, exist_ok=True)
            binary = build(bench, precision, args.opt, outdir,
                           args.jobs, args.cc)
            if binary is None:
                continue
            records = []
            for eta in etas[precision]:
                r = run(binary, bench, precision, eta, args.maxwarn,
                        outdir, args.opt, args.timeout)
                print("      eta=%-8s %7d flips @ %3d loc   verification=%s"
                      % (eta, r["flips"], r["locations"], r["verification"]))
                records.append(r)
            write_results(bench, precision, records, outdir)
            overall += records
        print()

    if not overall:
        print("No results produced.")
        return 1

    print("=" * 66)
    print("all results")
    print("=" * 66)
    print("  %-4s %-5s %-9s %8s %6s  %s"
          % ("bench", "prec", "eta", "flips", "sites", "verify"))
    for r in overall:
        print("  %-4s %-5s %-9s %8d %6d  %s"
              % (r["benchmark"], r["precision"], r["eta"], r["flips"],
                 r["locations"], r["verification"]))
    print("\nresults under %s/<bench>/%s/<precision>/"
          % (RESULT_ROOT, args.opt))
    print("\nbrtrace ground truth (fp32 vs fp64, -O0, sites):")
    print("  LU=3  SP=2  BT=2  EP=1  CG=0  MG=0  IS=0")
    print("  All but ep.c:207 are verify() epsilon gates -- they flip because")
    print("  fp32 failed verification, not because a computation branch was")
    print("  unstable. ep.c:207 (Marsaglia) is the only genuine algorithmic")
    print("  branch in the NAS ground truth.")
    return 0


if __name__ == "__main__":
    sys.exit(main())