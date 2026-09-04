#!/usr/bin/env python3
"""
run_quicksilver_eftsan.py -- build QuickSilver under EFTSanitizer branch-flip
instrumentation and run it, fp32 and fp64 separately.

    ./run_quicksilver_eftsan.py                  # both precisions
    ./run_quicksilver_eftsan.py -p fp32 -n 4000 -x 5 -N 3
    ./run_quicksilver_eftsan.py --no-run         # build and gate only

Same pipeline as run_lulesh_eftsan.py. The TU list is read from the Makefile.
Census config: -n 4000 mesh 5^3 N=3 (first divergence at cycle 2). Extra
gates: sizeof(qs_real) probe, and per-cycle tallies must match the
uninstrumented reference. The fp32 build may fail in the pass on vector-typed
values (SysV struct coercion); that is recorded as a tool failure.

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
    "BENCH_ROOT", HERE.parent.parent / "benchmarks" / "quicksilver"))

BENCH_NAME = "quicksilver"
WORK_ROOT = Path(os.environ.get("EFT_WORK_ROOT", HERE)) / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build" / "O0"
RESULT_ROOT = WORK_ROOT / "results" / "O0"

PRECISION_FLAG = {"fp32": "-DQS_FP32", "fp64": ""}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8}

SKIP_DIRS = {"docs", "CMakeFiles", "build", ".git", "obj", "doc", "Examples"}
SRC_DIRS = ["src"]

REFERENCE_TALLY = {
    "fp32": {2: (4422, 4632, 456, 16740)},
    "fp64": {2: (4421, 4632, 455, 16738)},
}
CENSUS_N, CENSUS_MESH = 4000, 5

BRTRACE_INSTRUMENTED = None

ERROR_LINE_RE = re.compile(r"\berror:")
UNDEF_RE = re.compile(r"undefined reference to [`']([^'\"]+)'")
TOTAL_RE = re.compile(r"Total branch flips found\s+(\d+)")
TALLY_RE = re.compile(r"^\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+"
                      r"(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+\d+\s+(\d+)")
SRC_RE = re.compile(r"\b([A-Za-z0-9_+-][A-Za-z0-9_.+-]*)\.(?:cc|cpp|o)\b")


def fnv1a_32(name):
    h = 2166136261
    for c in name.encode():
        h = ((h ^ c) * 16777619) & 0xFFFFFFFF
    return h


def sh(cmd, cwd=None, env=None, log=None):
    p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, text=True, errors="replace")
    if log:
        with open(log, "a") as fh:
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
    for t in ("clang++", "opt", "llvm-link", "llvm-dis", "llvm-nm"):
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
                int(rec["executions"]), int(rec["flips"]),
                int(rec["nonfinite"]))
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
    by_id = defaultdict(set)
    for fname, mods in seen.items():
        for m in mods:
            by_id[m].add(fname)
    for mod, names in by_id.items():
        if len(names) > 1:
            bad.append((sorted(names), mod, "BASENAME COLLISION"))
    return seen, bad


def tu_symbols(bc, env):
    rc, out = sh(["llvm-nm", str(bc)], env=env)
    if rc != 0:
        return set(), set()
    defined, undefined = set(), set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) < 2:
            continue
        kind, name = parts[-2], parts[-1]
        if kind == "U":
            undefined.add(name)
        elif kind in "TDBWVRSGtdbwvrsg":
            defined.add(name)
    return defined, undefined


def prune_dangling(bcs, missing, env):
    """Returns (kept, dropped, blocked). A TU defining main() is never dropped."""
    tables = {bc: tu_symbols(bc, env) for bc in bcs}
    defined_anywhere = set()
    for d, _ in tables.values():
        defined_anywhere |= d
    unresolvable = {m for m in missing if m not in defined_anywhere}
    if not unresolvable:
        return list(bcs), [], []

    kept, dropped, blocked = [], [], []
    for bc in bcs:
        defined, undefined = tables[bc]
        if undefined & unresolvable:
            if "main" in defined:
                blocked.append(bc)
                kept.append(bc)
            else:
                dropped.append(bc)
        else:
            kept.append(bc)
    return kept, dropped, blocked


def makefile_objects(mkfile):
    """Source stems a Makefile names (accepts .cc/.cpp and .o tokens)."""
    try:
        text = mkfile.read_text(errors="replace")
    except OSError:
        return set()
    text = re.sub(r"\\\s*\n", " ", text)
    stems = set()
    for line in text.splitlines():
        line = line.split("#", 1)[0]
        if ".o" not in line and ".cc" not in line and ".cpp" not in line:
            continue
        if line.startswith("\t") or "%" in line:
            continue
        for stem in SRC_RE.findall(line):
            stems.add(stem)
    return stems


def collect_sources(tree, extra_exclude=()):
    """The TU list QuickSilver's own Makefile builds. Returns (sources,
    dropped, fallback_dirs)."""
    excluded = set(extra_exclude)
    srcs, dropped, fallback = [], [], []

    dirs = [tree / d for d in SRC_DIRS if (tree / d).is_dir()]
    if not dirs:
        dirs = [tree]

    for d in dirs:
        present = sorted(list(d.glob("*.cc")) + list(d.glob("*.cpp")))
        if not present:
            continue
        mk = next((m for m in (d / "Makefile", tree / "Makefile",
                               d / "makefile", tree / "makefile")
                   if m.exists()), None)
        stems = makefile_objects(mk) if mk else set()
        wanted = [c for c in present if c.stem in stems] if stems else []
        if not wanted:
            fallback.append(str(d.relative_to(tree)) or ".")
            wanted = present
        else:
            dropped += [str(c.relative_to(tree)) for c in present
                        if c.stem not in stems]
        for c in wanted:
            if c.name in excluded:
                dropped.append(str(c.relative_to(tree)))
            else:
                srcs.append(c)
    return srcs, sorted(dropped), fallback


def include_flags(tree):
    incs = [f"-I{tree}"]
    for d in sorted(p for p in tree.rglob("*")
                    if p.is_dir() and p.name not in SKIP_DIRS
                    and not any(x in SKIP_DIRS for x in p.parts)):
        incs.append(f"-I{d}")
    return incs


def check_width(tree, precision, env, outdir):
    """sizeof(qs_real) probe with a plain compiler."""
    probe = outdir / "_width_probe.cc"
    probe.write_text('#include "QS_Precision.hh"\n#include <cstdio>\n'
                     'int main(){ printf("%zu\\n", sizeof(qs_real)); '
                     'return 0; }\n')
    out_bin = outdir / "_width_probe"
    cmd = ["clang++", "-O0", "-w", "-std=c++11", "-include", "cstdint"]
    if PRECISION_FLAG[precision]:
        cmd.append(PRECISION_FLAG[precision])
    cmd += [f"-I{tree}", f"-I{tree/'src'}", str(probe), "-o", str(out_bin)]
    rc, _ = sh(cmd, env=env)
    if rc != 0:
        return None, False
    rc, out = sh([str(out_bin)], env=env)
    try:
        w = int(out.strip())
    except ValueError:
        return None, False
    return w, w == EXPECTED_WIDTH[precision]


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


def build(precision, outdir, std, link_override, keep_bc, env, lib_dirs,
          extra_exclude):
    src = BENCH_ROOT / f"qs_{precision}"
    dst = BUILD_ROOT / f"qs_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        return None
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for pat in ("*.o", "*/*.o", "*.bc", "*/*.bc", "qs", "src/qs"):
        for junk in dst.glob(pat):
            if junk.is_file():
                junk.unlink()

    pass_so = EFT_HOME / "llvm_pass/build/EFTSan/libEFTSanitizer.so"
    runtime = EFT_HOME / "runtime/obj/libeftsanitizer.so"
    for p, what in ((pass_so, "pass plugin"), (runtime, "runtime")):
        if not p.exists():
            print(f"  FATAL: no {what} at {p}")
            return None

    srcs, dropped, fallback = collect_sources(dst, extra_exclude)
    if dropped:
        print(f"  {len(dropped)} file(s) present but not built by the "
              f"Makefile (skipped): {', '.join(map(str, dropped))}")
    if fallback:
        print(f"  *** no parseable Makefile object list in "
              f"{', '.join(fallback)} -- compiling every source there")
    if not srcs:
        print(f"  FATAL: no sources found under {dst}")
        return None

    log = outdir / "build.log"
    if log.exists():
        log.unlink()

    cflags = [f"-std={std}", "-O0", "-g", "-w", "-include", "cstdint",
              "-fno-vectorize", "-fno-slp-vectorize"]
    if PRECISION_FLAG[precision]:
        cflags.append(PRECISION_FLAG[precision])
    cflags += include_flags(dst)

    # ---- 1. per-TU bitcode ------------------------------------------
    print(f"  compiling {len(srcs)} TUs to bitcode (-O0, {std})")
    bcs, failed = [], []
    for s in srcs:
        bc = s.with_suffix(".bc")
        rc, out = sh(["clang++"] + cflags +
                     ["-emit-llvm", "-c", str(s), "-o", str(bc)],
                     cwd=dst, env=env, log=log)
        if rc != 0 or not bc.exists():
            failed.append((s.relative_to(dst), out))
            continue
        bcs.append(bc)
    if failed:
        print(f"  COMPILE FAILED on {len(failed)}/{len(srcs)} TUs -- see {log}")
        for f, out in failed[:3]:
            first = next((l for l in out.splitlines()
                          if ERROR_LINE_RE.search(l)), "")
            first = first.split("error:", 1)[-1].strip() or first
            print(f"    {f}: {first[:120]}")
        return None
    print(f"  compiled {len(bcs)} TUs")

    # ---- 2-4. merge, instrument, link -------------------------------
    merged = dst / "qs_merged.bc"
    opt_bc = dst / "qs.opt.bc"
    binary = dst / f"qs_{precision}.eftsan"
    sites_csv = outdir / "eftsan_sites.csv"
    pruned_total = []

    def attempt(modules):
        print(f"  llvm-link {len(modules)} modules -> {merged.name}")
        link_cmd = ["llvm-link"] + [str(b) for b in modules]
        if link_override:
            link_cmd.append("-override")
        rc, out = sh(link_cmd + ["-o", str(merged)], cwd=dst, env=env, log=log)
        if rc != 0 or not merged.exists():
            return False, set(), ("llvm-link FAILED.\n" +
                                  "\n".join(out.splitlines()[:4]))

        opt_env = dict(env)
        opt_env["EFTSAN_SITES"] = str(sites_csv)

        print("  opt -eftsan on the merged module")
        rc, out = sh(["opt", "-load", str(pass_so), "-eftsan",
                      str(merged), "-o", str(opt_bc)],
                     cwd=dst, env=opt_env, log=log)
        if rc != 0 or not opt_bc.exists() or opt_bc.stat().st_size == 0:
            tail = [l for l in out.splitlines() if l.strip()][-6:]
            msg = "INSTRUMENTATION FAILED.  Last opt output:\n" + \
                  "\n".join("      " + l[:150] for l in tail)
            if "vector" in out.lower():
                nvec, vfuncs = diagnose_vector_types(merged, env)
                msg += f"\n    {nvec} vector-typed value(s) in the module."
                for f in vfuncs:
                    msg += f"\n      {f}"
            return False, set(), msg

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
            return False, set(UNDEF_RE.findall(out)), "LINK FAILED."
        return True, set(), ""

    modules = list(bcs)
    ok, undef, msg = attempt(modules)
    for _ in range(2):
        if ok or not undef:
            break
        kept, dead, blocked = prune_dangling(modules, undef, env)
        if blocked:
            print("  *** the unresolvable symbol is referenced by the TU "
                  "defining main()")
            break
        if not dead:
            break
        print(f"  dropping {len(dead)} TU(s) with unresolvable symbols and "
              f"relinking:")
        for b in dead:
            print(f"      {b.relative_to(dst)}")
        pruned_total += [str(b.relative_to(dst)) for b in dead]
        modules = kept
        ok, undef, msg = attempt(modules)

    if not ok:
        if "vector" in msg.lower():
            nvec, vfuncs = diagnose_vector_types(merged, env)
            write_failure_record(
                precision, outdir, "opt -eftsan",
                "EFTSan cannot instrument this module: the pass exits on "
                "vector-typed loads.",
                f"{nvec} vector-typed value(s) in the merged module.\n"
                f"Functions holding vector values: "
                f"{', '.join(vfuncs) if vfuncs else '(none identified)'}",
                extra={"vector_values": nvec, "vector_functions": vfuncs})
            print(f"  NO RESULT for {precision}: EFTSan refuses this module "
                  f"(vector-typed loads). Recorded as a tool failure.")
            return None
        print(f"  BUILD FAILED -- see {log}")
        for line in msg.splitlines():
            print("   ", line[:160])
        if undef:
            syms = Counter(undef)
            print(f"    {len(syms)} distinct undefined symbol(s):")
            for sym, _ in syms.most_common(8):
                print(f"      {sym}")
        return None
    bcs = modules

    # ---- gate 2: the manifest exists --------------------------------
    if not sites_csv.exists() or sites_csv.stat().st_size == 0:
        print(f"  *** NO SITE MANIFEST at {sites_csv}")
        return None
    sites = read_sites(sites_csv)
    n_branch = sum(1 for k in sites if k[0] == "branch")
    n_select = sum(1 for k in sites if k[0] == "select")
    ref = (f"  (brtrace instrumented = {BRTRACE_INSTRUMENTED})"
           if BRTRACE_INSTRUMENTED else "")
    print(f"  site manifest: {n_branch} branch, {n_select} select{ref}")

    # ---- gate 3: hash convention and basename collisions ------------
    seen, bad = verify_module_ids(sites)
    if bad:
        print("  *** MODULE ID CHECK FAILED:")
        for entry in bad[:8]:
            print(f"      {entry}")
        return None
    print(f"  module ids verified: {len(seen)} distinct source file(s)")

    multi = [k for k, s in sites.items() if s["n_fcmp"] > 1]
    if multi:
        print(f"  note: {len(multi)} site(s) have n_fcmp > 1")

    # ---- gates 1 and 4 ----------------------------------------------
    _, nm_out = sh(["nm", str(binary)], env=env)
    nsym = sum(1 for l in nm_out.splitlines() if "eftsan" in l.lower())
    print(f"  eftsan symbol count = {nsym}")

    width, ok_w = check_width(dst, precision, env, outdir)
    print(f"  sizeof(qs_real) = {width} "
          f"({'ok' if ok_w else 'EXPECTED ' + str(EXPECTED_WIDTH[precision])})")

    (outdir / "build_info.txt").write_text(
        f"benchmark    = QuickSilver\n"
        f"precision    = {precision}\n"
        f"opt          = -O0\n"
        f"compiler     = clang++ (LLVM 10), -std={std}\n"
        f"TUs compiled = {len(bcs)}\n"
        f"TUs skipped  = {len(dropped)} "
        f"({', '.join(map(str, dropped)) or 'none'})\n"
        f"TUs pruned   = {len(pruned_total)} "
        f"({', '.join(map(str, pruned_total)) or 'none'})\n"
        f"TU list from = Makefile object list"
        f"{' (fallback glob: ' + ', '.join(fallback) + ')' if fallback else ''}\n"
        f"pass         = {pass_so} -eftsan\n"
        f"runtime      = {runtime}\n"
        f"cflags       = "
        f"{' '.join(c for c in cflags if not c.startswith('-I'))}\n"
        f"includes     = {sum(1 for c in cflags if c.startswith('-I'))} dirs\n"
        f"link libs    = -leftsanitizer -lm -lmpfr -lgmp -lstdc++\n"
        f"eftsan_syms  = {nsym}\n"
        f"qs_real      = {width} bytes "
        f"(expected {EXPECTED_WIDTH[precision]})\n"
        f"branch_sites = {n_branch}\n"
        f"select_sites = {n_select}\n"
        f"multi_fcmp   = {len(multi)}\n"
        f"modules      = {len(seen)} distinct source file(s)\n"
        f"mpi          = serial (QuickSilver's own stubs)\n"
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
    if not ok_w:
        print("  *** WRONG qs_real WIDTH -- aborting this precision.")
        return None

    if not keep_bc:
        for b in bcs:
            b.unlink(missing_ok=True)

    local_files = {p.name for p in dst.rglob("*")
                   if p.suffix in (".cc", ".cpp", ".hh", ".h", ".hpp")}
    return binary, sites, local_files


def run(binary, precision, opts, outdir, sites, local_files, env):
    events_csv = outdir / "eftsan_events.csv"
    totals_csv = outdir / "eftsan_totals.csv"
    run_env = dict(env)
    run_env["EFTSAN_BF_OUT"] = str(events_csv)
    run_env["EFTSAN_BF_TOTALS"] = str(totals_csv)

    args = ["-n", str(opts["n"]),
            "-X", "10", "-Y", "10", "-Z", "10",
            "-x", str(opts["mesh"]), "-y", str(opts["mesh"]),
            "-z", str(opts["mesh"]),
            "-I", "1", "-J", "1", "-K", "1",
            "-N", str(opts["cycles"])]
    print(f"  running -n {opts['n']} mesh {opts['mesh']}^3 "
          f"N={opts['cycles']}")
    rc, out, err = sh_split(["stdbuf", "-i0", "-o0", "-e0", str(binary)] + args,
                            cwd=binary.parent, env=run_env)
    (outdir / "run_O0.stdout").write_text(out)
    (outdir / "run_O0.stderr").write_text(err)

    src_errlog = binary.parent / "error.log"
    errlog_text = ""
    if src_errlog.exists():
        errlog_text = src_errlog.read_text(errors="replace")
        (outdir / "error.log").write_text(errlog_text)

    # ---- gate 6 -----------------------------------------------------
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

    tm = TOTAL_RE.search(errlog_text)
    runtime_total = int(tm.group(1)) if tm else None
    out_of_scope = (runtime_total - parsed) if runtime_total is not None else None

    totals_flips = sum(v[1] for v in totals.values())
    if totals_flips != parsed:
        print(f"  *** ACCOUNTING MISMATCH: events.csv has {parsed} detection "
              f"row(s) but totals.csv sums to {totals_flips}")

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
            "kind": key[0], "module_id": key[1], "site_id": key[2],
            "file": s["file"], "line": s["line"], "col": s["col"],
            "function": s["function"], "n_fcmp": s["n_fcmp"],
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

    tallies = []
    for l in out.splitlines():
        m = TALLY_RE.match(l)
        if m:
            tallies.append({"cycle": int(m.group(1)),
                            "absorb": int(m.group(3)),
                            "scatter": int(m.group(4)),
                            "fission": int(m.group(5)),
                            "num_seg": int(m.group(6))})

    # ---- gate 7: instrumentation must not perturb the physics -------
    tally_ok, tally_note = None, ""
    if opts["n"] == CENSUS_N and opts["mesh"] == CENSUS_MESH:
        ref = REFERENCE_TALLY.get(precision, {})
        for t in tallies:
            if t["cycle"] in ref:
                got = (t["absorb"], t["scatter"], t["fission"], t["num_seg"])
                want = ref[t["cycle"]]
                tally_ok = (got == want)
                if not tally_ok:
                    tally_note = (f"cycle {t['cycle']}: got {got}, "
                                  f"uninstrumented reference {want}")
                    print(f"  *** TALLY MISMATCH: {tally_note}")
                else:
                    print(f"  tally gate: cycle {t['cycle']} matches the "
                          f"uninstrumented reference")
        if tally_ok is None:
            print("  (no reference cycle in the tally output -- gate skipped)")

    if rc != 0:
        print(f"  *** non-zero exit ({rc})")
        for line in (err or out).splitlines()[:3]:
            if line.strip():
                print("     ", line[:150])

    return {
        "benchmark": "quicksilver",
        "precision": precision,
        "opt": "O0",
        "threshold": 45,
        "branch_sites": sum(1 for k in sites if k[0] == "branch"),
        "select_sites": sum(1 for k in sites if k[0] == "select"),
        "n": opts["n"], "mesh": opts["mesh"], "cycles": opts["cycles"],
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
        "tallies": tallies,
        "tally_matches_reference": tally_ok,
        "tally_note": tally_note,
        "exit_code": rc,
        "events_csv": str(events_csv.relative_to(HERE)),
        "totals_csv": str(totals_csv.relative_to(HERE)),
        "sites_csv": str((outdir / "eftsan_sites.csv").relative_to(HERE)),
        "log": str((outdir / "run_O0.stderr").relative_to(HERE)),
        "stdout_log": str((outdir / "run_O0.stdout").relative_to(HERE)),
    }


def write_results(precision, records, outdir):
    r0 = records[0]
    lines = [f"QuickSilver {precision} -- EFTSanitizer branch flips",
             f"(-{r0['opt']}, merged module, n={r0['n']} "
             f"mesh={r0['mesh']}^3 N={r0['cycles']})", ""]
    for r in records:
        own = r["flips"] - r["foreign_flips"] - r["unmapped_flips"]
        oos = ("n/a" if r["out_of_scope_flips"] is None
               else str(r["out_of_scope_flips"]))
        lines.append(f"{r['flips']} in-scope flips @ {r['locations']} loc"
                     f"   [in-TU {own}, foreign {r['foreign_flips']}]")
        lines.append(f"{r['nonfinite']} with non-finite shadow"
                     f"   |   {oos} out-of-scope")
    lines += ["", f"{r0['branch_sites']} branch sites, "
                  f"{r0['select_sites']} select sites", ""]
    if r0["tallies"]:
        lines.append("control-flow tallies:")
        lines.append("  cycle   absorb  scatter  fission  num_seg")
        for t in r0["tallies"]:
            lines.append(f"  {t['cycle']:5d} {t['absorb']:8d} {t['scatter']:8d}"
                         f" {t['fission']:8d} {t['num_seg']:8d}")
        if r0["tally_matches_reference"] is True:
            lines.append("  -> matches the uninstrumented reference")
        elif r0["tally_matches_reference"] is False:
            lines.append(f"  -> MISMATCH: {r0['tally_note']}")
        lines.append("")
    for r in records:
        lines.append(f"--- {r['opt']} : {r['flips']} flips @ "
                     f"{r['locations']} loc ---")
        for site, cnt in r["sites"].items():
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
            lines.append(f"    {cnt:9d}  {site}  [{d.get('function','?')}]{tail}")
        if r["runtime_total"] is not None:
            lines.append(f"    runtime total: {r['runtime_total']}")
        if r["unmapped_sites"]:
            lines.append(f"    UNMAPPED: {list(r['unmapped_sites'])[:6]}")
        lines.append(f"    exit code: {r['exit_code']}")
        lines.append("")
    (outdir / "summary.txt").write_text("\n".join(lines))
    (outdir / "summary.json").write_text(json.dumps(records, indent=2))

    with open(outdir / f"quicksilver_{precision}_eftsan_summary.csv", "w") as fh:
        w = csv.writer(fh)
        w.writerow(["location", "module_id", "line", "col", "function",
                    "kind", "flips", "nonfinite", "executions",
                    "first_flip_k"])
        for site, cnt in records[0]["sites"].items():
            d = records[0]["site_detail"].get(site, {})
            w.writerow([site, d.get("module_id"), d.get("line"), d.get("col"),
                        d.get("function"), d.get("kind"), cnt,
                        d.get("nonfinite"), d.get("executions"),
                        d.get("first_flip_k")])


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("-n", type=int, default=4000, help="particles")
    ap.add_argument("-x", "--mesh", type=int, default=5,
                    help="mesh cells per axis")
    ap.add_argument("-N", "--cycles", type=int, default=3)
    ap.add_argument("--std", default="c++11")
    ap.add_argument("--link-override", action="store_true",
                    help="pass -override to llvm-link")
    ap.add_argument("--exclude", nargs="*", default=[], metavar="FILE.cc",
                    help="additional source files to skip")
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

    opts = {"n": args.n, "mesh": args.mesh, "cycles": args.cycles}
    print(f"QuickSilver / EFTSanitizer   n={args.n} mesh={args.mesh}^3 "
          f"N={args.cycles}   -O0")
    print(f"  precisions: {' '.join(args.precision)}")
    print(f"  EFT_HOME:   {EFT_HOME}")
    print(f"  bench:      {BENCH_ROOT}")
    print(f"  workdir:    {WORK_ROOT}")
    print(f"  libs:       {' '.join(str(d) for d in lib_dirs)}")
    if args.n != CENSUS_N or args.mesh != CENSUS_MESH:
        print(f"  off the census config (n={CENSUS_N}, mesh={CENSUS_MESH}^3)")
    print()

    overall = {}
    for precision in args.precision:
        print(f"===== {precision} =====")
        outdir = RESULT_ROOT / precision
        outdir.mkdir(parents=True, exist_ok=True)

        built = build(precision, outdir, args.std, args.link_override,
                      args.keep_bc, env, lib_dirs, args.exclude)
        if built is None:
            continue
        binary, sites, local_files = built
        if args.no_run:
            print(f"  built: {binary}\n")
            continue

        records = [run(binary, precision, opts, outdir, sites, local_files,
                       env)]
        write_results(precision, records, outdir)
        overall[precision] = records

        for r in records:
            own = r["flips"] - r["foreign_flips"] - r["unmapped_flips"]
            oos = ("?" if r["out_of_scope_flips"] is None
                   else r["out_of_scope_flips"])
            print(f"  -> {r['flips']} in-scope flips @ {r['locations']} loc  "
                  f"[in-TU {own}, foreign {r['foreign_flips']}]  "
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
            tg = {True: "tally ok", False: "TALLY MISMATCH",
                  None: "tally n/a"}[r["tally_matches_reference"]]
            print(f"  {precision:5s} {r['flips']:9d} flips @ "
                  f"{r['locations']:3d} loc   "
                  f"{r['branch_sites']:4d} sites   {tg}   "
                  f"exit={r['exit_code']}")

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

    print(f"\n{BENCH_NAME}/results/O0/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
