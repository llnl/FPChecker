#!/usr/bin/env python3
"""
run_lulesh_eftsan.py -- build LULESH under EFTSanitizer branch-flip
instrumentation and run it, fp32 and fp64 separately.

    ./run_lulesh_eftsan.py                    # both precisions
    ./run_lulesh_eftsan.py -p fp32 -s 10 -i 50
    ./run_lulesh_eftsan.py --no-run           # build and gate only

Pipeline: clang++ -O0 -emit-llvm per TU -> llvm-link -> opt -eftsan ->
clang++ -leftsanitizer. The merge must precede instrumentation (per-TU
instrumentation corrupts argument shadows at call sites). -O0 only; EFTSan's
threshold (ERRORTHRESHOLD=45) is compiled into the runtime, so there is one
run per precision and no sweep.

The pass writes eftsan_sites.csv (module_id, site_id, file, line, col); the
runtime writes eftsan_events.csv (one row per verdict, with occurrence index
k) and eftsan_totals.csv (per-site executions). site_id ordinals are
EFTSan's own; the cross-tool join key is (module_id, line, col).

Env: EFT_HOME, EFT_SETUP, BENCH_ROOT, EFT_WORK_ROOT.
"""

import argparse
import csv
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
EFT_HOME = Path(os.environ.get("EFT_HOME", HERE.parents[5] / "EFTSanitizer"))
SETUP_SH = Path(os.environ.get(
    "EFT_SETUP", HERE.parents[2] / "env_setup" / "activate_eftsan_env.sh"))
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT", HERE.parent.parent / "benchmarks" / "lulesh"))

BENCH_NAME = "lulesh"
WORK_ROOT = Path(os.environ.get("EFT_WORK_ROOT", HERE)) / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build" / "O0"
RESULT_ROOT = WORK_ROOT / "results" / "O0"

SRCS = ["lulesh.cc", "lulesh-comm.cc", "lulesh-init.cc",
        "lulesh-util.cc", "lulesh-viz.cc"]
PRECISION_FLAG = {"fp32": "-DLULESH_FP32", "fp64": ""}
EXPECTED_MODULE_IDS = {"lulesh.cc": 1179233406, "lulesh-util.cc": 594952751}

ERROR_LINE_RE = re.compile(r"\berror:")
UNDEF_RE = re.compile(r"undefined reference to [`']([^'\"]+)'")
TOTAL_RE = re.compile(r"Total branch flips found\s+(\d+)")


def fnv1a_32(name):
    h = 2166136261
    for c in name.encode():
        h = ((h ^ c) * 16777619) & 0xFFFFFFFF
    return h


def sh(cmd, cwd=None, env=None, log=None, append=True):
    p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, text=True, errors="replace")
    if log:
        with open(log, "a" if append else "w") as fh:
            fh.write("$ " + " ".join(map(str, cmd)) + "\n")
            fh.write(p.stdout + "\n")
    return p.returncode, p.stdout


def sh_split(cmd, cwd=None, env=None):
    p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE, text=True, errors="replace")
    return p.returncode, p.stdout, p.stderr


def eftsan_env():
    """Source the activate script under bash and harvest its environment."""
    if not SETUP_SH.exists():
        print(f"FATAL: no setup script at {SETUP_SH}")
        print("       set EFT_SETUP=/path/to/activate_eftsan_env.sh")
        return None
    p = subprocess.run(
        ["bash", "-c",
         f"source {shlex.quote(str(SETUP_SH))} >/dev/null 2>&1 && env -0"],
        stdout=subprocess.PIPE, text=True)
    if p.returncode != 0:
        print(f"FATAL: sourcing {SETUP_SH} failed")
        return None
    env = {}
    for kv in p.stdout.split("\0"):
        if "=" in kv:
            k, v = kv.split("=", 1)
            env[k] = v
    return env


NEEDED_LIBS = ["libmpfr.so", "libgmp.so"]


