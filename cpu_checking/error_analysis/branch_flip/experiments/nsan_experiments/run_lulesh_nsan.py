#!/usr/bin/env python3
"""
run_lulesh_nsan.py -- build LULESH under nsan with the NsanBFSites plugin and
run it, fp32 and fp64 separately.

    ./run_lulesh_nsan.py                     # both precisions
    ./run_lulesh_nsan.py -p fp32 -s 10 -i 50
    ./run_lulesh_nsan.py --sites-only        # build tables, verify, stop
    ./run_lulesh_nsan.py --resume-policy discard --eq-policy truncated

nsan shadows the same binary: lulesh_fp32 with mapping dqq (float->double)
is the fp32-vs-fp64 cell; lulesh_fp64 with dlq (double->x86_fp80) is the
fp64-vs-ld cell. The default dqq would shadow double as fp128, overshooting
the oracle.

Policy axes (nsan has no eta): resume_after_warning discards the shadow after
any value warning and hides downstream flips; -nsan-truncate-fcmp-eq rounds
the shadow before == / != and creates disagreements the oracle does not see.
Both default to the measurement position here; run both to report the delta.

Sets RLIMIT_STACK itself: under an unlimited stack ld.so lands inside nsan's
MAP_FIXED shadow range and every nsan binary segfaults before main.

Event records (NSAN_BF_LOG):
    #NSAN_EVENT mod=<id> site=<id> k=<occ> kind=<branch|select|oos>
                verdict=FLIP pred=<n> native=<0|1> shadow=<0|1>
    #NSAN_SITE  <mod> <site> <kind> <executions> <flagged>
    #NSAN_TOTALS events=.. out_of_scope=.. unticked=.. overflow=..
nsan cannot abstain, so every verdict is FLIP. unticked must be 0.
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
FPC_ROOT = Path(os.environ.get("FPC_ROOT", Path(__file__).resolve().parents[5]))
BRANCH_FLIP = FPC_ROOT / "cpu_checking/error_analysis/branch_flip"
BENCH_ROOT = Path(os.environ.get("BENCH_ROOT", BRANCH_FLIP / "benchmarks/lulesh"))

NSAN_BF = Path(os.environ.get("NSAN_BF_HOME", Path(__file__).resolve().parent / "nsan"))
PLUGIN = NSAN_BF / "plugin" / "libNsanBFSites.so"
SHIM = NSAN_BF / "runtime" / "libnsan_bf.a"

CONDA_LIB = os.environ.get("CONDA_PREFIX", "") and \
    os.path.join(os.environ["CONDA_PREFIX"], "lib")

HERE = Path(__file__).resolve().parent
BENCH_NAME = "lulesh"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

SHADOW_MAP = {"fp32": "dqq", "fp64": "dlq"}

MEM_NOTE = "nsan shadow memory is 2x per value plus a type-tag byte per byte"

EXPECTED_SITES = 75

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


def sh(cmd, cwd=None, env=None, log=None):
    p = subprocess.run(cmd, cwd=cwd, env=env, shell=isinstance(cmd, str),
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True, errors="replace")
    if log:
        log.write_text(p.stdout)
    return p.returncode, p.stdout


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
    """module basename -> (module_id, branch_sites, select_sites, oos, shared)"""
    out = {}
    for line in text.splitlines():
        m = BANNER_RE.match(line.strip())
        if m:
            out[os.path.basename(m.group(1))] = (
                int(m.group(2)), int(m.group(3)), int(m.group(4)),
                int(m.group(5)), int(m.group(6) or 0))
    return out


def parse_events(text):
    """-> (events, site_totals, totals)"""
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


def build(precision, opt, outdir, eq_policy, jobs=1):
    src = BENCH_ROOT / f"lulesh_{precision}"
    dst = BUILD_ROOT / opt / f"lulesh_{precision}"
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
    for junk in list(dst.glob("*.o")) + list(dst.glob("lulesh2.0")):
        junk.unlink()

    env = os.environ.copy()
    if CONDA_LIB:
        env["LD_LIBRARY_PATH"] = CONDA_LIB + ":" + env.get("LD_LIBRARY_PATH", "")
    env["NSAN_BF_OPT_LEVEL"] = opt

    flag = "-DLULESH_FP32" if precision == "fp32" else ""
    mapping = SHADOW_MAP[precision]
    eq = "1" if eq_policy == "truncated" else "0"

    cxx = (f"clang++ -DUSE_MPI=0 {flag} -fsanitize=numerical "
           f"-fpass-plugin={PLUGIN} "
           f"-mllvm -nsan-shadow-type-mapping={mapping} "
           f"-mllvm -nsan-truncate-fcmp-eq={eq}").strip()
    cxxflags = f"-g -{opt} -I. -std=c++11"
    wrap = " ".join(f"-Wl,--wrap={s}" for s in WRAP_SYMS)
    ldflags = f"-g -{opt} -fsanitize=numerical {SHIM} {wrap}"

    cmd = ["make", f"-j{jobs}", f"CXX={cxx}", f"CXXFLAGS={cxxflags}",
           f"LDFLAGS={ldflags}"]
    print(f"  building ({precision}, -{opt}, map={mapping}, "
          f"eq-trunc={eq}, -j{jobs})")
    rc, _ = sh(cmd, cwd=dst, env=env, log=outdir / "build.log")

    binary = dst / "lulesh2.0"
    if rc != 0 or not binary.exists():
        print(f"  BUILD FAILED -- see {outdir/'build.log'}")
        for line in (outdir / "build.log").read_text().splitlines():
            if "error" in line.lower():
                print("   ", line[:140])
                break
        return None

    # --- gate 1: instrumented at all? ---
    _, nm_out = sh(["nm", str(binary)])
    n_nsan = sum(1 for l in nm_out.splitlines() if "__nsan_" in l)
    has_tick = any("__nsan_bf_tick" in l for l in nm_out.splitlines())
    print(f"  __nsan_* symbols = {n_nsan}   tick present = {has_tick}")
    if n_nsan == 0 or not has_tick:
        return None

    # --- gate 2: did every wrap take? ---
    defined = set()
    for l in nm_out.splitlines():
        parts = l.split()
        if len(parts) >= 2 and parts[-1].startswith("__wrap___nsan_fcmp_fail"):
            defined.add(parts[-1])
    missing = [s for s in WRAP_SYMS if f"__wrap_{s}" not in defined]
    if missing:
        print(f"  *** WRAP NOT APPLIED for: {', '.join(missing)}")
        return None
    print(f"  wrapped hooks = {len(WRAP_SYMS)}/{len(WRAP_SYMS)}")

    # --- gate 3: the site census ---
    build_log = (outdir / "build.log").read_text()
    banners = parse_banners(build_log)
    if not banners:
        print("  *** no [NSanBF] banner in the build log -- the plugin did not")
        return None
    branch_total = sum(b[1] for b in banners.values())
    select_total = sum(b[2] for b in banners.values())
    oos_total = sum(b[3] for b in banners.values())
    shared_total = sum(b[4] for b in banners.values())
    print(f"  branch sites = {branch_total} across {len(banners)} TU(s)"
          f"   (selects {select_total}, out-of-scope fcmps {oos_total})")
    for mod in sorted(banners):
        print(f"      {mod:<20s} mod {banners[mod][0]:<11d} "
              f"{banners[mod][1]:5d} branch  {banners[mod][2]:4d} select")
    if EXPECTED_SITES is not None and branch_total != EXPECTED_SITES:
        print(f"  *** WARNING: expected {EXPECTED_SITES} FP-controlled branch")
        print(f"      sites, got {branch_total}. site_id is a positional")
    if shared_total:
        print("      space's event counts are a lower bound there.")

    # --- collect the manifests ---
    manifests = sorted(dst.glob("*.nsansites")) + \
        sorted(dst.glob("*.nsanselsites"))
    if manifests:
        mdir = outdir / "nsansites"
        mdir.mkdir(parents=True, exist_ok=True)
        for m in manifests:
            shutil.copy2(m, mdir / m.name)
        print(f"  collected {len(manifests)} site table(s) -> "
              f"{mdir.relative_to(HERE)}")
    else:
        print("  *** no .nsansites tables written -- check_sites.py will have")

    (outdir / "build_info.txt").write_text(
        f"precision     = {precision}\n"
        f"opt           = -{opt}   (compile AND link)\n"
        f"shadow_map    = {mapping}   "
        f"({'float->double' if precision == 'fp32' else 'double->x86_fp80'})\n"
        f"eq_truncate   = {eq}\n"
        f"jobs          = -j{jobs}\n"
        f"CXX           = {cxx}\n"
        f"CXXFLAGS      = {cxxflags}\n"
        f"LDFLAGS       = {ldflags}\n"
        f"nsan_symbols  = {n_nsan}\n"
        f"wrapped_hooks = {len(WRAP_SYMS)}\n"
        f"branch_sites  = {branch_total}\n"
        f"select_sites  = {select_total}\n"
        f"oos_fcmps     = {oos_total}\n"
        f"shared_fcmps  = {shared_total}\n"
        f"sites_by_tu   = "
        + ", ".join(f"{m}={banners[m][1]}" for m in sorted(banners)) + "\n"
        f"module_ids    = "
        + ", ".join(f"{m}={banners[m][0]}" for m in sorted(banners)) + "\n"
        f"oracle_cell   = "
        + ("fp32_vs_fp64" if precision == "fp32" else "fp64_vs_ld") + "\n"
        f"assumption    = the oracle's high-precision leg is a pure precision\n"
        f"                substitution of this source; nsan shadows the SAME\n"
        f"                binary rather than comparing two builds\n"
        f"note          = {MEM_NOTE}\n")
    return binary


def run(binary, precision, size, iters, outdir, opt, resume_policy):
    env = os.environ.copy()
    if CONDA_LIB:
        env["LD_LIBRARY_PATH"] = CONDA_LIB + ":" + env.get("LD_LIBRARY_PATH", "")

    resume = "0" if resume_policy == "keep" else "1"
    env["NSAN_OPTIONS"] = (f"halt_on_error=0:resume_after_warning={resume}:"
                           f"disable_warnings=1")

    log = outdir / f"run_{opt}.log"
    bflog = outdir / f"events_{opt}.log"
    env["NSAN_BF_LOG"] = str(bflog)

    print(f"  running (resume_after_warning={resume}) -> "
          f"{log.relative_to(HERE)}")
    rc, out = sh(["stdbuf", "-i0", "-o0", "-e0", str(binary),
                  "-s", str(size), "-i", str(iters), "-p"],
                 cwd=binary.parent, env=env, log=log)

    bftext = bflog.read_text() if bflog.exists() else ""
    events, site_totals, totals = parse_events(bftext)

    by_site = Counter()
    kinds = Counter()
    for mod, site, k, kind, verdict, pred, native, shadow in events:
        kinds[kind] += 1
        by_site[f"mod{mod}:{kind}{site}"] += 1

    scoreable = kinds["branch"]
    executed = sum(1 for (m, s, kd), v in site_totals.items()
                   if kd == "branch" and v[0] > 0)
    total_exec = sum(v[0] for (m, s, kd), v in site_totals.items()
                     if kd == "branch")

    return {
        "precision": precision,
        "opt": opt,
        "size": size,
        "iterations": iters,
        "shadow_map": SHADOW_MAP[precision],
        "resume_after_warning": resume,
        "flips": len(events),
        "branch_flips": kinds["branch"],
        "select_flips": kinds["select"],
        "out_of_scope_flips": kinds["oos"],
        "scoreable_flips": scoreable,
        "locations": len(by_site),
        "sites": dict(by_site.most_common()),
        "executed_branch_sites": executed,
        "total_branch_executions": total_exec,
        "totals": totals,
        "exit_code": rc,
        "log": str(log.relative_to(HERE)),
        "event_log": str(bflog.relative_to(HERE)),
    }


def write_results(precision, r, outdir):
    L = [f"LULESH {precision} -- nsan branch flips",
         f"(-{r['opt']} compile+link, shadow map {r['shadow_map']}, "
         f"s={r['size']}, i={r['iterations']})", ""]
    L.append(f"{r['branch_flips']} branch flips @ {r['locations']} site(s)")
    L.append("")
    L.append("opposite end of the abstention axis from FPChecker.")
    L.append("")
    if r["out_of_scope_flips"]:
        L.append(f"out-of-scope flips (site_id -1): {r['out_of_scope_flips']}")
    if r["select_flips"]:
        L.append("    CONSTRUCTION; recorded, not scored against the branch")
        L.append("    oracle.")
        L.append("")
    for site, n in r["sites"].items():
        L.append(f"    {n:8d}  {site}")
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
    ap.add_argument("-s", "--size", type=int, default=5)
    ap.add_argument("-i", "--iter", type=int, default=20)
    ap.add_argument("--opt", default="O0", choices=["O0", "O1", "O2", "O3"],
                    help="optimisation level, compile AND link, exported as "
                         "NSAN_BF_OPT_LEVEL so it lands in every site table. "
                         "Site ids correspond across levels (the walk runs at "
                         "PipelineStartEP); occurrence indices do not. This "
                         "MUST match the level the oracle traces were made at.")
    ap.add_argument("--resume-policy", choices=["keep", "discard"],
                    default="keep",
                    help="what nsan does with the shadow after a value "
                         "warning. keep (default) sets resume_after_warning=0 "
                         "and preserves the shadow, so flips downstream of a "
                         "warning are still visible. discard is nsan's shipped "
                         "default and suppresses them. Run both: the delta is "
                         "the cost of the default.")
    ap.add_argument("--eq-policy", choices=["untruncated", "truncated"],
                    default="untruncated",
                    help="-nsan-truncate-fcmp-eq. truncated is nsan's default "
                         "and rounds the shadow to app precision before == and "
                         "!=, which CREATES disagreements the untruncated "
                         "comparison does not have. Those are false positives "
                         "against a trajectory oracle.")
    ap.add_argument("-j", "--jobs", type=int, default=1)
    ap.add_argument("--sites-only", action="store_true",
                    help="build and emit the site tables, then stop. Use this "
                         "to verify the enumeration against brtrace BEFORE "
                         "spending a run on it: if the ordinals disagree, "
                         "every number the run produces is unjoinable.")
    ap.add_argument("--stack-mb", type=int, default=64,
                    help="RLIMIT_STACK for the child. Must be finite: see the "
                         "STACK LIMIT note. Raise if LULESH overflows.")
    args = ap.parse_args()

    note = set_stack_limit(args.stack_mb)
    print(f"LULESH / nsan   s={args.size} i={args.iter} -{args.opt}")
    print(f"  stack limit: {note}")
    print(f"  resume policy: {args.resume_policy}   "
          f"eq policy: {args.eq_policy}")
    print(f"  plugin:  {PLUGIN}")
    print(f"  shim:    {SHIM}")
    print(f"  bench:   {BENCH_ROOT}\n")

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)

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

        binary = build(precision, args.opt, outdir, args.eq_policy, args.jobs)
        if binary is None:
            continue
        if args.sites_only:
            overall[precision] = {"sites_only": True}
            continue
        r = run(binary, precision, args.size, args.iter, outdir, args.opt,
                args.resume_policy)
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
              f"{r['branch_flips']:6d} branch flips @ {r['locations']} site(s)"
              f"   ({r['total_branch_executions']:,} executions)")
    print(f"\n{BENCH_NAME}/results/{args.opt}/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
