#!/usr/bin/env python3
"""
run_quicksilver_nsan.py -- build QuickSilver under nsan with the NsanBFSites
plugin and run it, fp32 and fp64 separately.

    ./run_quicksilver_nsan.py                  # both precisions
    ./run_quicksilver_nsan.py -p fp32
    ./run_quicksilver_nsan.py --sites-only
    ./run_quicksilver_nsan.py -n 2000 -N 10
    ./run_quicksilver_nsan.py --resume-policy discard --eq-policy truncated

Shadow mapping dqq for fp32 (fp32-vs-fp64 cell), dlq for fp64 (fp64-vs-ld).
QuickSilver is oracle-stable at fp64, so that cell is a false-alarm
measurement.

Census config: -n 4000 -X 10 -Y 10 -Z 10 -x 5 -y 5 -z 5 -I 1 -J 1 -K 1 -N 3.
First divergence is at cycle 2. Nothing flips below 2800 particles, and
divergence is non-monotonic in mesh size. The RNG is integer-only, so any
divergence is physics arithmetic.

This is the heaviest cell; consider a compute node. Sets RLIMIT_STACK itself;
see run_lulesh_nsan.py.
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

# --------------------------------------------------------------------
PRECISION_FLAG = {"fp32": "-DQS_FP32", "fp64": "", "ld": "-DQS_LD"}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8, "ld": 16}
SHADOW_MAP = {"fp32": "dqq", "fp64": "dlq"}

FPC_ROOT = Path(os.environ.get("FPC_ROOT", Path(__file__).resolve().parents[5]))
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT",
    FPC_ROOT / "cpu_checking/error_analysis/branch_flip/benchmarks/"
               "quicksilver"))

NSAN_BF = Path(os.environ.get("NSAN_BF_HOME", Path(__file__).resolve().parent / "nsan"))
PLUGIN = NSAN_BF / "plugin" / "libNsanBFSites.so"
SHIM = NSAN_BF / "runtime" / "libnsan_bf.a"

CONDA_LIB = os.environ.get("CONDA_PREFIX", "") and \
    os.path.join(os.environ["CONDA_PREFIX"], "lib")

HERE = Path(__file__).resolve().parent
BENCH_NAME = "quicksilver"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

EXPECTED_SITES = None

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

# tally rows: cycle start source rr split absorb scatter fission produce
#             collisn escape census num_seg scalar_flux ...
TALLY_RE = re.compile(r"^\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+"
                      r"(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+\d+\s+(\d+)")

_DEMANGLE_CACHE = {}


def demangle(sym):
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


def set_stack_limit(mb=64):
    soft, hard = resource.getrlimit(resource.RLIMIT_STACK)
    want = mb * 1024 * 1024
    if hard != resource.RLIM_INFINITY:
        want = min(want, hard)
    if soft == resource.RLIM_INFINITY or soft > want:
        resource.setrlimit(resource.RLIMIT_STACK, (want, hard))
        return f"lowered to {want // (1024*1024)} MiB (was unlimited)"
    return f"already finite ({soft // (1024*1024)} MiB)"


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


def check_width(tree, precision, cxx="clang++"):
    probe = WORK_ROOT / "_width_probe.cc"
    probe.parent.mkdir(parents=True, exist_ok=True)
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


def build(precision, opt, outdir, eq_policy, jobs=1, cxx="clang++"):
    src = BENCH_ROOT / f"qs_{precision}"
    dst = BUILD_ROOT / opt / f"qs_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        return None
    for p, what in ((PLUGIN, "plugin"), (SHIM, "runtime shim")):
        if not p.exists():
            print(f"  FATAL: no {what} at {p}")
            print(f"         run {NSAN_BF}/build_instrumentation.sh")
            return None

    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for junk in list(dst.glob("*.o")) + list(dst.glob("qs")):
        junk.unlink()

    env = base_env()
    env["NSAN_BF_OPT_LEVEL"] = opt

    mapping = SHADOW_MAP[precision]
    eq = "1" if eq_policy == "truncated" else "0"
    wrap = " ".join(f"-Wl,--wrap={s}" for s in WRAP_SYMS)

    cxxf = (f"-std=c++11 -g -{opt} -include cstdint "
            f"{PRECISION_FLAG[precision]} "
            f"-fsanitize=numerical -fpass-plugin={PLUGIN} "
            f"-mllvm -nsan-shadow-type-mapping={mapping} "
            f"-mllvm -nsan-truncate-fcmp-eq={eq}").strip()
    shim_link = f"-Wl,--whole-archive {SHIM} -Wl,--no-whole-archive"
    ldf = f"-g -{opt} -fsanitize=numerical {shim_link} {wrap}"
    cmd = ["make", f"-j{jobs}", "CXX=clang++",
           f"CXXFLAGS={cxxf}", "CPPFLAGS=", f"LDFLAGS={ldf}"]

    print(f"  building ({precision}, -{opt}, map={mapping}, "
          f"eq-trunc={eq}, -j{jobs})")
    rc, _ = sh(cmd, cwd=dst, env=env, log=outdir / "build.log")

    binary = dst / "qs"
    if not binary.exists():
        print(f"  BUILD FAILED -- see {outdir/'build.log'}")
        for line in (outdir / "build.log").read_text().splitlines():
            if "error" in line.lower():
                print("   ", line[:150])
                break
        return None

    # --- gate 1: instrumented at all? ---
    _, nm_out = sh(["nm", str(binary)])
    n_nsan = sum(1 for l in nm_out.splitlines() if "__nsan_" in l)
    has_tick = any("__nsan_bf_tick" in l for l in nm_out.splitlines())
    print(f"  __nsan_* symbols   = {n_nsan}   tick present = {has_tick}")
    if n_nsan == 0 or not has_tick:
        return None

    # --- gate 2: did every wrap take? ---
    defined = {parts[-1] for l in nm_out.splitlines()
               for parts in [l.split()]
               if len(parts) >= 2 and
               parts[-1].startswith("__wrap___nsan_fcmp_fail")}
    missing = [s for s in WRAP_SYMS if f"__wrap_{s}" not in defined]
    if missing:
        print(f"  *** WRAP NOT APPLIED for: {', '.join(missing)}")
        return None
    print(f"  wrapped hooks      = {len(WRAP_SYMS)}/{len(WRAP_SYMS)}")

    # --- gate 3: the site census ---
    build_log = (outdir / "build.log").read_text()
    banners = parse_banners(build_log)
    if not banners:
        print("  *** no [NSanBF] banner in the build log -- the plugin did "
              "not run.")
        return None
    branch_total = sum(b[1] for b in banners.values())
    select_total = sum(b[2] for b in banners.values())
    oos_total = sum(b[3] for b in banners.values())
    shared_total = sum(b[4] for b in banners.values())
    print(f"  branch sites       = {branch_total} across {len(banners)} TU(s)"
          f"   (selects {select_total}, oos fcmps {oos_total})")
    top = sorted(banners.items(), key=lambda kv: -kv[1][1])[:8]
    for mod, b in top:
        if b[1]:
            print(f"      {mod:<28s} mod {b[0]:<11d} {b[1]:5d} branch "
                  f"{b[2]:4d} select")
    if len(banners) > len(top):
        print(f"      ... and {len(banners) - len(top)} more TU(s)")

    total_sites = branch_total + select_total + oos_total

    manifests = sorted(dst.glob("**/*.nsansites")) + \
        sorted(dst.glob("**/*.nsanselsites"))
    if manifests:
        mdir = outdir / "nsansites"
        if mdir.exists():
            shutil.rmtree(mdir)
        mdir.mkdir(parents=True, exist_ok=True)
        for m in manifests:
            shutil.copy2(m, mdir / m.name)
        print(f"  collected {len(manifests)} site table(s) -> "
              f"{mdir.relative_to(HERE)}")
    else:
        pass

    # --- gate 4: the precision flag reached the header ---
    width = check_width(dst, precision, cxx)
    ok_w = (width == EXPECTED_WIDTH[precision])
    print(f"  sizeof(qs_real)    = {width} "
          f"({'ok' if ok_w else 'EXPECTED ' + str(EXPECTED_WIDTH[precision])})")

    (outdir / "build_info.txt").write_text(
        f"precision     = {precision}\n"
        f"opt           = -{opt}   (compile AND link)\n"
        f"shadow_map    = {mapping}   "
        f"({'float->double' if precision == 'fp32' else 'double->x86_fp80'})\n"
        f"eq_truncate   = {eq}\n"
        f"jobs          = -j{jobs}\n"
        f"CXXFLAGS      = {cxxf}\n"
        f"LDFLAGS       = {ldf}\n"
        f"nsan_symbols  = {n_nsan}\n"
        f"wrapped_hooks = {len(WRAP_SYMS)}\n"
        f"branch_sites  = {branch_total}\n"
        f"select_sites  = {select_total}\n"
        f"oos_fcmps     = {oos_total}\n"
        f"shared_fcmps  = {shared_total}\n"
        f"tus           = {len(banners)}\n"
        f"qs_real       = {width} bytes "
        f"(expected {EXPECTED_WIDTH[precision]})\n"
        f"oracle_cell   = "
        + ("fp32_vs_fp64" if precision == "fp32" else "fp64_vs_ld") + "\n"
        f"mpi           = serial (QuickSilver's own stubs in utilsMpi.cc)\n")

    if not ok_w:
        print("  *** WRONG qs_real WIDTH -- precision flag did not take. "
              "Aborting this precision.")
        return None
    return binary


def run(binary, precision, opts, outdir, opt, resume_policy):
    env = base_env()
    resume = "0" if resume_policy == "keep" else "1"
    env["NSAN_OPTIONS"] = (f"halt_on_error=0:resume_after_warning={resume}:"
                           f"disable_warnings=1")

    log = outdir / f"run_{opt}.log"
    bflog = outdir / f"events_{opt}.log"
    env["NSAN_BF_LOG"] = str(bflog)

    print(f"  running (resume_after_warning={resume}) -> "
          f"{log.relative_to(HERE)}")
    args = ["-n", str(opts["n"]),
            "-X", "10", "-Y", "10", "-Z", "10",
            "-x", str(opts["x"]), "-y", str(opts["x"]), "-z", str(opts["x"]),
            "-I", "1", "-J", "1", "-K", "1",
            "-N", str(opts["N"])]
    rc, out = sh(["stdbuf", "-i0", "-o0", "-e0", str(binary)] + args,
                 cwd=binary.parent, env=env, log=log)

    bftext = bflog.read_text() if bflog.exists() else ""
    events, site_totals, totals = parse_events(bftext)

    by_site, kinds = Counter(), Counter()
    for mod, site, k, kind, verdict, pred, native, shadow in events:
        kinds[kind] += 1
        by_site[f"mod{mod}:{kind}{site}"] += 1

    executed = sum(1 for (m, s, kd), v in site_totals.items()
                   if kd == "branch" and v[0] > 0)
    total_exec = sum(v[0] for (m, s, kd), v in site_totals.items()
                     if kd == "branch")

    tallies = []
    for l in out.splitlines():
        m = TALLY_RE.match(l)
        if m:
            tallies.append({"cycle": int(m.group(1)),
                            "absorb": int(m.group(3)),
                            "scatter": int(m.group(4)),
                            "fission": int(m.group(5)),
                            "num_seg": int(m.group(6))})

    return {
        "precision": precision, "opt": opt,
        "n": opts["n"], "mesh": opts["x"], "cycles": opts["N"],
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
        "tallies": tallies,
        "totals": totals,
        "exit_code": rc,
        "log": str(log.relative_to(HERE)),
        "event_log": str(bflog.relative_to(HERE)),
    }


def write_results(precision, r, outdir):
    L = [f"QuickSilver {precision} -- nsan branch flips",
         f"(-{r['opt']} compile+link, shadow map {r['shadow_map']}, "
         f"n={r['n']} mesh={r['mesh']}^3 N={r['cycles']})", ""]
    L.append(f"{r['branch_flips']} branch flips @ {r['locations']} site(s)")
    L.append("")
    if precision == "fp64":
        L.append("every control-flow tally at every configuration tested. So "
                 "every")
    if r["tallies"]:
        L.append("control-flow tallies (should match the uninstrumented run):")
        L.append("  cycle   absorb  scatter  fission  num_seg")
        for t in r["tallies"]:
            L.append(f"  {t['cycle']:5d} {t['absorb']:8d} {t['scatter']:8d}"
                     f" {t['fission']:8d} {t['num_seg']:8d}")
        L.append("")
    if r["out_of_scope_flips"]:
        L.append(f"out-of-scope flips (site_id -1): {r['out_of_scope_flips']}")
    for site, n in list(r["sites"].items())[:40]:
        L.append(f"    {n:8d}  {site}")
    if len(r["sites"]) > 40:
        L.append(f"    ... and {len(r['sites']) - 40} more site(s)")
    L.append("")
    L.append(f"branch sites executed: {r['executed_branch_sites']}   "
             f"total executions: {r['total_branch_executions']:,}")
    t = r["totals"]
    if t:
        L.append(f"runtime totals: {t}")
        if t["unticked"]:
            L.append("")
            L.append("*** unticked > 0 -- DO NOT SCORE THIS RUN.")
        if t["overflow"]:
            L.append("*** site table overflow -- raise NSANBF_TAB_BITS.")
    else:
        L.append("*** no #NSAN_TOTALS line: the shim did not run at exit.")
    L.append("")
    L.append(f"events: {r['event_log']}")
    (outdir / "summary.txt").write_text("\n".join(L))
    (outdir / "summary.json").write_text(json.dumps(r, indent=2))


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("-n", type=int, default=4000,
                    help="particles (default 4000; nothing flips below 2800)")
    ap.add_argument("-x", type=int, default=5, help="mesh cells per axis")
    ap.add_argument("-N", type=int, default=3,
                    help="cycles (default 3; first divergence is at cycle 2)")
    ap.add_argument("--opt", default="O0", choices=["O0", "O1", "O2", "O3"],
                    help="compile AND link level, exported as "
                         "NSAN_BF_OPT_LEVEL. MUST match the oracle's level.")
    ap.add_argument("--resume-policy", choices=["keep", "discard"],
                    default="keep")
    ap.add_argument("--eq-policy", choices=["untruncated", "truncated"],
                    default="untruncated")
    ap.add_argument("--sites-only", action="store_true",
                    help="build and emit the site tables, then stop")
    ap.add_argument("-j", "--jobs", type=int, default=1)
    ap.add_argument("--stack-mb", type=int, default=64)
    ap.add_argument("--cxx", default="clang++",
                    help="plain compiler for the width probe")
    args = ap.parse_args()

    note = set_stack_limit(args.stack_mb)
    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)

    opts = {"n": args.n, "x": args.x, "N": args.N}
    print(f"QuickSilver / nsan   n={args.n} mesh={args.x}^3 N={args.N}  "
          f"-{args.opt}")
    print(f"  stack limit: {note}")
    print(f"  resume policy: {args.resume_policy}   "
          f"eq policy: {args.eq_policy}")
    print(f"  plugin:  {PLUGIN}")
    print(f"  shim:    {SHIM}")
    print(f"  bench:   {BENCH_ROOT}")

    overall = {}
    for precision in args.precision:
        print(f"===== {precision}  (map {SHADOW_MAP[precision]}) =====")
        outdir = RESULT_ROOT / args.opt / precision
        tag = []
        if args.resume_policy != "keep":
            tag.append("resume-" + args.resume_policy)
        if args.eq_policy != "untruncated":
            tag.append("eq-" + args.eq_policy)
        if tag:
            outdir = outdir.with_name(precision + "_" + "_".join(tag))
        outdir.mkdir(parents=True, exist_ok=True)

        binary = build(precision, args.opt, outdir, args.eq_policy,
                       args.jobs, args.cxx)
        if binary is None:
            continue
        if args.sites_only:
            overall[precision] = {"sites_only": True}
            continue

        r = run(binary, precision, opts, outdir, args.opt, args.resume_policy)
        write_results(precision, r, outdir)
        overall[precision] = r

        extra = ""
        if r["out_of_scope_flips"]:
            extra += f"  out-of-scope {r['out_of_scope_flips']}"
        if r["select_flips"]:
            extra += f"  select {r['select_flips']}"
        print(f"  -> {r['branch_flips']} branch flips @ {r['locations']} "
              f"site(s){extra}")
        if r["totals"] and r["totals"]["unticked"]:
            print(f"  *** unticked={r['totals']['unticked']} -- "
                  f"DO NOT SCORE THIS RUN")
        print()

    if not overall:
        print("No results produced.")
        return 1

    if args.sites_only:
        print("===== site tables built; nothing run =====")
        return 0

    print("===== results =====")
    for precision, r in overall.items():
        print(f"  {precision:5s} map={r['shadow_map']}  "
              f"{r['branch_flips']:7d} branch flips @ {r['locations']:3d} "
              f"site(s)   ({r['total_branch_executions']:,} executions)")

    if "fp32" in overall and "fp64" in overall:
        t32 = overall["fp32"].get("tallies")
        t64 = overall["fp64"].get("tallies")
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

    print(f"\n{BENCH_NAME}/results/{args.opt}/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())