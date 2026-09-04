#!/usr/bin/env python3
"""
run_nas_nsan.py -- build the NAS benchmarks under nsan with the NsanBFSites
plugin and run them, fp32 and fp64 separately.

    ./run_nas_nsan.py                      # all 7, both precisions
    ./run_nas_nsan.py -b lu sp bt ep -p fp32
    ./run_nas_nsan.py --sites-only
    ./run_nas_nsan.py --resume-policy discard --eq-policy truncated

Shadow mapping dqq for fp32 (fp32-vs-fp64 cell), dlq for fp64 (fp64-vs-ld).
All NAS benchmarks are oracle-stable at fp64.

Precision selection differs per tree: BT/CG/LU/SP take -DNAS_FP32, EP takes
-DWORKING_T=..., IS/MG bake it in. A missed define silently builds the wrong
precision; the sizeof probe and objdump profile catch that. Problem size is
compiled in (class S; CG is W). Compiled with direct clang invocations,
mirroring run_nas_fpchecker.py.

Ground truth (brtrace, fp32 vs fp64, -O0): LU=3 SP=2 BT=2 EP=1 sites,
LU=11 SP=10 BT=10 EP=1 events; CG=MG=IS=0. All but ep.c:207 are verify()
epsilon gates. IS has no FP-controlled branches; its zero is vacuous.

Sets RLIMIT_STACK itself; see run_lulesh_nsan.py.
"""

import argparse
import json
import os
import re
import resource
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

BENCHES = ["bt", "cg", "ep", "is", "lu", "mg", "sp"]

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
    "ep": "working_t",
    "is": None, "mg": None,
}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8}

PRECISION_MARKERS = [("single", r"\b(mulss|addss|subss|divss)\b"),
                     ("double", r"\b(mulsd|addsd|subsd|divsd)\b")]

SHADOW_MAP = {"fp32": "dqq", "fp64": "dlq"}

FPC_ROOT = Path(os.environ.get("FPC_ROOT", Path(__file__).resolve().parents[5]))
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT",
    FPC_ROOT / "cpu_checking/error_analysis/branch_flip/benchmarks/nas"))

NSAN_BF = Path(os.environ.get("NSAN_BF_HOME", Path(__file__).resolve().parent / "nsan"))
PLUGIN = NSAN_BF / "plugin" / "libNsanBFSites.so"
SHIM = NSAN_BF / "runtime" / "libnsan_bf.a"

CONDA_LIB = os.environ.get("CONDA_PREFIX", "") and \
    os.path.join(os.environ["CONDA_PREFIX"], "lib")

HERE = Path(__file__).resolve().parent
BENCH_NAME = "nas"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

EXPECTED_SITES = {b: None for b in BENCHES}

WRAP_SYMS = [
    "__nsan_fcmp_fail_float_d",
    "__nsan_fcmp_fail_double_q",
    "__nsan_fcmp_fail_double_l",
    "__nsan_fcmp_fail_longdouble_q",
]

BANNER_RE = re.compile(
    r"^\[NSanBF\]\s+(\S+)\s+\(mod\s+(\d+)\):\s+(\d+)\s+FP-controlled branch "
    r"sites,\s+(\d+)\s+select sites,\s+(\d+)\s+out-of-scope fcmps"
    r"(?:,\s+(\d+)\s+shared)?")
EVENT_RE = re.compile(
    r"^#NSAN_EVENT\s+mod=(\d+)\s+site=(-?\d+)\s+k=(\d+)\s+kind=(\w+)\s+"
    r"verdict=(\w+)\s+pred=(\d+)\s+native=(\d)\s+shadow=(\d)")
SITE_DUMP_RE = re.compile(
    r"^#NSAN_SITE\s+(\d+)\s+(-?\d+)\s+(\w+)\s+(\d+)\s+(\d+)")
TOTALS_RE = re.compile(
    r"^#NSAN_TOTALS\s+events=(\d+)\s+out_of_scope=(\d+)\s+unticked=(\d+)\s+"
    r"overflow=(\d+)")
VERIFY_RE = re.compile(r"\b(SUCCESSFUL|UNSUCCESSFUL)\b", re.I)