def find_lib_dirs(env):
    roots = []
    for prefix in (env.get("CONDA_PREFIX"), os.environ.get("CONDA_PREFIX")):
        if prefix:
            roots.append(Path(prefix) / "lib")
    roots += [Path(d) for d in env.get("LD_LIBRARY_PATH", "").split(":") if d]
    roots += [Path(d) for d in
              os.environ.get("LD_LIBRARY_PATH", "").split(":") if d]
    roots += [EFT_HOME / "runtime" / "obj",
              Path("/usr/lib64"), Path("/usr/lib/x86_64-linux-gnu")]

    dirs, missing = [], []
    for lib in NEEDED_LIBS:
        hit = None
        for r in roots:
            try:
                if any(r.glob(lib + "*")):
                    hit = r
                    break
            except OSError:
                continue
        if hit is None:
            missing.append(lib)
        elif hit not in dirs:
            dirs.append(hit)
    return dirs, missing


def with_lib_path(env, dirs):
    env = dict(env)
    existing = env.get("LD_LIBRARY_PATH", "")
    prefix = ":".join(str(d) for d in dirs)
    env["LD_LIBRARY_PATH"] = (prefix + ":" + existing) if existing else prefix
    return env


def check_tools(env):
    missing = []
    for t in ("clang++", "opt", "llvm-link", "llvm-dis"):
        rc, _ = sh([t, "--version"], env=env)
        if rc != 0:
            missing.append(t)
    return missing


def read_sites(path):
    """{(kind, module_id, site_id): {file, line, col, function, n_fcmp}}"""
    sites = {}
    with open(path) as fh:
        lines = [l.rstrip("\n") for l in fh if l.strip()]
    if not lines:
        return sites
    hdr = lines[0].lstrip("# ").split(",")
    for l in lines[1:]:
        if l.startswith("#"):
            continue
        rec = dict(zip(hdr, l.split(",")))
        key = (rec["kind"], int(rec["module_id"]), int(rec["site_id"]))
        sites[key] = {
            "file": rec["file"],
            "line": int(rec["line"]),
            "col": int(rec["col"]),
            "function": rec["function"],
            "n_fcmp": int(rec["n_fcmp"]),
        }
    return sites


def read_events(path):
    """[(kind, module_id, site_id, k, verdict)]"""
    out = []
    if not Path(path).exists():
        return out
    with open(path) as fh:
        for rec in csv.DictReader(fh):
            out.append((rec["kind"], int(rec["module_id"]),
                        int(rec["site_id"]), int(rec["k"]), rec["verdict"]))
    return out


def read_totals(path):
    """{(kind, module_id, site_id): (executions, flips, nonfinite)}"""
    out = {}
    if not Path(path).exists():
        return out
    with open(path) as fh:
        for rec in csv.DictReader(fh):
            out[(rec["kind"], int(rec["module_id"]), int(rec["site_id"]))] = (
                int(rec["executions"]), int(rec["flips"]), int(rec["nonfinite"]))
    return out


def verify_module_ids(sites):
    """Check the FNV-1a-32 convention and basename collisions."""
    seen, bad = {}, []
    for (_kind, mod, _sid), s in sites.items():
        seen.setdefault(s["file"], set()).add(mod)
    for fname, mods in sorted(seen.items()):
        for mod in mods:
            if fnv1a_32(fname) != mod:
                bad.append((fname, mod, fnv1a_32(fname)))
        if len(mods) > 1:
            bad.append((fname, sorted(mods), "one basename, several ids"))
    for fname, expected in EXPECTED_MODULE_IDS.items():
        if fname in seen and expected not in seen[fname]:
            bad.append((fname, sorted(seen[fname]), expected))
    by_id = defaultdict(set)
    for fname, mods in seen.items():
        for m in mods:
            by_id[m].add(fname)
    for mod, names in by_id.items():
        if len(names) > 1:
            bad.append((sorted(names), mod, "basename collision"))
    return seen, bad


