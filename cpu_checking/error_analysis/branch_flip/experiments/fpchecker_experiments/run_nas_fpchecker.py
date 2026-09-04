#!/usr/bin/env python3
"""
run_nas_fpchecker.py -- build the NAS benchmarks under FPChecker branch-flip
instrumentation and run them, fp32 and fp64 separately.

    ./run_nas_fpchecker.py                     # all 7, both precisions
    ./run_nas_fpchecker.py -b lu sp bt ep -p fp32
    ./run_nas_fpchecker.py --bf-mode both
    ./run_nas_fpchecker.py --shadow-fallback zero

Precision selection differs per tree: BT/CG/LU/SP take -DNAS_FP32, EP takes
-DWORKING_T=..., IS/MG bake it in; the sizeof probe and the objdump
arithmetic profile catch a define that did not take. Problem size is
compiled in (class S; CG is W). Compiled with direct clang-fpchecker
invocations, two-step (compile, then link). See fpc_common.py for the
environment and outputs.
"""

import argparse
import os
import re
import shutil
import sys

from fpc_common import (BENCH_BASE, FPC_INSTALL, WORK_BASE, add_common_args,
                        base_env, check_binary, check_shadow_invariance,
                        collect_manifests, etas_for, fallback_flag, instr_var,
                        make_env, one_line, print_header, run_instrumented, sh,
                        write_results)

BENCHES = ["bt", "cg", "ep", "is", "lu", "mg", "sp"]
BENCH_ROOT = BENCH_BASE / "nas"
WORK_ROOT = WORK_BASE / "nas"
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

SRCS = {
    "bt": ["bt.c"], "cg": ["cg.c"], "lu": ["lu.c"], "mg": ["mg.c"],
    "sp": ["sp.c"],
    "ep": ["ep.c", "c_randdp.c", "c_print_results.c", "c_timers.c",
           "wtime.c"],
    "is": ["is.c", "c_print_results.c", "c_timers.c", "wtime.c"],
}
_SWITCH = {"fp32": "-DNAS_FP32", "fp64": ""}
_NONE = {"fp32": "", "fp64": ""}
DEFINES = {
    "bt": _SWITCH, "cg": _SWITCH, "lu": _SWITCH, "sp": _SWITCH,
    "is": _NONE, "mg": _NONE,
    "ep": {"fp32": "-DWORKING_T=float -DWORKING_T_IS_FLOAT=1",
           "fp64": "-DWORKING_T=double"},
}
PROBE_TYPE = {
    "bt": "nas_real", "cg": "nas_real", "lu": "nas_real", "sp": "nas_real",
    "ep": "working_t", "is": None, "mg": None,
}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8}
PRECISION_MARKERS = [("single", r"\b(mulss|addss|subss|divss)\b"),
                     ("double", r"\b(mulsd|addsd|subsd|divsd)\b")]
EXPECTED_SITES = {b: None for b in BENCHES}
VERIFY_RE = re.compile(r"\b(SUCCESSFUL|UNSUCCESSFUL)\b", re.I)


def check_width(tree, bench, precision, cc="clang"):
    typ = PROBE_TYPE[bench]
    if not typ:
        return None
    WORK_ROOT.mkdir(parents=True, exist_ok=True)
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