def sh(cmd, cwd=None, env=None, log=None, timeout=None):
    try:
        p = subprocess.run(cmd, cwd=cwd, env=env, shell=isinstance(cmd, str),
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


def set_stack_limit(mb=64):
    soft, hard = resource.getrlimit(resource.RLIMIT_STACK)
    want = mb * 1024 * 1024
    if hard != resource.RLIM_INFINITY:
        want = min(want, hard)
    if soft == resource.RLIM_INFINITY or soft > want:
        resource.setrlimit(resource.RLIMIT_STACK, (want, hard))
        return "lowered to %d MiB (was unlimited)" % (want // (1024 * 1024))
    return "already finite (%d MiB)" % (soft // (1024 * 1024))


def parse_banners(text):
    out = {}
    for line in text.splitlines():
        m = BANNER_RE.match(line.strip())
        if m:
            out[os.path.basename(m.group(1))] = (
                int(m.group(2)), int(m.group(3)), int(m.group(4)),
                int(m.group(5)), int(m.group(6) or 0))
    return out


def parse_events(text):
    events, totals, tot = [], {}, None
    for line in text.splitlines():
        m = EVENT_RE.match(line)
        if m:
            events.append((int(m.group(1)), int(m.group(2)), int(m.group(3)),
                           m.group(4), m.group(5), int(m.group(6)),
                           int(m.group(7)), int(m.group(8))))
            continue
        m = SITE_DUMP_RE.match(line)
        if m:
            totals[(int(m.group(1)), int(m.group(2)), m.group(3))] = (
                int(m.group(4)), int(m.group(5)))
            continue
        m = TOTALS_RE.match(line)
        if m:
            tot = {"events": int(m.group(1)), "out_of_scope": int(m.group(2)),
                   "unticked": int(m.group(3)), "overflow": int(m.group(4))}
    return events, totals, tot


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


def build(bench, precision, opt, outdir, eq_policy, cc="clang"):
    src = BENCH_ROOT / bench / ("%s_%s" % (bench, precision))
    dst = BUILD_ROOT / bench / opt / ("%s_%s" % (bench, precision))
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print("  FATAL: no source tree at %s" % src)
        return None
    for p, what in ((PLUGIN, "plugin"), (SHIM, "runtime shim")):
        if not p.exists():
            print("  FATAL: no %s at %s" % (what, p))
            print("         run %s/build_instrumentation.sh" % NSAN_BF)
            return None

    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for pat in ("*.o", "*.brsites", "*.brmods", "*.brx", "*.nsansites",
                "*.nsanselsites"):
        for j in dst.glob(pat):
            j.unlink()
    for j in dst.iterdir():
        if j.is_file() and os.access(j, os.X_OK) and j.suffix == "":
            j.unlink()

    env = base_env()
    env["NSAN_BF_OPT_LEVEL"] = opt

    mapping = SHADOW_MAP[precision]
    eq = "1" if eq_policy == "truncated" else "0"
    defines = DEFINES[bench][precision]
    cflags = ("-g -%s -I. -std=c99 -w %s -fsanitize=numerical "
              "-fpass-plugin=%s -mllvm -nsan-shadow-type-mapping=%s "
              "-mllvm -nsan-truncate-fcmp-eq=%s"
              % (opt, defines, PLUGIN, mapping, eq))
    wrap = " ".join("-Wl,--wrap=%s" % s for s in WRAP_SYMS)
    ldflags = ("-lm -fsanitize=numerical -Wl,--whole-archive %s "
               "-Wl,--no-whole-archive %s" % (SHIM, wrap))
    print("  %-4s %-5s  map=%s  %s" % (bench, precision, mapping,
                                       defines or "(no define)"))

    objs, log_parts, rc = [], [], 0
    for srcf in SRCS[bench]:
        obj = srcf[:-2] + ".o"
        c = re.sub(r"\s+", " ", "clang %s -c %s -o %s" % (cflags, srcf, obj))
        rc, out = sh(c, cwd=str(dst), env=env)
        log_parts.append("$ %s\n%s" % (c, out))
        if rc != 0:
            break
        objs.append(obj)
    if rc == 0:
        c = re.sub(r"\s+", " ",
                   "clang -g -%s %s %s -o %s.nsan"
                   % (opt, " ".join(objs), ldflags, bench))
        rc, out = sh(c, cwd=str(dst), env=env)
        log_parts.append("$ %s\n%s" % (c, out))
    out = "\n".join(log_parts)
    (outdir / "build.log").write_text(out)

    binary = dst / ("%s.nsan" % bench)
    if rc != 0 or not binary.exists():
        print("    BUILD FAILED -- see %s" % (outdir / "build.log"))
        for line in out.splitlines():
            if "error" in line.lower():
                print("      %s" % line[:140])
                break
        return None

    # --- gate 1: instrumented at all? ---
    _, nm_out = sh(["nm", str(binary)])
    n_nsan = sum(1 for l in nm_out.splitlines() if "__nsan_" in l)
    has_tick = any("__nsan_bf_tick" in l for l in nm_out.splitlines())
    if n_nsan == 0 or not has_tick:
        print("    *** UNINSTRUMENTED BINARY (nsan syms=%d, tick=%s). "
              "Skipping." % (n_nsan, has_tick))
        return None

    # --- gate 2: did every wrap take? ---
    defined = {parts[-1] for l in nm_out.splitlines()
               for parts in [l.split()]
               if len(parts) >= 2 and
               parts[-1].startswith("__wrap___nsan_fcmp_fail")}
    missing = [s for s in WRAP_SYMS if "__wrap_%s" % s not in defined]
    if missing:
        print("    *** WRAP NOT APPLIED for: %s" % ", ".join(missing))
        return None

    # --- gate 3: the site census ---
    banners = parse_banners(out)
    if not banners:
        print("    *** no [NSanBF] banner -- the plugin did not run. "
              "Skipping.")
        return None
    branch_total = sum(b[1] for b in banners.values())
    select_total = sum(b[2] for b in banners.values())
    oos_total = sum(b[3] for b in banners.values())
    shared_total = sum(b[4] for b in banners.values())
    exp = EXPECTED_SITES.get(bench)
    flag = "  *** EXPECTED %d" % exp if (exp is not None
                                         and branch_total != exp) else ""
    print("    branch sites=%d across %d TU(s)  (select %d, oos %d)%s"
          % (branch_total, len(banners), select_total, oos_total, flag))

    # --- collect the site tables ---
    manifests = (sorted(dst.glob("*.nsansites"))
                 + sorted(dst.glob("*.nsanselsites")))
    if manifests:
        mdir = outdir / "nsansites"
        if mdir.exists():
            shutil.rmtree(mdir)
        mdir.mkdir(parents=True, exist_ok=True)
        for m in manifests:
            shutil.copy2(m, mdir / m.name)
    else:
        pass

    # --- gate 4: did the precision define take? ---
    width = check_width(dst, bench, precision, cc)
    prof = arith_profile(binary)
    print("    nsan syms=%d  wraps=4/4  sizeof(%s)=%s  arith(s,d)=%s"
          % (n_nsan, PROBE_TYPE[bench] or "-", width if width else "n/a",
             prof))

    (outdir / "build_info.txt").write_text(
        "benchmark     = %s\nprecision     = %s\n"
        "opt           = -%s   (compile AND link)\n"
        "shadow_map    = %s   (%s)\neq_truncate   = %s\n"
        "CFLAGS        = %s\nLDFLAGS       = %s\ndefines       = %s\n"
        "nsan_symbols  = %d\nwrapped_hooks = %d\n"
        "width         = %s (expected %s)\narith         = single=%s "
        "double=%s\nbranch_sites  = %d\nselect_sites  = %d\n"
        "oos_fcmps     = %d\nshared_fcmps  = %d\noracle_cell   = %s\n"
        % (bench, precision, opt, mapping,
           "float->double" if precision == "fp32" else "double->x86_fp80",
           eq, cflags, ldflags, defines or "(none)", n_nsan, len(WRAP_SYMS),
           width, EXPECTED_WIDTH[precision], prof[0] if prof else "?",
           prof[1] if prof else "?", branch_total, select_total, oos_total,
           shared_total,
           "fp32_vs_fp64" if precision == "fp32" else "fp64_vs_ld"))

    if width is not None and width != EXPECTED_WIDTH[precision]:
        print("    *** WRONG WIDTH (%s, expected %s) -- the precision define "
              "did not take. Skipping."
              % (width, EXPECTED_WIDTH[precision]))
        return None
    if (prof and precision == "fp32" and prof[0] == 0
            and PROBE_TYPE[bench] is not None):
        print("    *** fp32 emits NO single-precision arithmetic -- the "
              "precision define did not take. Skipping.")
        return None
    return binary


def run(binary, bench, precision, outdir, opt, resume_policy, timeout):
    env = base_env()
    resume = "0" if resume_policy == "keep" else "1"
    env["NSAN_OPTIONS"] = ("halt_on_error=0:resume_after_warning=%s:"
                           "disable_warnings=1" % resume)

    log = outdir / ("run_%s.log" % opt)
    bflog = outdir / ("events_%s.log" % opt)
    env["NSAN_BF_LOG"] = str(bflog)

    rc, out = sh(["stdbuf", "-i0", "-o0", "-e0", str(binary)],
                 cwd=str(binary.parent), env=env, log=log, timeout=timeout)

    bftext = bflog.read_text() if bflog.exists() else ""
    events, site_totals, totals = parse_events(bftext)

    by_site, kinds = Counter(), Counter()
    for mod, site, k, kind, verdict, pred, native, shadow in events:
        kinds[kind] += 1
        by_site["mod%d:%s%d" % (mod, kind, site)] += 1

    executed = sum(1 for (m, s, kd), v in site_totals.items()
                   if kd == "branch" and v[0] > 0)
    total_exec = sum(v[0] for (m, s, kd), v in site_totals.items()
                     if kd == "branch")
    ver = VERIFY_RE.search(out)

    return {"benchmark": bench, "precision": precision, "opt": opt,
            "shadow_map": SHADOW_MAP[precision],
            "resume_after_warning": resume,
            "flips": len(events),
            "branch_flips": kinds["branch"],
            "select_flips": kinds["select"],
            "out_of_scope_flips": kinds["oos"],
            "locations": len(by_site),
            "sites": dict(by_site.most_common()),
            "executed_branch_sites": executed,
            "total_branch_executions": total_exec,
            "totals": totals,
            "verification": ver.group(1).upper() if ver else "?",
            "exit_code": rc,
            "log": str(log.relative_to(HERE)),
            "event_log": str(bflog.relative_to(HERE))}


def write_results(bench, precision, r, outdir):
    L = ["NAS %s %s -- nsan branch flips" % (bench.upper(), precision),
         "(-%s compile+link, shadow map %s)" % (r["opt"], r["shadow_map"]),
         ""]
    L.append("%d branch flips @ %d site(s)   [verification=%s]"
             % (r["branch_flips"], r["locations"], r["verification"]))
    L.append("")
    if bench == "is":
        L.append("success.")
        L.append("")
    if r["out_of_scope_flips"]:
        L.append("out-of-scope flips (site_id -1): %d"
                 % r["out_of_scope_flips"])
    for s, n in list(r["sites"].items())[:40]:
        L.append("    %8d  %s" % (n, s))
    L.append("")
    L.append("branch sites executed: %d   total executions: %s"
             % (r["executed_branch_sites"],
                "{:,}".format(r["total_branch_executions"])))
    t = r["totals"]
    if t:
        L.append("runtime totals: %s" % t)
        if t["unticked"]:
            L.append("")
            L.append("*** unticked > 0 -- DO NOT SCORE THIS RUN.")
        if t["overflow"]:
            L.append("*** site table overflow -- raise NSANBF_TAB_BITS.")
    else:
        L.append("*** no #NSAN_TOTALS line: the shim did not run at exit.")
    L.append("")
    L.append("events: %s" % r["event_log"])
    (outdir / "summary.txt").write_text("\n".join(L))
    (outdir / "summary.json").write_text(json.dumps(r, indent=2))


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-b", "--benchmarks", nargs="+", default=BENCHES,
                    choices=BENCHES)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("--opt", default="O0", choices=["O0", "O1", "O2", "O3"],
                    help="compile AND link level, exported as "
                         "NSAN_BF_OPT_LEVEL. Site ids correspond across "
                         "levels; occurrence indices do not. MUST match the "
                         "level the oracle traces were produced at.")
    ap.add_argument("--resume-policy", choices=["keep", "discard"],
                    default="keep",
                    help="keep (default) sets resume_after_warning=0 and "
                         "preserves the shadow, so flips downstream of a "
                         "value warning stay visible. discard is nsan's "
                         "shipped default and suppresses them.")
    ap.add_argument("--eq-policy", choices=["untruncated", "truncated"],
                    default="untruncated",
                    help="-nsan-truncate-fcmp-eq. truncated is nsan's "
                         "default and rounds the shadow to app precision "
                         "before == and !=, which CREATES disagreements the "
                         "untruncated comparison does not have.")
    ap.add_argument("--sites-only", action="store_true",
                    help="build and emit the site tables, then stop")
    ap.add_argument("--timeout", type=int, default=None,
                    help="per-run timeout in seconds (default: none)")
    ap.add_argument("--stack-mb", type=int, default=64)
    ap.add_argument("--cc", default="clang",
                    help="plain compiler for the width probe")
    args = ap.parse_args()

    note = set_stack_limit(args.stack_mb)
    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)

    print("NAS / nsan   benchmarks: %s   -%s"
          % (" ".join(args.benchmarks), args.opt))
    print("  stack limit: %s" % note)
    print("  resume policy: %s   eq policy: %s"
          % (args.resume_policy, args.eq_policy))
    print("  plugin: %s" % PLUGIN)
    print("  shim:   %s" % SHIM)
    print("  bench:  %s" % BENCH_ROOT)
    print()

    overall = []
    for bench in args.benchmarks:
        print("===== %s =====" % bench.upper())
        for precision in args.precision:
            outdir = RESULT_ROOT / bench / args.opt / precision
            tag = []
            if args.resume_policy != "keep":
                tag.append("resume-" + args.resume_policy)
            if args.eq_policy != "untruncated":
                tag.append("eq-" + args.eq_policy)
            if tag:
                outdir = outdir.with_name(precision + "_" + "_".join(tag))
            outdir.mkdir(parents=True, exist_ok=True)

            binary = build(bench, precision, args.opt, outdir,
                           args.eq_policy, args.cc)
            if binary is None:
                continue
            if args.sites_only:
                continue

            r = run(binary, bench, precision, outdir, args.opt,
                    args.resume_policy, args.timeout)
            write_results(bench, precision, r, outdir)
            overall.append(r)

            extra = ""
            if r["out_of_scope_flips"]:
                extra += "  out-of-scope %d" % r["out_of_scope_flips"]
            if r["select_flips"]:
                extra += "  select %d" % r["select_flips"]
            print("      %7d branch flips @ %3d site(s)   verification=%s%s"
                  % (r["branch_flips"], r["locations"], r["verification"],
                     extra))
            if r["totals"] and r["totals"]["unticked"]:
                print("      *** unticked=%d -- DO NOT SCORE THIS RUN"
                      % r["totals"]["unticked"])
        print()

    if args.sites_only:
        print("===== site tables built; nothing run =====")
        return 0

    if not overall:
        print("No results produced.")
        return 1

    print("=" * 72)
    print("all results")
    print("=" * 72)
    print("  %-4s %-5s %-4s %8s %6s %12s  %s"
          % ("bench", "prec", "map", "flips", "sites", "executions",
             "verify"))
    for r in overall:
        print("  %-4s %-5s %-4s %8d %6d %12s  %s"
              % (r["benchmark"], r["precision"], r["shadow_map"],
                 r["branch_flips"], r["locations"],
                 "{:,}".format(r["total_branch_executions"]),
                 r["verification"]))
    print("\nresults under %s/<bench>/%s/<precision>/"
          % (RESULT_ROOT, args.opt))
    return 0


if __name__ == "__main__":
    sys.exit(main())