VECTOR_TYPE_RE = re.compile(r"<\d+ x (?:float|double|half)>")


def diagnose_vector_types(merged_bc, env, limit=6):
    """Vector-typed values in the module and the functions holding them."""
    ll = merged_bc.with_suffix(".diag.ll")
    rc, _ = sh(["llvm-dis", str(merged_bc), "-o", str(ll)], env=env)
    if rc != 0 or not ll.exists():
        return 0, []
    hits, funcs, current = 0, [], None
    try:
        with open(ll, errors="replace") as fh:
            for line in fh:
                if line.startswith("define"):
                    m = re.search(r"@([A-Za-z0-9_$.]+)\(", line)
                    current = m.group(1) if m else None
                if VECTOR_TYPE_RE.search(line):
                    hits += 1
                    if current and current not in funcs:
                        funcs.append(current)
    except OSError:
        return 0, []
    finally:
        try:
            ll.unlink()
        except OSError:
            pass
    return hits, funcs[:limit]


def write_failure_record(precision, outdir, stage, reason, detail, extra=None):
    """Record a build the tool refused as a result, not a missing run."""
    rec = {
        "precision": precision,
        "opt": "O0",
        "status": "tool_failure",
        "failed_stage": stage,
        "reason": reason,
        "detail": detail,
        "flips": None,
        "locations": None,
        "sites": {},
        "exit_code": None,
    }
    if extra:
        rec.update(extra)
    lines = [f"{precision} -- EFTSanitizer: NO RESULT",
             "", f"TOOL FAILURE at {stage}.", "", f"  {reason}", ""]
    for l in detail.splitlines():
        lines.append("  " + l)
    (outdir / "summary.txt").write_text("\n".join(lines) + "\n")
    (outdir / "summary.json").write_text(json.dumps([rec], indent=2))
    return rec