def build(bench, precision, args, outdir):
    src = BENCH_ROOT / bench / f"{bench}_{precision}"
    dst = BUILD_ROOT / bench / args.opt / f"{bench}_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        return None
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for pat in ("*.o", "*.brsites", "*.brmods", "*.brx"):
        for j in dst.glob(pat):
            j.unlink()
    for j in dst.iterdir():
        if j.is_file() and os.access(j, os.X_OK) and j.suffix == "":
            j.unlink()

    fpcc = FPC_INSTALL / "bin" / "clang-fpchecker"
    if not fpcc.exists():
        print(f"  FATAL: no clang-fpchecker at {fpcc}")
        return None

    env = make_env(precision, args.opt)
    defines = DEFINES[bench][precision]
    cflags = (f"-g -{args.opt} -I. -std=c99 -w {defines} "
              f"{fallback_flag(args.shadow_fallback)}").strip()
    ldflags = "-lm -Wl,--allow-multiple-definition"
    print(f"  {bench:<4s} {precision:<5s} {instr_var(precision)}=1  "
          f"{defines or '(no define)'}")

    objs, log_parts, rc = [], [], 0
    for srcf in SRCS[bench]:
        obj = srcf[:-2] + ".o"
        c = re.sub(r"\s+", " ", f"{fpcc} {cflags} -c {srcf} -o {obj}")
        rc, out = sh(c, cwd=str(dst), env=env)
        log_parts.append(f"$ {c}\n{out}")
        if rc != 0:
            break
        objs.append(obj)
    if rc == 0:
        c = re.sub(r"\s+", " ", f"{fpcc} -g -{args.opt} {' '.join(objs)} "
                               f"{ldflags} -o {bench}.fpc")
        rc, out = sh(c, cwd=str(dst), env=env)
        log_parts.append(f"$ {c}\n{out}")
    out = "\n".join(log_parts)
    (outdir / "build.log").write_text(out)

    binary = dst / f"{bench}.fpc"
    if rc != 0 or not binary.exists():
        print(f"    BUILD FAILED -- see {outdir / 'build.log'}")
        return None

    gate = check_binary(binary, out, outdir, EXPECTED_SITES.get(bench),
                        indent="    ")
    if gate is None:
        return None
    nsym, banners, site_total, ntab, nmode = gate
    n_man = collect_manifests(dst, outdir)

    width = check_width(dst, bench, precision, args.cc)
    prof = arith_profile(binary)
    print(f"    sizeof({PROBE_TYPE[bench] or '-'})={width if width else 'n/a'}"
          f"  arith(single,double)={prof}")

    (outdir / "build_info.txt").write_text(
        f"benchmark   = {bench}\nprecision   = {precision}\n"
        f"opt         = -{args.opt}\nCC          = {fpcc}\n"
        f"CFLAGS      = {cflags}\nLDFLAGS     = {ldflags}\n"
        f"defines     = {defines or '(none)'}\n"
        f"instr_var   = {instr_var(precision)}=1\n"
        f"fallback    = {args.shadow_fallback}\n"
        f"fpc_symbols = {nsym}\n"
        f"width       = {width} (expected {EXPECTED_WIDTH[precision]})\n"
        f"arith       = single={prof[0] if prof else '?'} "
        f"double={prof[1] if prof else '?'}\n"
        f"branch_sites= {site_total}\nsite_tables = {ntab}\n"
        f"bf_mode_syms= {nmode}\nmanifests   = {n_man}\n")

    if width is not None and width != EXPECTED_WIDTH[precision]:
        print(f"    *** WRONG WIDTH ({width}, expected "
              f"{EXPECTED_WIDTH[precision]}). Skipping.")
        return None
    if (prof and precision == "fp32" and prof[0] == 0
            and PROBE_TYPE[bench] is not None):
        print("    *** fp32 emits no single-precision arithmetic. Skipping.")
        return None
    return binary


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-b", "--benchmarks", nargs="+", default=BENCHES,
                    choices=BENCHES)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("--timeout", type=int, default=None,
                    help="per-run timeout in seconds")
    ap.add_argument("--cc", default="clang", help="plain compiler for the probe")
    add_common_args(ap)
    args = ap.parse_args()

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)
    etas = etas_for(args, args.precision)
    print_header("NAS", args, f"benchmarks: {' '.join(args.benchmarks)}")

    overall = []
    for bench in args.benchmarks:
        print(f"===== {bench.upper()} =====")
        for precision in args.precision:
            suffix = ""
            if args.bf_mode != "interval":
                suffix += "_rule-" + args.bf_mode
            if args.nonfinite_policy != "abstain":
                suffix += "_nonfinite-" + args.nonfinite_policy
            if args.shadow_fallback != "native":
                suffix += "_fallback-" + args.shadow_fallback
            outdir = RESULT_ROOT / bench / args.opt / (precision + suffix)
            outdir.mkdir(parents=True, exist_ok=True)

            binary = build(bench, precision, args, outdir)
            if binary is None:
                continue

            records = []
            for eta in etas[precision]:
                tag = (f"{args.opt}_eta{eta}" if args.bf_mode != "shadow"
                       else f"{args.opt}_shadow")
                rec, out = run_instrumented(binary, [], eta, args, outdir, tag,
                                            timeout=args.timeout)
                ver = VERIFY_RE.search(out)
                rec.update({"benchmark": bench, "precision": precision,
                            "verification": ver.group(1).upper() if ver else "?"})
                records.append(rec)
                print(f"      eta={eta:<8s} {one_line(rec)}   "
                      f"verification={rec['verification']}")
                for msg in rec["consistency"]:
                    print(f"         *** {msg}")
            inv = check_shadow_invariance(records)
            if inv:
                print(f"         *** {inv}")
            write_results(f"NAS {bench.upper()} {precision} -- FPChecker "
                          f"branch flips (-{args.opt}, rule={args.bf_mode})",
                          records, outdir,
                          extra_per_record=lambda r: [
                              f"  verification: {r['verification']}"])
            overall += records
        print()

    if not overall:
        print("No results produced.")
        return 1

    print("=" * 70)
    print("  %-4s %-5s %-9s %-9s %8s %6s  %s"
          % ("bench", "prec", "rule", "eta", "flips", "sites", "verify"))
    for r in overall:
        for rule in ("interval", "shadow"):
            if rule in r:
                print("  %-4s %-5s %-9s %-9s %8d %6d  %s"
                      % (r["benchmark"], r["precision"], rule,
                         r["eta"] or "n/a", r[rule]["flips"],
                         r[rule]["locations"], r["verification"]))
    print(f"\nresults under {RESULT_ROOT}/<bench>/{args.opt}/<precision>/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
