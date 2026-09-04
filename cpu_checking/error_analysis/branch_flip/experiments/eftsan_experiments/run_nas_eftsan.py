#!/usr/bin/env python3
"""
run_nas_eftsan.py -- build a NAS benchmark under EFTSanitizer branch-flip
instrumentation and run it, fp32 and fp64 separately.

    ./run_nas_eftsan.py cg                 # both precisions
    ./run_nas_eftsan.py bt -p fp32
    ./run_nas_eftsan.py all
    ./run_nas_eftsan.py sp --timeout 7200
    ./run_nas_eftsan.py ep --no-run

Same pipeline as run_lulesh_eftsan.py. Precision selection differs per tree:
BT/CG/LU/SP take -DNAS_FP32, EP takes -DWORKING_T=..., IS/MG bake it in; the
FP-op gate catches a define that did not take. The RNG stays fp64 in every
variant. IS exits 26 on successful verification. When *.brsites are present
in the source tree the instrumented site set is cross-checked against them
(site_agreement.txt).

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
    "BENCH_ROOT", HERE.parent.parent / "benchmarks" / "nas"))

WORK_ROOT = Path(os.environ.get("EFT_WORK_ROOT", HERE)) / "nas"

UNIFIED = {"fp32": "-DNAS_FP32", "fp64": ""}
BENCH = {
    "bt": {"define": UNIFIED, "std": "c99", "note": "NAS_Precision.h"},
    "cg": {"define": UNIFIED, "std": "c99", "note": "NAS_Precision.h"},
    "lu": {"define": UNIFIED, "std": "c99", "note": "NAS_Precision.h"},
    "sp": {"define": UNIFIED, "std": "c99", "note": "NAS_Precision.h"},
    "ep": {"define": {"fp32": "-DWORKING_T=float -DWORKING_T_IS_FLOAT=1",
                      "fp64": "-DWORKING_T=double"},
           "std": "c99", "note": "WORKING_T typedef"},
    "is": {"define": {"fp32": "", "fp64": ""},
           "std": "c99", "expect_float": False,
           "note": "integer sort; no FP branches"},
    "mg": {"define": {"fp32": "", "fp64": ""},
           "std": "c99",
           "note": "separately-edited trees; precision baked into the source"},
}
IS_SUCCESS_EXIT = {0, 26}

ERROR_LINE_RE = re.compile(r"\berror:")
UNDEF_RE = re.compile(r"undefined reference to [`']([^'\"]+)'")
TOTAL_RE = re.compile(r"Total branch flips found\s+(\d+)")
VERIFY_OK_RE = re.compile(r"Verification\s+(?:Successful|=\s*SUCCESSFUL)", re.I)
VERIFY_BAD_RE = re.compile(r"Verification\s+(?:failed|=\s*UNSUCCESSFUL)", re.I)
CLASS_RE = re.compile(r"[Cc]lass\s*=?\s*([A-Z])\b")
MAKE_SRC_RE = re.compile(r"^\s*SRCS?\s*:?\??=\s*(.+)$", re.M)


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


def sh_split(cmd, cwd=None, env=None, timeout=None):
    try:
        p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE, text=True,
                           errors="replace", timeout=timeout)
        return p.returncode, p.stdout, p.stderr, False
    except subprocess.TimeoutExpired as e:
        return None, e.stdout or "", e.stderr or "", True


def eftsan_env():
    """Source the activate script under bash and harvest its environment."""
    if not SETUP_SH.exists():
        print(f"FATAL: no setup script at {SETUP_SH}")
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
    return [t for t in ("clang", "opt", "llvm-link", "llvm-dis", "llvm-nm")
            if sh([t, "--version"], env=env)[0] != 0]


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


FP_OP_RE = re.compile(r'\b(?:fadd|fsub|fmul|fdiv|fcmp\s+\w+)\s+'
                      r'(?:fast |nnan |ninf |nsz |arcp |contract |afn |reassoc )*'
                      r'(float|double|x86_fp80)\b')


def count_fp_ops(merged_bc, env):
    """{float: n, double: n, x86_fp80: n} over the uninstrumented module."""
    ll = merged_bc.with_suffix(".fp.ll")
    rc, _ = sh(["llvm-dis", str(merged_bc), "-o", str(ll)], env=env)
    if rc != 0 or not ll.exists():
        return {}
    counts = Counter()
    try:
        with open(ll, errors="replace") as fh:
            for line in fh:
                m = FP_OP_RE.search(line)
                if m:
                    counts[m.group(1)] += 1
    except OSError:
        return {}
    finally:
        try:
            ll.unlink()
        except OSError:
            pass
    return dict(counts)


def compare_with_brsites(src_tree, sites, outdir):
    """Cross-check the instrumented site set against brtrace's *.brsites.
    Compared on file:line:col when the census is fcmp-anchored (v3), else
    on file:line."""
    files = sorted(src_tree.glob("*.brsites"))
    if not files:
        return None

    version, anchor = None, None
    brt_branches = 0
    brt_full, brt_line = set(), set()
    for f in files:
        for line in f.read_text(errors="replace").splitlines():
            if line.startswith("#"):
                if "brtrace-table-version" in line:
                    version = line.split()[-1]
                elif "loc_anchor" in line:
                    anchor = line.split()[-1]
                continue
            if not line.strip():
                continue
            parts = line.split("\t")
            if len(parts) < 2:
                continue
            brt_branches += 1
            loc = parts[1].strip()
            bits = loc.rsplit(":", 2)
            if len(bits) == 3 and bits[1].isdigit() and bits[2].isdigit():
                fn, ln, col = os.path.basename(bits[0]), bits[1], bits[2]
                brt_full.add(f"{fn}:{ln}:{col}")
                brt_line.add(f"{fn}:{ln}")
            else:
                bits = loc.rsplit(":", 1)
                fn, ln = os.path.basename(bits[0]), bits[-1]
                brt_line.add(f"{fn}:{ln}")

    exact = (anchor == "fcmp") and bool(brt_full)
    if exact:
        brt_sites = brt_full
        eft_sites = {f"{s['file']}:{s['line']}:{s['col']}"
                     for k, s in sites.items() if k[0] == "branch"}
        keyed = "file:line:col"
    else:
        brt_sites = brt_line
        eft_sites = {f"{s['file']}:{s['line']}"
                     for k, s in sites.items() if k[0] == "branch"}
        keyed = "file:line"

    only_brt = sorted(brt_sites - eft_sites)
    only_eft = sorted(eft_sites - brt_sites)
    n_eft_branch = sum(1 for k in sites if k[0] == "branch")

    lines = [
        "instrumented site agreement: EFTSan vs brtrace census", "",
        f"  brtrace table    : version {version or '?'}, "
        f"loc_anchor {anchor or '(not recorded)'}",
        f"  compared on      : {keyed}",
        f"  brtrace branches : {brt_branches}",
        f"  EFTSan sites     : {n_eft_branch}",
        f"  brtrace locations: {len(brt_sites)}",
        f"  EFTSan locations : {len(eft_sites)}", "",
    ]
    if not only_brt and not only_eft:
        lines.append("  SITE SETS IDENTICAL.")
    else:
        if only_brt:
            lines.append(f"  brtrace only ({len(only_brt)}):")
            lines += [f"      {s}" for s in only_brt[:40]]
        if only_eft:
            lines.append(f"  EFTSan only ({len(only_eft)}):")
            lines += [f"      {s}" for s in only_eft[:40]]
    (outdir / "site_agreement.txt").write_text("\n".join(lines) + "\n")
    return {"table_version": version, "loc_anchor": anchor, "keyed_on": keyed,
            "brtrace_branches": brt_branches, "brtrace_sites": len(brt_sites),
            "eftsan_branches": n_eft_branch, "eftsan_sites": len(eft_sites),
            "only_brtrace": only_brt, "only_eftsan": only_eft,
            "sets_identical": not only_brt and not only_eft}


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
    """Drop TUs referencing symbols nothing in the merge defines; never the
    TU defining main()."""
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
            (blocked if "main" in defined else dropped).append(bc)
            if "main" in defined:
                kept.append(bc)
        else:
            kept.append(bc)
    return kept, dropped, blocked


def makefile_sources(tree):
    """TU list from the Makefile's SRC=/SRCS= line."""
    mk = next((tree / n for n in ("Makefile", "makefile") if (tree / n).exists()),
              None)
    if mk is None:
        return sorted(tree.glob("*.c")), "glob (no Makefile)"
    text = mk.read_text(errors="replace")
    text = re.sub(r"\\\s*\n", " ", text)
    m = MAKE_SRC_RE.search(text)
    if not m:
        return sorted(tree.glob("*.c")), "glob (no SRC= line)"
    value = m.group(1)
    if "wildcard" in value:
        return sorted(tree.glob("*.c")), "Makefile $(wildcard *.c)"
    names = [n for n in re.findall(r"[A-Za-z0-9_./+-]+\.c\b", value)]
    srcs = [tree / n for n in names if (tree / n).exists()]
    missing = [n for n in names if not (tree / n).exists()]
    if missing:
        print(f"  WARNING: Makefile names missing files: {', '.join(missing)}")
    return (srcs or sorted(tree.glob("*.c"))), "Makefile SRC="