def build(precision, outdir, std, link_override, keep_bc, env, lib_dirs):
    src = BENCH_ROOT / f"lulesh_{precision}"
    dst = BUILD_ROOT / f"lulesh_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        return None
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for junk in list(dst.glob("*.o")) + list(dst.glob("*.bc")) + \
            list(dst.glob("lulesh2.0")):
        junk.unlink()

    pass_so = EFT_HOME / "llvm_pass/build/EFTSan/libEFTSanitizer.so"
    runtime = EFT_HOME / "runtime/obj/libeftsanitizer.so"
    for p, what in ((pass_so, "pass plugin"), (runtime, "runtime")):
        if not p.exists():
            print(f"  FATAL: no {what} at {p}")
            return None

    missing = [s for s in SRCS if not (dst / s).exists()]
    if missing:
        print(f"  FATAL: missing sources: {', '.join(missing)}")
        return None

    log = outdir / "build.log"
    if log.exists():
        log.unlink()

    cflags = [f"-std={std}", "-O0", "-g", "-w", "-DUSE_MPI=0", "-I.",
              "-fno-vectorize", "-fno-slp-vectorize"]
    if PRECISION_FLAG[precision]:
        cflags.append(PRECISION_FLAG[precision])

    # ---- 1. per-TU bitcode ------------------------------------------
    print(f"  compiling {len(SRCS)} TUs to bitcode (-O0, {std})")
    bcs = []
    for s in SRCS:
        bc = dst / (Path(s).stem + ".bc")
        rc, out = sh(["clang++"] + cflags +
                     ["-emit-llvm", "-c", s, "-o", str(bc)],
                     cwd=dst, env=env, log=log)
        if rc != 0 or not bc.exists():
            print(f"  COMPILE FAILED on {s} -- see {log}")
            for line in out.splitlines():
                if ERROR_LINE_RE.search(line):
                    print("   ", line[:160])
                    break
            return None
        bcs.append(bc)

    # ---- 2. merge ---------------------------------------------------
    merged = dst / "lulesh_merged.bc"
    print(f"  llvm-link {len(bcs)} modules -> {merged.name}")
    link_cmd = ["llvm-link"] + [str(b) for b in bcs]
    if link_override:
        link_cmd.append("-override")
    rc, out = sh(link_cmd + ["-o", str(merged)], cwd=dst, env=env, log=log)
    if rc != 0 or not merged.exists():
        print(f"  llvm-link FAILED -- see {log}")
        for line in out.splitlines()[:4]:
            print("   ", line[:150])
        return None

    # ---- 3. instrument ----------------------------------------------
    sites_csv = outdir / "eftsan_sites.csv"
    opt_env = dict(env)
    opt_env["EFTSAN_SITES"] = str(sites_csv)

    opt_bc = dst / "lulesh.opt.bc"
    print("  opt -eftsan on the merged module")
    rc, out = sh(["opt", "-load", str(pass_so), "-eftsan",
                  str(merged), "-o", str(opt_bc)],
                 cwd=dst, env=opt_env, log=log)
    if rc != 0 or not opt_bc.exists() or opt_bc.stat().st_size == 0:
        print(f"  INSTRUMENTATION FAILED -- see {log}")
        for l in [x for x in out.splitlines() if x.strip()][-6:]:
            print("     ", l[:150])
        if "vector" in out.lower():
            nvec, vfuncs = diagnose_vector_types(merged, env)
            print(f"    {nvec} vector-typed value(s)")
            for f in vfuncs:
                print(f"      {f}")
        return None

    # ---- gate 2: the manifest exists --------------------------------
    if not sites_csv.exists() or sites_csv.stat().st_size == 0:
        print(f"  *** NO SITE MANIFEST at {sites_csv}")
        return None
    sites = read_sites(sites_csv)
    n_branch = sum(1 for k in sites if k[0] == "branch")
    n_select = sum(1 for k in sites if k[0] == "select")
    print(f"  site manifest: {n_branch} branch, {n_select} select")

    # ---- gate 3: the hash convention holds --------------------------
    seen, bad = verify_module_ids(sites)
    if bad:
        print("  *** MODULE ID CHECK FAILED:")
        for entry in bad[:6]:
            print(f"      {entry}")
        return None
    print(f"  module ids verified: "
          f"{', '.join(f'{f}={sorted(m)[0]}' for f, m in sorted(seen.items())[:4])}"
          f"{' ...' if len(seen) > 4 else ''}")

    multi = [k for k, s in sites.items() if s["n_fcmp"] > 1]
    if multi:
        print(f"  note: {len(multi)} site(s) have n_fcmp > 1")

    # ---- 4. link ----------------------------------------------------
    binary = dst / f"lulesh_{precision}.eftsan"
    print("  linking against the EFTSan runtime")
    link = ["clang++", "-O0", "-g", str(opt_bc),
            f"-L{EFT_HOME/'runtime/obj'}", "-leftsanitizer",
            "-lm", "-lmpfr", "-lgmp", "-lstdc++",
            f"-Wl,-rpath,{EFT_HOME/'runtime/obj'}"]
    for d in lib_dirs:
        link += [f"-L{d}", f"-Wl,-rpath,{d}"]
    link += ["-o", str(binary)]
    rc, out = sh(link, cwd=dst, env=env, log=log)
    if rc != 0 or not binary.exists():
        print(f"  LINK FAILED -- see {log}")
        syms = Counter(UNDEF_RE.findall(out))
        if syms:
            print(f"    {sum(syms.values())} undefined reference(s), "
                  f"{len(syms)} distinct symbol(s):")
            for sym, n in syms.most_common(8):
                print(f"      {n:6d}  {sym}")
        else:
            for line in out.splitlines():
                if ERROR_LINE_RE.search(line):
                    print("   ", line[:160])
                    break
        return None

    # ---- gate 1 -----------------------------------------------------
    _, nm_out = sh(["nm", str(binary)], env=env)
    nsym = sum(1 for l in nm_out.splitlines() if "eftsan" in l.lower())
    print(f"  eftsan symbol count = {nsym}")

    (outdir / "build_info.txt").write_text(
        f"benchmark    = LULESH\n"
        f"precision    = {precision}\n"
        f"opt          = -O0\n"
        f"compiler     = clang++ (LLVM 10), -std={std}\n"
        f"TUs compiled = {len(bcs)}  ({' '.join(SRCS)})\n"
        f"pass         = {pass_so} -eftsan\n"
        f"runtime      = {runtime}\n"
        f"cflags       = {' '.join(cflags)}\n"
        f"link libs    = -leftsanitizer -lm -lmpfr -lgmp -lstdc++\n"
        f"eftsan_syms  = {nsym}\n"
        f"branch_sites = {n_branch}\n"
        f"select_sites = {n_select}\n"
        f"multi_fcmp   = {len(multi)}\n"
        f"modules      = {len(seen)}  ({', '.join(sorted(seen))})\n"
        f"openmp       = disabled\n")

    _, ldd_out = sh(["ldd", str(binary)], env=env)
    unfound = [l.strip() for l in ldd_out.splitlines() if "not found" in l]
    if unfound:
        print("  *** LOADER GATE FAILED -- the binary cannot start:")
        for l in unfound[:4]:
            print("     ", l[:120])
        return None
    print("  loader gate: all shared libraries resolve")

    if nsym == 0:
        print("  *** UNINSTRUMENTED BINARY -- aborting this precision.")
        return None

    if not keep_bc:
        for b in bcs:
            b.unlink(missing_ok=True)

    local_files = {p.name for p in dst.iterdir()
                   if p.suffix in (".cc", ".h", ".hpp", ".hh")}
    return binary, sites, local_files


def run(binary, precision, size, iters, outdir, sites, local_files, env):
    events_csv = outdir / "eftsan_events.csv"
    totals_csv = outdir / "eftsan_totals.csv"
    run_env = dict(env)
    run_env["EFTSAN_BF_OUT"] = str(events_csv)
    run_env["EFTSAN_BF_TOTALS"] = str(totals_csv)

    print(f"  running -s {size} -i {iters}")
    rc, out, err = sh_split(["stdbuf", "-i0", "-o0", "-e0", str(binary),
                             "-s", str(size), "-i", str(iters), "-p"],
                            cwd=binary.parent, env=run_env)
    (outdir / "run_O0.stdout").write_text(out)
    (outdir / "run_O0.stderr").write_text(err)

    src_errlog = binary.parent / "error.log"
    errlog_text = ""
    if src_errlog.exists():
        errlog_text = src_errlog.read_text(errors="replace")
        (outdir / "error.log").write_text(errlog_text)

    # ---- gate 4 -----------------------------------------------------
    if not events_csv.exists():
        print(f"  *** NO EVENT LOG at {events_csv}")

    events = read_events(events_csv)
    totals = read_totals(totals_csv)

    flips = Counter()
    nonfinite = Counter()
    ks = defaultdict(list)
    for kind, mod, sid, k, verdict in events:
        key = (kind, mod, sid)
        ks[key].append(k)
        if verdict in ("FLIP", "FLIP_NONFINITE"):
            flips[key] += 1
        if verdict in ("FLIP_NONFINITE", "NONFINITE"):
            nonfinite[key] += 1

    parsed = sum(flips.values())

    # ---- gate 5: the accounting closes ------------------------------
    tm = TOTAL_RE.search(errlog_text)
    runtime_total = int(tm.group(1)) if tm else None
    out_of_scope = (runtime_total - parsed) if runtime_total is not None else None

    totals_flips = sum(v[1] for v in totals.values())
    if totals_flips != parsed:
        print(f"  *** ACCOUNTING MISMATCH: events.csv has {parsed} FLIP rows "
              f"but totals.csv sums to {totals_flips}")

    named, foreign, unmapped = Counter(), Counter(), Counter()
    site_detail = {}
    for key, n in flips.most_common():
        s = sites.get(key)
        if s is None:
            unmapped[f"{key[0]}:mod{key[1]}:site{key[2]}"] = n
            continue
        loc = f"{s['file']}:{s['line']}"
        named[loc] += n
        if s["file"] not in local_files:
            foreign[loc] += n
        site_detail[loc] = {
            "kind": key[0],
            "module_id": key[1],
            "site_id": key[2],
            "file": s["file"],
            "line": s["line"],
            "col": s["col"],
            "function": s["function"],
            "n_fcmp": s["n_fcmp"],
            "flips": n,
            "nonfinite": nonfinite.get(key, 0),
            "executions": totals.get(key, (None, None, None))[0],
            "first_flip_k": min(ks[key]) if ks.get(key) else None,
        }

    if unmapped:
        print(f"  *** {len(unmapped)} event key(s) absent from the manifest")

    n_nonfinite = sum(nonfinite.values())
    if n_nonfinite:
        print(f"  {n_nonfinite} execution(s) with a non-finite shadow")

    if rc != 0:
        print(f"  *** non-zero exit ({rc})")
        for line in (err or out).splitlines()[:3]:
            if line.strip():
                print("     ", line[:150])

    return {
        "benchmark": "lulesh",
        "precision": precision,
        "opt": "O0",
        "threshold": 45,
        "branch_sites": sum(1 for k in sites if k[0] == "branch"),
        "select_sites": sum(1 for k in sites if k[0] == "select"),
        "size": size,
        "iterations": iters,
        "flips": parsed,
        "nonfinite": n_nonfinite,
        "runtime_total": runtime_total,
        "out_of_scope_flips": out_of_scope,
        "locations": len(named),
        "sites": dict(named.most_common()),
        "site_detail": site_detail,
        "foreign_sites": dict(foreign),
        "foreign_flips": sum(foreign.values()),
        "unmapped_sites": dict(unmapped),
        "unmapped_flips": sum(unmapped.values()),
        "exit_code": rc,
        "events_csv": str(events_csv.relative_to(HERE)),
        "totals_csv": str(totals_csv.relative_to(HERE)),
        "sites_csv": str((outdir / "eftsan_sites.csv").relative_to(HERE)),
        "log": str((outdir / "run_O0.stderr").relative_to(HERE)),
        "stdout_log": str((outdir / "run_O0.stdout").relative_to(HERE)),
    }