def write_failure_record(bench, precision, outdir, stage, reason, detail,
                         extra=None):
    """Record a build the tool refused as a result, not a missing run."""
    rec = {"benchmark": bench, "precision": precision, "opt": "O0",
           "status": "tool_failure", "failed_stage": stage, "reason": reason,
           "detail": detail, "flips": None, "locations": None, "sites": {},
           "exit_code": None}
    if extra:
        rec.update(extra)
    lines = [f"{bench} {precision} -- EFTSanitizer: NO RESULT", "",
             f"TOOL FAILURE at {stage}.", "", f"  {reason}", ""]
    lines += ["  " + l for l in detail.splitlines()]
    (outdir / "summary.txt").write_text("\n".join(lines) + "\n")
    (outdir / "summary.json").write_text(json.dumps([rec], indent=2))
    return rec


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


def build(bench, precision, outdir, env, lib_dirs, args):
    cfg = BENCH[bench]
    src = BENCH_ROOT / bench / f"{bench}_{precision}"
    dst = WORK_ROOT / bench / "build" / "O0" / f"{bench}_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        return None
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for pat in ("*.o", "*.bc", bench, f"{bench}_{precision}"):
        for junk in dst.glob(pat):
            if junk.is_file():
                junk.unlink()

    pass_so = EFT_HOME / "llvm_pass/build/EFTSan/libEFTSanitizer.so"
    runtime = EFT_HOME / "runtime/obj/libeftsanitizer.so"
    for p, what in ((pass_so, "pass plugin"), (runtime, "runtime")):
        if not p.exists():
            print(f"  FATAL: no {what} at {p}")
            return None

    srcs, how = makefile_sources(dst)
    if not srcs:
        print(f"  FATAL: no sources under {dst}")
        return None
    print(f"  TU list from {how}: {len(srcs)} file(s)")

    log = outdir / "build.log"
    if log.exists():
        log.unlink()

    cflags = [f"-std={cfg['std']}", "-O0", "-g", "-w", "-I.",
              "-fno-vectorize", "-fno-slp-vectorize"]
    define = cfg["define"][precision]
    if define:
        cflags += shlex.split(define)
        print(f"  precision flag: {define}")
    else:
        print(f"  precision flag: none ({cfg['note']})")

    print(f"  compiling {len(srcs)} TU(s) to bitcode (-O0)")
    bcs, failed = [], []
    for s in srcs:
        bc = s.with_suffix(".bc")
        rc, out = sh(["clang"] + cflags + ["-emit-llvm", "-c", str(s),
                                           "-o", str(bc)],
                     cwd=dst, env=env, log=log)
        if rc != 0 or not bc.exists():
            failed.append((s.name, out))
            continue
        bcs.append(bc)
    if failed:
        print(f"  COMPILE FAILED on {len(failed)}/{len(srcs)} TU(s) -- {log}")
        for f, out in failed[:3]:
            first = next((l for l in out.splitlines()
                          if ERROR_LINE_RE.search(l)), "")
            print(f"    {f}: {first.split('error:', 1)[-1].strip()[:120]}")
        return None

    merged = dst / f"{bench}_merged.bc"
    opt_bc = dst / f"{bench}.opt.bc"
    binary = dst / f"{bench}_{precision}.eftsan"
    sites_csv = outdir / "eftsan_sites.csv"
    pruned_total = []

    def attempt(modules):
        print(f"  llvm-link {len(modules)} module(s) -> {merged.name}")
        cmd = ["llvm-link"] + [str(b) for b in modules]
        if args.link_override:
            cmd.append("-override")
        rc, out = sh(cmd + ["-o", str(merged)], cwd=dst, env=env, log=log)
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
            msg = ("INSTRUMENTATION FAILED.  Last opt output:\n" +
                   "\n".join("      " + l[:150] for l in tail))
            if "vector" in out.lower():
                nvec, vfuncs = diagnose_vector_types(merged, env)
                msg += f"\n    {nvec} vector-typed value(s)"
                if vfuncs:
                    msg += "\n    " + ", ".join(vfuncs)
            return False, set(), msg
        print("  linking against the EFTSan runtime")
        link = ["clang", "-O0", "-g", str(opt_bc),
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
        if blocked or not dead:
            break
        print(f"  dropping {len(dead)} TU(s) with unresolvable symbols and "
              f"relinking")
        pruned_total += [b.name for b in dead]
        modules = kept
        ok, undef, msg = attempt(modules)

    if not ok:
        if "vector" in msg.lower():
            nvec, vfuncs = diagnose_vector_types(merged, env)
            write_failure_record(
                bench, precision, outdir, "opt -eftsan",
                "EFTSan cannot instrument this module: the pass exits on "
                "vector-typed loads.",
                f"{nvec} vector-typed value(s); functions: "
                f"{', '.join(vfuncs) or '(none identified)'}",
                extra={"vector_values": nvec})
            print("  NO RESULT: EFTSan refuses this module (vector loads). "
                  "Recorded as a tool failure.")
            return None
        print(f"  BUILD FAILED -- see {log}")
        for l in msg.splitlines():
            print("   ", l[:160])
        if undef:
            for sym, n in Counter(undef).most_common(6):
                print(f"      {sym}")
        write_failure_record(bench, precision, outdir, "build",
                             "build failed", msg)
        return None
    bcs = modules

    # ---- the manifest -----------------------------------------------
    if not sites_csv.exists() or sites_csv.stat().st_size == 0:
        if not cfg.get("expect_float", True):
            print("  no site manifest -- expected: this benchmark has no "
                  "FP-controlled branches")
            sites = {}
        else:
            print(f"  *** NO SITE MANIFEST at {sites_csv}")
            return None
    else:
        sites = read_sites(sites_csv)
    n_branch = sum(1 for k in sites if k[0] == "branch")
    n_select = sum(1 for k in sites if k[0] == "select")
    print(f"  site manifest: {n_branch} branch, {n_select} select")

    if sites:
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
    else:
        seen, multi = {}, []

    _, nm_out = sh(["nm", str(binary)], env=env)
    nsym = sum(1 for l in nm_out.splitlines() if "eftsan" in l.lower())
    print(f"  eftsan symbol count = {nsym}")

    # ---- precision gate ---------------------------------------------
    fp_ops = count_fp_ops(merged, env)
    print("  FP ops in module: " +
          (", ".join(f"{k}={v}" for k, v in sorted(fp_ops.items())) or "none"))
    n_float = fp_ops.get("float", 0)
    n_double = fp_ops.get("double", 0)
    prec_ok = True
    expect_float = cfg.get("expect_float", True)
    if precision == "fp32" and n_float == 0 and not expect_float:
        print("  precision gate: skipped -- no float arithmetic by design")
    elif precision == "fp32" and n_float == 0:
        prec_ok = False
        print("  *** PRECISION GATE: fp32 build has NO float arithmetic.")
    if precision == "fp64" and n_float > n_double:
        print("  *** PRECISION GATE: fp64 build is mostly float ops.")
        prec_ok = False

    # ---- census cross-check -----------------------------------------
    agree = compare_with_brsites(src, sites, outdir) if sites else None
    if agree:
        verdict = ("IDENTICAL" if agree["sets_identical"]
                   else f"{len(agree['only_brtrace'])} brtrace-only, "
                        f"{len(agree['only_eftsan'])} EFTSan-only")
        print(f"  census cross-check ({agree['keyed_on']}, table v"
              f"{agree['table_version'] or '?'}): brtrace "
              f"{agree['brtrace_branches']} branches -> site sets {verdict}")

    (outdir / "build_info.txt").write_text(
        f"benchmark    = NAS {bench.upper()}\n"
        f"precision    = {precision}\n"
        f"opt          = -O0\n"
        f"compiler     = clang (LLVM 10), C\n"
        f"precision by = {define or cfg['note']}\n"
        f"TUs compiled = {len(bcs)} (from {how})\n"
        f"TUs pruned   = {len(pruned_total)} "
        f"({', '.join(pruned_total) or 'none'})\n"
        f"cflags       = {' '.join(cflags)}\n"
        f"eftsan_syms  = {nsym}\n"
        f"fp_ops       = {fp_ops}\n"
        f"branch_sites = {n_branch}\n"
        f"select_sites = {n_select}\n"
        f"multi_fcmp   = {len(multi)}\n"
        f"modules      = {len(seen)} distinct source file(s)\n"
        f"census       = {json.dumps(agree) if agree else 'no brsites found'}\n")

    _, ldd_out = sh(["ldd", str(binary)], env=env)
    if [l for l in ldd_out.splitlines() if "not found" in l]:
        print("  *** LOADER GATE FAILED:")
        for l in ldd_out.splitlines():
            if "not found" in l:
                print("     ", l.strip()[:120])
        return None
    print("  loader gate: all shared libraries resolve")

    if nsym == 0:
        print("  *** UNINSTRUMENTED BINARY -- the pass did not take.")
        return None
    if not prec_ok:
        print("  *** aborting this precision on the gate above.")
        return None

    if not args.keep_bc:
        for b in bcs:
            b.unlink(missing_ok=True)
    local_files = {p.name for p in dst.iterdir() if p.suffix in (".c", ".h")}
    return binary, sites, local_files, fp_ops, agree


def run(bench, binary, precision, outdir, sites, local_files, env, timeout):
    events_csv = outdir / "eftsan_events.csv"
    totals_csv = outdir / "eftsan_totals.csv"
    run_env = dict(env)
    run_env["EFTSAN_BF_OUT"] = str(events_csv)
    run_env["EFTSAN_BF_TOTALS"] = str(totals_csv)

    print(f"  running {binary.name}")
    rc, out, err, timed_out = sh_split(
        ["stdbuf", "-i0", "-o0", "-e0", str(binary)],
        cwd=binary.parent, env=run_env, timeout=timeout)
    (outdir / "run_O0.stdout").write_text(out)
    (outdir / "run_O0.stderr").write_text(err)
    if timed_out:
        print(f"  *** TIMEOUT after {timeout}s")

    src_errlog = binary.parent / "error.log"
    errlog_text = ""
    if src_errlog.exists():
        errlog_text = src_errlog.read_text(errors="replace")
        (outdir / "error.log").write_text(errlog_text)

    if not events_csv.exists() and sites:
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
    for key, cnt in flips.most_common():
        s = sites.get(key)
        if s is None:
            unmapped[f"{key[0]}:mod{key[1]}:site{key[2]}"] = cnt
            continue
        loc = f"{s['file']}:{s['line']}"
        named[loc] += cnt
        if s["file"] not in local_files:
            foreign[loc] += cnt
        site_detail[loc] = {
            "kind": key[0], "module_id": key[1], "site_id": key[2],
            "file": s["file"], "line": s["line"], "col": s["col"],
            "function": s["function"], "n_fcmp": s["n_fcmp"],
            "flips": cnt,
            "nonfinite": nonfinite.get(key, 0),
            "executions": totals.get(key, (None, None, None))[0],
            "first_flip_k": min(ks[key]) if ks.get(key) else None,
        }

    if unmapped:
        print(f"  *** {len(unmapped)} event key(s) absent from the manifest")

    n_nonfinite = sum(nonfinite.values())
    if n_nonfinite:
        print(f"  {n_nonfinite} execution(s) with a non-finite shadow")

    verified = None
    if VERIFY_OK_RE.search(out):
        verified = True
    elif VERIFY_BAD_RE.search(out):
        verified = False
    cm = CLASS_RE.search(out)
    nas_class = cm.group(1) if cm else None

    ok_exit = rc in IS_SUCCESS_EXIT if bench == "is" else rc == 0
    if verified is True:
        print(f"  verification: SUCCESSFUL (class {nas_class})")
    elif verified is False:
        print(f"  verification: FAILED (class {nas_class})")
    else:
        print("  *** no verification line in the output")
    if not ok_exit and not timed_out:
        print(f"  *** exit {rc}")
        for l in (err or out).splitlines()[:3]:
            if l.strip():
                print("     ", l[:150])

    return {
        "benchmark": bench, "precision": precision, "opt": "O0",
        "threshold": 45,
        "branch_sites": sum(1 for k in sites if k[0] == "branch"),
        "select_sites": sum(1 for k in sites if k[0] == "select"),
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
        "verified": verified, "class": nas_class,
        "timed_out": timed_out, "exit_code": rc, "exit_ok": ok_exit,
        "events_csv": str(events_csv.relative_to(HERE)),
        "totals_csv": str(totals_csv.relative_to(HERE)),
    }


def verdict_of(v):
    return "SUCCESSFUL" if v else ("FAILED" if v is False else "ABSENT")


def write_results(bench, precision, records, outdir, fp_ops, agree):
    r0 = records[0]
    lines = [f"NAS {bench.upper()} {precision} -- EFTSanitizer branch flips",
             f"(-O0, merged module, class {r0['class'] or '?'})", ""]
    for r in records:
        own = r["flips"] - r["foreign_flips"] - r["unmapped_flips"]
        oos = ("n/a" if r["out_of_scope_flips"] is None
               else str(r["out_of_scope_flips"]))
        lines.append(f"{r['flips']} in-scope flips @ {r['locations']} loc"
                     f"   [in-TU {own}, foreign {r['foreign_flips']}]")
        lines.append(f"{r['nonfinite']} with non-finite shadow"
                     f"   |   {oos} out-of-scope")
    lines += ["",
              f"{r0['branch_sites']} branch sites, "
              f"{r0['select_sites']} select sites.  FP ops: {fp_ops}.",
              f"verification: {verdict_of(r0['verified'])}"
              f"   exit code: {r0['exit_code']}", ""]
    if agree:
        lines.append(f"census cross-check ({agree['keyed_on']}, table v"
                     f"{agree['table_version'] or '?'}): brtrace "
                     f"{agree['brtrace_branches']} branches / "
                     f"{agree['brtrace_sites']} locations vs EFTSan "
                     f"{agree['eftsan_branches']} sites / "
                     f"{agree['eftsan_sites']} locations -- sets "
                     f"{'IDENTICAL' if agree['sets_identical'] else 'DIFFER'}")
        lines.append("")
    for r in records:
        lines.append(f"--- O0 : {r['flips']} flips @ {r['locations']} loc ---")
        for site, cnt in r["sites"].items():
            d = r["site_detail"].get(site, {})
            extra = []
            if d.get("executions") is not None:
                extra.append(f"of {d['executions']} exec")
            if d.get("first_flip_k") is not None:
                extra.append(f"first k={d['first_flip_k']}")
            if d.get("nonfinite"):
                extra.append(f"{d['nonfinite']} non-finite")
            tail = ("   (" + ", ".join(extra) + ")") if extra else ""
            lines.append(f"    {cnt:9d}  {site}  "
                         f"[{d.get('function','?')}]{tail}")
        if r["runtime_total"] is not None:
            lines.append(f"    runtime total: {r['runtime_total']}")
        if r["unmapped_sites"]:
            lines.append(f"    UNMAPPED: {list(r['unmapped_sites'])[:6]}")
        if r["timed_out"]:
            lines.append("    *** RUN TIMED OUT -- counts are partial")
        lines.append("")
    (outdir / "summary.txt").write_text("\n".join(lines))
    (outdir / "summary.json").write_text(json.dumps(records, indent=2))

    with open(outdir / f"{bench}_{precision}_eftsan_summary.csv", "w") as fh:
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
    ap.add_argument("benchmark", help="bt cg ep is lu mg sp, or 'all'")
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("--timeout", type=int, default=3600,
                    help="per-run seconds (default 3600)")
    ap.add_argument("--link-override", action="store_true")
    ap.add_argument("--keep-bc", action="store_true")
    ap.add_argument("--no-run", action="store_true")
    args = ap.parse_args()

    benches = sorted(BENCH) if args.benchmark == "all" else [args.benchmark]
    bad = [b for b in benches if b not in BENCH]
    if bad:
        print(f"unknown benchmark(s): {', '.join(bad)}; "
              f"choose from {', '.join(sorted(BENCH))} or 'all'")
        return 1

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

    print(f"NAS / EFTSanitizer   -O0   timeout {args.timeout}s")
    print(f"  benchmarks: {' '.join(benches)}")
    print(f"  precisions: {' '.join(args.precision)}")
    print(f"  bench root: {BENCH_ROOT}")
    print(f"  libs:       {' '.join(str(d) for d in lib_dirs)}\n")

    grand = {}
    for bench in benches:
        print(f"########## {bench.upper()} ({BENCH[bench]['note']}) ##########")
        for precision in args.precision:
            print(f"===== {bench} {precision} =====")
            outdir = WORK_ROOT / bench / "results" / "O0" / precision
            outdir.mkdir(parents=True, exist_ok=True)
            built = build(bench, precision, outdir, env, lib_dirs, args)
            if built is None:
                print()
                continue
            binary, sites, local_files, fp_ops, agree = built
            if args.no_run:
                print(f"  built: {binary}\n")
                continue
            rec = run(bench, binary, precision, outdir, sites, local_files,
                      env, args.timeout)
            write_results(bench, precision, [rec], outdir, fp_ops, agree)
            grand[(bench, precision)] = rec
            oos = ("?" if rec["out_of_scope_flips"] is None
                   else rec["out_of_scope_flips"])
            print(f"  -> {rec['flips']} in-scope flips @ "
                  f"{rec['locations']} loc  non-finite {rec['nonfinite']}  "
                  f"out-of-scope {oos}\n")

    if args.no_run:
        return 0
    if not grand:
        print("No results produced.")
        return 1

    print("===== results =====")
    for (bench, precision), r in sorted(grand.items()):
        v = ("verified" if r["verified"] else
             ("VERIFY FAILED" if r["verified"] is False else "no verify line"))
        t = "  TIMED OUT" if r["timed_out"] else ""
        print(f"  {bench:3s} {precision:5s} {r['flips']:9d} flips @ "
              f"{r['locations']:3d} loc   {r['branch_sites']:4d} sites   "
              f"{v}   exit={r['exit_code']}{t}")
    print("\nnas/<bench>/results/O0/<precision>/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