def write_results(precision, records, outdir):
    r0 = records[0]
    lines = [f"LULESH {precision} -- EFTSanitizer branch flips",
             f"(-{r0['opt']}, merged module, s={r0['size']}, "
             f"i={r0['iterations']})", ""]
    for r in records:
        own = r["flips"] - r["foreign_flips"] - r["unmapped_flips"]
        oos = ("n/a" if r["out_of_scope_flips"] is None
               else str(r["out_of_scope_flips"]))
        lines.append(f"{r['flips']} in-scope flips @ {r['locations']} loc"
                     f"   [in-TU {own}, STL {r['foreign_flips']}]")
        lines.append(f"{r['nonfinite']} with non-finite shadow"
                     f"   |   {oos} out-of-scope")
    lines += ["", f"{r0['branch_sites']} branch sites, "
                  f"{r0['select_sites']} select sites", ""]
    for r in records:
        lines.append(f"--- {r['opt']} : {r['flips']} flips @ "
                     f"{r['locations']} loc ---")
        for site, n in r["sites"].items():
            d = r["site_detail"].get(site, {})
            ex = d.get("executions")
            k0 = d.get("first_flip_k")
            extra = []
            if ex is not None:
                extra.append(f"of {ex} exec")
            if k0 is not None:
                extra.append(f"first k={k0}")
            if d.get("nonfinite"):
                extra.append(f"{d['nonfinite']} non-finite")
            tail = ("   (" + ", ".join(extra) + ")") if extra else ""
            lines.append(f"    {n:9d}  {site}  [{d.get('function','?')}]{tail}")
        if r["runtime_total"] is not None:
            lines.append(f"    runtime total: {r['runtime_total']}")
        if r["unmapped_sites"]:
            lines.append(f"    UNMAPPED: {list(r['unmapped_sites'])[:6]}")
        lines.append(f"    exit code: {r['exit_code']}")
        lines.append("")
    (outdir / "summary.txt").write_text("\n".join(lines))
    (outdir / "summary.json").write_text(json.dumps(records, indent=2))

    with open(outdir / f"lulesh_{precision}_eftsan_summary.csv", "w") as fh:
        w = csv.writer(fh)
        w.writerow(["location", "module_id", "line", "col", "function",
                    "kind", "flips", "nonfinite", "executions", "first_flip_k"])
        for site, n in records[0]["sites"].items():
            d = records[0]["site_detail"].get(site, {})
            w.writerow([site, d.get("module_id"), d.get("line"), d.get("col"),
                        d.get("function"), d.get("kind"), n,
                        d.get("nonfinite"), d.get("executions"),
                        d.get("first_flip_k")])


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("-s", "--size", type=int, default=5)
    ap.add_argument("-i", "--iter", type=int, default=20)
    ap.add_argument("--std", default="c++11")
    ap.add_argument("--link-override", action="store_true",
                    help="pass -override to llvm-link")
    ap.add_argument("--keep-bc", action="store_true",
                    help="keep the per-TU .bc files")
    ap.add_argument("--no-run", action="store_true", help="build and gate only")
    args = ap.parse_args()

    env = eftsan_env()
    if env is None:
        return 1

    lib_dirs, missing_libs = find_lib_dirs(env)
    if missing_libs:
        print(f"FATAL: cannot locate {', '.join(missing_libs)}")
        return 1
    env = with_lib_path(env, lib_dirs)

    missing = check_tools(env)
    if missing:
        print(f"FATAL: not on PATH after sourcing {SETUP_SH.name}: "
              f"{', '.join(missing)}")
        return 1

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)

    print(f"LULESH / EFTSanitizer   s={args.size} i={args.iter}   -O0")
    print(f"  precisions: {' '.join(args.precision)}")
    print(f"  EFT_HOME:   {EFT_HOME}")
    print(f"  bench:      {BENCH_ROOT}")
    print(f"  workdir:    {WORK_ROOT}")
    print(f"  libs:       {' '.join(str(d) for d in lib_dirs)}\n")

    overall = {}
    for precision in args.precision:
        print(f"===== {precision} =====")
        outdir = RESULT_ROOT / precision
        outdir.mkdir(parents=True, exist_ok=True)

        built = build(precision, outdir, args.std, args.link_override,
                      args.keep_bc, env, lib_dirs)
        if built is None:
            continue
        binary, sites, local_files = built
        if args.no_run:
            print(f"  built: {binary}\n")
            continue

        records = [run(binary, precision, args.size, args.iter, outdir,
                       sites, local_files, env)]
        write_results(precision, records, outdir)
        overall[precision] = records

        for r in records:
            own = r["flips"] - r["foreign_flips"] - r["unmapped_flips"]
            oos = ("?" if r["out_of_scope_flips"] is None
                   else r["out_of_scope_flips"])
            print(f"  -> {r['flips']} in-scope flips @ {r['locations']} loc  "
                  f"[in-TU {own}, STL {r['foreign_flips']}]  "
                  f"non-finite {r['nonfinite']}  out-of-scope {oos}")
        print()

    if args.no_run:
        return 0
    if not overall:
        print("No results produced.")
        return 1

    print("===== results =====")
    for precision, records in overall.items():
        for r in records:
            print(f"  {precision:5s} {r['flips']:9d} flips @ "
                  f"{r['locations']:3d} loc   "
                  f"{r['branch_sites']:4d} sites   exit={r['exit_code']}")
    print(f"\n{BENCH_NAME}/results/O0/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
