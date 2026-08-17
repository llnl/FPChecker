#!/usr/bin/env python3
"""
run_nas_eftsan.py

Build a NAS Parallel Benchmark under EFTSanitizer branch-flip instrumentation
and run it, keeping fp32 and fp64 results completely separate.  Same structure
as run_lulesh_eftsan.py / run_amg_eftsan.py / run_quicksilver_eftsan.py.

Place in: branch_flip/experiments/eftsan_experiments/

    ./run_nas_eftsan.py cg                 # both precisions
    ./run_nas_eftsan.py bt -p fp32         # fp32 only
    ./run_nas_eftsan.py all                # every benchmark, both precisions
    ./run_nas_eftsan.py sp --timeout 7200  # SP is slow under EFTSan
    ./run_nas_eftsan.py ep --no-run        # build and gate only

Layout produced (one subtree per benchmark):
    nas/
      cg/results/O0/  fp32/  build.log  run_O0.stdout  run_O0.stderr
                             summary.txt  summary.json  build_info.txt
                             instrumented_sites.csv  site_agreement.txt
                             cg_fp32_eftsan_summary.csv
                      fp64/  ...
      cg/build/O0/    cg_fp32/  cg_fp64/

THREE PRECISION MECHANISMS, NOT ONE

  The seven benchmarks do not share a precision idiom, so the flag table is
  per-benchmark and must not be "simplified":

    BT CG LU SP   unified tree + NAS_Precision.h.  fp32/fp64 sources are
                  byte-identical; precision comes from -DNAS_FP32 (or nothing
                  for double).  Matches the -DLULESH_FP32 idiom.
    EP            -DWORKING_T=float -DWORKING_T_IS_FLOAT=1  (from its
                  Makefile).  ep.c defaults WORKING_T to double if unset.
    IS            NO precision flag.  IS is an integer sort; its fp32 and
                  fp64 trees are byte-identical apart from the binary name,
                  so both builds are the same program.  Expect identical
                  results, and treat any difference as a bug in the harness.
    MG            separately-edited trees (392 lines differ between fp32 and
                  fp64).  Precision is baked into the source, NOT selected by
                  a flag.  Passing one would do nothing.

  Passing NAS_FP32 to EP, IS or MG is silently ineffective -- the build
  succeeds and produces the wrong precision -- which is why the IR precision
  gate below exists.

THE RNG STACK STAYS FP64 IN EVERY VARIANT

  randlc / vranlc / c_randdp.c emulate exact 46-bit modular arithmetic and
  need the full 53-bit double mantissa.  Converting them to float corrupts
  the LCG recurrence (the multiplier 1220703125 rounds to 1220703072),
  producing different matrix sparsity structures and different particle
  streams -- which would confound every branch comparison.  CG and MG say so
  in their headers ("generation-double / solve-fp32").  So an fp32 build
  legitimately contains double-precision arithmetic; the gate warns only when
  a build has NO float ops at all.

WHY THIS IS NOT THE FPCHECKER HARNESS WITH A DIFFERENT COMPILER

  * No eta sweep.  ERRORTHRESHOLD is a COMPILE-TIME constant in
    $EFT_HOME/runtime/handleReal.h (45), and that header is not tracked as a
    make dependency -- `rm -rf obj` before rebuilding or the edit does
    nothing silently.
  * -O0 only, enforced.  Adjudicate against the -O0 census.
  * The Makefiles are not used to build; the TU list is read FROM them.

BUILD PIPELINE (ordering is load-bearing)

    1. clang -O0 -g -emit-llvm -c (each TU)   -> one .bc per TU
    2. llvm-link                             -> <b>_merged.bc
    3. opt -load libEFTSanitizer.so -eftsan  -> <b>.opt.bc
    4. clang -O0 <b>.opt.bc -leftsanitizer   -> <b>_<prec>.eftsan

  Step 2 must precede step 3: instrumenting TUs separately corrupts argument
  shadow bookkeeping at call sites.  BT/CG/LU/SP/MG are single-TU so the
  merge is a formality; EP and IS are multi-TU and genuinely need it.

  This is where NAS previously bit: EP's vranlc(x_seed=0x0, a=nan) and IS's
  c_print_results(t=2.09e-317) were both the separate-instrumentation bug.

KNOWN CRASH: LARGE-ARITY REPORTING FUNCTIONS

  c_print_results takes ~18 arguments and has segfaulted in the EFTSan
  prologue (IS).  If a run dies with empty stdout and exit 139, check the
  arity of the function in the backtrace before suspecting the build.  The
  fix used previously was replacing the CALL with a plain printf in the
  benchmark source, leaving FP compute and branch structure untouched.

VERIFICATION IS A GATE, NOT DECORATION

  NAS prints "Verification Successful" / "Verification failed" (or
  "Verification    =    SUCCESSFUL").  The verify block is itself a cluster
  of FP comparisons and a large part of the interesting branch set, so a run
  that skips verification is not comparable to the census.

  BT's reference table is NITER-locked to the canonical class: a
  non-canonical NITER gives class 'U', no verification, and zero verify
  branches.  This harness never changes NITER.

  IS exits with status 26 on SUCCESSFUL verification.  That is success, not a
  crash, and is treated as such here.

AUTOMATIC CENSUS CROSS-CHECK

  The benchmark trees carry brtrace's *.brsites files.  When they are found,
  this harness compares its instrumented site set against them and writes
  site_agreement.txt: branch counts, unique-site counts, and both set
  differences.  On LULESH, AMG and QuickSilver the site sets matched exactly
  (74=74, 544=544, 86=86), which is what licenses reading an EFTSan silence
  at a census site as a detection failure rather than a coverage gap.
"""

import argparse
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

# --------------------------------------------------------------------
EFT_HOME = Path(os.environ.get("EFT_HOME", "/usr/workspace/das9/EFTSanitizer"))
SETUP_SH = Path(os.environ.get("EFT_SETUP", EFT_HOME / "setup_eftsan.sh"))

HERE = Path(__file__).resolve().parent
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT", HERE.parent.parent / "benchmarks" / "nas"))

WORK_ROOT = HERE / "nas"

# Per-benchmark precision flags.  See the module docstring: three different
# mechanisms, and an empty string means "this tree already IS that precision".
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
           "note": "integer sort -- fp32 and fp64 trees are identical; "
                   "NO float arithmetic by design, and no FP branches"},
    "mg": {"define": {"fp32": "", "fp64": ""},
           "std": "c99",
           "note": "separately-edited trees; precision baked into the source"},
}
IS_SUCCESS_EXIT = {0, 26}      # IS returns 26 on successful verification

ERROR_LINE_RE = re.compile(r"\berror:")
UNDEF_RE = re.compile(r"undefined reference to [`']([^'\"]+)'")
FLIP_RE = re.compile(r"branch flip @ file\s+(\d+)\s+line\s+(\d+)")

def fnv1a(name):
    """FNV-1a over the file BASENAME.  Must match handleFcmp in
    EFTSanitizer.cpp exactly -- EFTSan now prints a file id with every flip,
    so a bare line number is no longer ambiguous in multi-TU builds."""
    h = 14695981039346656037
    for c in name.encode():
        h = ((h ^ c) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h

TOTAL_RE = re.compile(r"Total branch flips found\s+(\d+)")
VERIFY_OK_RE = re.compile(r"Verification\s+(?:Successful|=\s*SUCCESSFUL)", re.I)
VERIFY_BAD_RE = re.compile(r"Verification\s+(?:failed|=\s*UNSUCCESSFUL)", re.I)
CLASS_RE = re.compile(r"[Cc]lass\s*=?\s*([A-Z])\b")
MAKE_SRC_RE = re.compile(r"^\s*SRCS?\s*:?\??=\s*(.+)$", re.M)


def sh(cmd, cwd=None, env=None, log=None):
    p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, text=True, errors="replace")
    if log:
        with open(log, "a") as fh:
            fh.write("$ " + " ".join(map(str, cmd)) + "\n")
            fh.write(p.stdout + "\n")
    return p.returncode, p.stdout


def sh_split(cmd, cwd=None, env=None, timeout=None):
    """Flip prints go to stderr, benchmark output to stdout."""
    try:
        p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE, text=True,
                           errors="replace", timeout=timeout)
        return p.returncode, p.stdout, p.stderr, False
    except subprocess.TimeoutExpired as e:
        return None, e.stdout or "", e.stderr or "", True


def eftsan_env():
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
    """MPFR and GMP are EFTSan's shadow arithmetic but live in the conda env,
    not in $EFT_HOME/runtime/obj, and setup_eftsan.sh does not export them.
    Without this the binary links cleanly and dies at exec with exit 127."""
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


# --------------------------------------------------------------------
# line -> file resolution, restricted to instrumented branch sites.
# --------------------------------------------------------------------
DIFILE_RE = re.compile(r'^!(\d+) = !DIFile\(filename: "([^"]+)"')
SCOPE_RE = re.compile(
    r'^!(\d+) = (?:distinct )?!DI(?:Subprogram|LexicalBlock|LexicalBlockFile)'
    r'\((?P<body>.*)\)\s*$')
LOC_RE = re.compile(r'^!(\d+) = !DILocation\(line: (\d+),.*?scope: !(\d+)')
FILE_FIELD_RE = re.compile(r'\bfile: !(\d+)')
SCOPE_FIELD_RE = re.compile(r'\bscope: !(\d+)')
BRANCH_CALL_RE = re.compile(r'call\b.*@(eftsan_check_branch\w*)')
DBG_REF_RE = re.compile(r'!dbg !(\d+)')
FP_OP_RE = re.compile(r'\b(?:fadd|fsub|fmul|fdiv|fcmp\s+\w+)\s+'
                      r'(?:fast |nnan |ninf |nsz |arcp |contract |afn |reassoc )*'
                      r'(float|double|x86_fp80)\b')


def _parse_debug_info(ll_path):
    files, scope_file, scope_parent, loc_by_id = {}, {}, {}, {}
    branch_dbg = set()
    # Hooks and distinct debug locations are DIFFERENT counts: several hooks
    # can share one !dbg (compound conditions).  Report both.
    n_calls = [0]
    try:
        with open(ll_path, errors="replace") as fh:
            for line in fh:
                if line.startswith("!"):
                    m = DIFILE_RE.match(line)
                    if m:
                        files[m.group(1)] = os.path.basename(m.group(2))
                        continue
                    m = LOC_RE.match(line)
                    if m:
                        loc_by_id[m.group(1)] = (int(m.group(2)), m.group(3))
                        continue
                    m = SCOPE_RE.match(line)
                    if m:
                        body = m.group("body")
                        f = FILE_FIELD_RE.search(body)
                        sc = SCOPE_FIELD_RE.search(body)
                        if f:
                            scope_file[m.group(1)] = f.group(1)
                        if sc:
                            scope_parent[m.group(1)] = sc.group(1)
                    continue
                if BRANCH_CALL_RE.search(line):
                    n_calls[0] += 1
                    d = DBG_REF_RE.search(line)
                    if d:
                        branch_dbg.add(d.group(1))
    except OSError:
        return None
    return files, scope_file, scope_parent, loc_by_id, branch_dbg, n_calls[0]


def build_line_map(merged_bc, opt_bc, env, mode="branches", log=None,
                   keep_ll=False):
    """Returns (line_map, n_hooks, n_locs).  mode="branches" maps only lines
    carrying an EFTSan branch hook in the INSTRUMENTED module -- exactly the
    sites that can produce a flip print."""
    target = opt_bc if mode == "branches" else merged_bc
    ll = target.with_suffix(".ll")
    rc, _ = sh(["llvm-dis", str(target), "-o", str(ll)], env=env, log=log)
    if rc != 0 or not ll.exists():
        return {}, 0, 0
    parsed = _parse_debug_info(ll)
    if not keep_ll:
        try:
            ll.unlink()
        except OSError:
            pass
    if parsed is None:
        return {}, 0, 0
    files, scope_file, scope_parent, loc_by_id, branch_dbg, n_calls = parsed

    def resolve(scope, depth=0):
        if scope is None or depth > 32:
            return None
        if scope in scope_file:
            return files.get(scope_file[scope])
        return resolve(scope_parent.get(scope), depth + 1)

    ids = branch_dbg if mode == "branches" else loc_by_id.keys()
    out = defaultdict(set)
    for did in ids:
        if did not in loc_by_id:
            continue
        lineno, scope = loc_by_id[did]
        fn = resolve(scope)
        if fn:
            out[lineno].add(fn)
    if mode == "branches" and not out:
        if n_calls == 0:
            # No hooks in the module at all.  For an integer benchmark that
            # is the correct answer; do NOT fall back, because mapping every
            # DILocation would fabricate a site universe that has nothing to
            # do with what the tool can report.
            print("  no EFTSan branch hooks in this module -- the tool "
                  "instrumented zero FP-controlled branches")
            return {}, 0, 0
        print("  (hooks present but none resolved to a file -- falling back "
              "to every DILocation; check BRANCH_CALL_RE against the pass)")
        return build_line_map(merged_bc, opt_bc, env, mode="all", log=log,
                              keep_ll=keep_ll)
    return {k: sorted(v) for k, v in out.items()}, n_calls, len(branch_dbg)


def count_fp_ops(merged_bc, env):
    """{float: n, double: n, x86_fp80: n} over the UNINSTRUMENTED module.

    The precision gate.  BT/CG/LU/SP take -DNAS_FP32, but EP uses WORKING_T
    and IS/MG take no flag at all -- so a flag typo produces a clean build of
    the wrong precision, silently.  Note an fp32 NAS build legitimately keeps
    double arithmetic in the RNG (randlc needs the 53-bit mantissa), so the
    test is 'has float ops', not 'has no double ops'."""
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


def write_site_universe(line_map, outdir, name="instrumented_sites.csv"):
    rows = []
    for ln, files in line_map.items():
        for f in files:
            rows.append((f, ln, len(files) > 1))
    rows.sort(key=lambda r: (r[0], r[1]))
    path = outdir / name
    with open(path, "w") as fh:
        fh.write("location,ambiguous\n")
        for f, ln, amb in rows:
            fh.write(f"{f}:{ln},{int(amb)}\n")
    return path


def compare_with_brsites(src_tree, line_map, n_hooks, outdir):
    """Cross-check the instrumented site set against brtrace's *.brsites.

    Matching COUNTS does not prove matching SETS, and the difference decides
    whether an unreported census site is a false negative or a coverage gap.
    brsites rows are: site_id \\t file:line \\t function, with '#' headers.
    brtrace stores full paths, we store basenames, so compare basenames."""
    files = sorted(src_tree.glob("*.brsites"))
    if not files:
        return None
    brt_branches, brt_sites = 0, set()
    for f in files:
        for line in f.read_text(errors="replace").splitlines():
            if line.startswith("#") or not line.strip():
                continue
            parts = line.split("\t")
            if len(parts) < 2:
                continue
            brt_branches += 1
            brt_sites.add(os.path.basename(parts[1].strip()))
    eft_sites = {f"{f}:{ln}" for ln, fl in line_map.items() for f in fl}
    only_brt = sorted(brt_sites - eft_sites)
    only_eft = sorted(eft_sites - brt_sites)

    lines = [
        "instrumented site agreement: EFTSan vs brtrace census", "",
        f"  brtrace branches : {brt_branches}",
        f"  EFTSan hooks     : {n_hooks}",
        f"  brtrace sites    : {len(brt_sites)}",
        f"  EFTSan sites     : {len(eft_sites)}", "",
    ]
    if not only_brt and not only_eft:
        lines.append("  SITE SETS IDENTICAL.")
        lines.append("  An EFTSan silence at a census site is therefore a")
        lines.append("  detection failure, not a coverage gap.")
    else:
        if only_brt:
            lines.append(f"  brtrace only ({len(only_brt)}) -- sites EFTSan "
                         f"cannot see; unreported flips here are COVERAGE "
                         f"GAPS, not false negatives:")
            lines += [f"      {s}" for s in only_brt[:40]]
        if only_eft:
            lines.append(f"  EFTSan only ({len(only_eft)}):")
            lines += [f"      {s}" for s in only_eft[:40]]
    if brt_branches != n_hooks:
        lines += ["",
                  "  NOTE: branch counts differ while sites may still match.",
                  "  brtrace counts source-level conditions, EFTSan counts IR",
                  "  hooks; a compound condition on one line yields several of",
                  "  either.  Adjudication is site-keyed, so this is a",
                  "  counting convention, not a coverage difference."]
    (outdir / "site_agreement.txt").write_text("\n".join(lines) + "\n")
    return {"brtrace_branches": brt_branches, "brtrace_sites": len(brt_sites),
            "eftsan_hooks": n_hooks, "eftsan_sites": len(eft_sites),
            "only_brtrace": only_brt, "only_eftsan": only_eft,
            "sets_identical": not only_brt and not only_eft}


# --------------------------------------------------------------------
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
    """Drop TUs referencing symbols nothing in the merge defines (archive
    semantics).  A TU defining main() is never dropped."""
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
    """TU list from the Makefile's SRC=/SRCS= line.  BT/CG/LU/SP use
    $(wildcard *.c); EP and IS name their files explicitly."""
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
    """A tool that cannot build is a RESULT, not a missing run."""
    rec = {"benchmark": bench, "precision": precision, "opt": "O0",
           "status": "tool_failure", "failed_stage": stage, "reason": reason,
           "detail": detail, "flips": None, "locations": None, "sites": {},
           "exit_code": None}
    if extra:
        rec.update(extra)
    lines = [f"{bench} {precision} -- EFTSanitizer: NO RESULT", "",
             f"TOOL FAILURE at {stage}.", "", f"  {reason}", ""]
    lines += ["  " + l for l in detail.splitlines()]
    lines += ["", "This is a tool limitation, not a missing experiment: the",
              "build was attempted and EFTSan refused it.", ""]
    (outdir / "summary.txt").write_text("\n".join(lines))
    (outdir / "summary.json").write_text(json.dumps([rec], indent=2))
    return rec


VECTOR_TYPE_RE = re.compile(r"<\d+ x (?:float|double|half)>")


def diagnose_vector_types(merged_bc, env, limit=6):
    """EFTSan's pass exits on vector-typed loads.  At -O0 these are SysV
    argument coercion (a small by-value float struct packed into <2 x float>),
    not vectorization -- the QuickSilver fp32 failure."""
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


# --------------------------------------------------------------------
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
        print("  opt -eftsan on the merged module")
        rc, out = sh(["opt", "-load", str(pass_so), "-eftsan",
                      str(merged), "-o", str(opt_bc)],
                     cwd=dst, env=env, log=log)
        if rc != 0 or not opt_bc.exists() or opt_bc.stat().st_size == 0:
            tail = [l for l in out.splitlines() if l.strip()][-6:]
            msg = ("INSTRUMENTATION FAILED.  Last opt output:\n" +
                   "\n".join("      " + l[:150] for l in tail))
            if "vector" in out.lower():
                nvec, vfuncs = diagnose_vector_types(merged, env)
                msg += (f"\n    {nvec} vector-typed value(s): at -O0 this is "
                        f"SysV argument coercion, not vectorization.")
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
            print(f"  NO RESULT: EFTSan refuses this module (vector loads). "
                  f"Recorded as a tool failure.")
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

    _, nm_out = sh(["nm", str(binary)], env=env)
    nsym = sum(1 for l in nm_out.splitlines() if "eftsan" in l.lower())
    print(f"  eftsan symbol count = {nsym}")

    fp_ops = count_fp_ops(merged, env)
    print(f"  FP ops in module: " +
          ", ".join(f"{k}={v}" for k, v in sorted(fp_ops.items())) or "none")
    n_float = fp_ops.get("float", 0)
    n_double = fp_ops.get("double", 0)
    prec_ok = True
    expect_float = cfg.get("expect_float", True)
    if precision == "fp32" and n_float == 0 and not expect_float:
        print("  precision gate: skipped -- this benchmark has no float "
              "arithmetic by design")
    elif precision == "fp32" and n_float == 0:
        prec_ok = False
        print("  *** PRECISION GATE: fp32 build has NO float arithmetic. The "
              "precision flag did not take effect.")
    if precision == "fp64" and n_float > n_double:
        print("  *** PRECISION GATE: fp64 build is mostly float ops -- check "
              "the tree.")
        prec_ok = False

    print(f"  resolving line numbers from debug info (mode={args.resolve})")
    line_map, n_hooks, n_locs = build_line_map(merged, opt_bc, env,
                                               mode=args.resolve, log=log,
                                               keep_ll=args.keep_ll)
    collisions = sum(1 for v in line_map.values() if len(v) > 1)
    print(f"  instrumented branch hooks = {n_hooks} "
          f"({n_locs} distinct debug locations)")
    print(f"  lines mapped = {len(line_map)} ({collisions} multi-file)")
    write_site_universe(line_map, outdir)
    agree = compare_with_brsites(src, line_map, n_hooks, outdir)
    if agree:
        verdict = ("IDENTICAL" if agree["sets_identical"]
                   else f"{len(agree['only_brtrace'])} brtrace-only, "
                        f"{len(agree['only_eftsan'])} EFTSan-only")
        print(f"  census cross-check: brtrace {agree['brtrace_branches']} "
              f"branches / {agree['brtrace_sites']} sites -> site sets "
              f"{verdict}")
    else:
        print("  census cross-check: no *.brsites in the source tree")

    (outdir / "build_info.txt").write_text(
        f"benchmark    = NAS {bench.upper()}\n"
        f"precision    = {precision}\n"
        f"opt          = -O0  (enforced)\n"
        f"compiler     = clang (LLVM 10 via {SETUP_SH.name}), C\n"
        f"precision by = {define or cfg['note']}\n"
        f"TUs compiled = {len(bcs)} (from {how})\n"
        f"TUs pruned   = {len(pruned_total)} "
        f"({', '.join(pruned_total) or 'none'})\n"
        f"cflags       = {' '.join(cflags)}\n"
        f"eftsan_syms  = {nsym}\n"
        f"fp_ops       = {fp_ops}\n"
        f"branch_hooks = {n_hooks}  (eftsan_check_branch call sites)\n"
        f"debug_locs   = {n_locs}\n"
        f"sites        = {sum(len(v) for v in line_map.values())}\n"
        f"census       = {json.dumps(agree) if agree else 'no brsites found'}\n"
        f"RNG stays fp64 in every variant (randlc needs the 53-bit mantissa),\n"
        f"so double ops in an fp32 build are expected.\n"
        f"ERRORTHRESHOLD is compiled into the runtime (45); no eta sweep.\n")

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
    return binary, line_map, local_files, n_hooks, n_locs, fp_ops, agree


# --------------------------------------------------------------------
def run(bench, binary, precision, outdir, line_map, local_files, env,
        n_hooks, timeout):
    print(f"  running {binary.name}")
    rc, out, err, timed_out = sh_split(
        ["stdbuf", "-i0", "-o0", "-e0", str(binary)],
        cwd=binary.parent, env=env, timeout=timeout)
    (outdir / "run_O0.stdout").write_text(out)
    (outdir / "run_O0.stderr").write_text(err)
    if timed_out:
        print(f"  *** TIMEOUT after {timeout}s -- partial output kept. "
              f"Raise --timeout; EFTSan slows NAS by orders of magnitude.")

    lines = Counter()
    for stream in (err, out):
        for m in FLIP_RE.finditer(stream):
            lines[(int(m.group(1)), int(m.group(2)))] += 1
    parsed = sum(lines.values())
    tm = TOTAL_RE.search(err) or TOTAL_RE.search(out)
    runtime_total = int(tm.group(1)) if tm else None

    sites, foreign, ambiguous, unresolved = Counter(), Counter(), [], []
    for (fhash, ln), cnt in lines.most_common():
        cands = [f for f in line_map.get(ln, []) if fnv1a(f) == fhash]
        if not cands:
            cands = line_map.get(ln, [])
        if len(cands) == 1:
            key = f"{cands[0]}:{ln}"
            sites[key] = cnt
            if cands[0] not in local_files:
                foreign[key] = cnt
        elif len(cands) > 1:
            sites[f"AMBIGUOUS:{ln}  [{' | '.join(cands)}]"] = cnt
            ambiguous.append(ln)
        else:
            sites[f"line:{ln}"] = cnt
            unresolved.append(ln)

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
        print(f"  *** verification FAILED (class {nas_class}) -- for fp32 "
              f"this may be the real result, not a bug")
    else:
        print("  *** no verification line in the output. The verify block is "
              "a large part of the interesting branch set; a run that skips "
              "it is not comparable to the census.")
    if not ok_exit and not timed_out:
        # subprocess reports a killing signal as a NEGATIVE return code, so
        # -11 and 139 are the same SIGSEGV seen from two directions.
        segv = ("SIGSEGV -- check the ARITY of the function in the backtrace; "
                "c_print_results takes ~18 args and has crashed the EFTSan "
                "prologue before")
        why = {139: segv, -11: segv,
               134: "abort -- usually an MPFR assertion in the shadow",
               -6: "abort -- usually an MPFR assertion in the shadow",
               127: "loader failure"}
        print(f"  *** exit {rc}: {why.get(rc, 'see the stderr log')}")
        for l in (err or out).splitlines()[:3]:
            if l.strip():
                print("     ", l[:150])
    elif bench == "is" and rc == 26:
        print("  exit 26 = IS verification SUCCESSFUL (not a crash)")

    return {
        "benchmark": bench, "precision": precision, "opt": "O0",
        "threshold": 45, "branch_hooks": n_hooks,
        "flips": parsed, "runtime_total": runtime_total,
        "locations": len(sites), "sites": dict(sites.most_common()),
        "foreign_sites": dict(foreign),
        "foreign_flips": sum(foreign.values()),
        "ambiguous_lines": sorted(ambiguous),
        "ambiguous_flips": sum(c for k, c in sites.items()
                               if k.startswith("AMBIGUOUS:")),
        "unresolved_lines": sorted(unresolved),
        "unresolved_flips": sum(c for (_fh, ln), c in lines.items()
                                if ln in unresolved),
        "verified": verified, "class": nas_class,
        "timed_out": timed_out, "exit_code": rc, "exit_ok": ok_exit,
    }


def verdict_of(v):
    return "SUCCESSFUL" if v else ("FAILED" if v is False else "ABSENT")


def write_results(bench, precision, records, outdir, fp_ops, agree):
    r0 = records[0]
    lines = [f"NAS {bench.upper()} {precision} -- EFTSanitizer branch flips",
             f"(-O0 compile+link, merged module, serial, class "
             f"{r0['class'] or '?'})", ""]
    for r in records:
        own = (r["flips"] - r["foreign_flips"] - r["ambiguous_flips"]
               - r["unresolved_flips"])
        lines.append(f"{r['flips']} flips @ {r['locations']} loc"
                     f"   (ERRORTHRESHOLD={r['threshold']}, no sweep)"
                     f"   [in-TU {own}, foreign {r['foreign_flips']}, "
                     f"ambiguous {r['ambiguous_flips']}, "
                     f"unresolved {r['unresolved_flips']}]")
    lines += ["",
              f"{r0['branch_hooks']} instrumented branch hooks.  "
              f"FP ops: {fp_ops}.",
              f"verification: {verdict_of(r0['verified'])}"
              f"   exit code: {r0['exit_code']}", ""]
    if agree:
        lines.append(f"census cross-check: brtrace {agree['brtrace_branches']} "
                     f"branches / {agree['brtrace_sites']} sites vs EFTSan "
                     f"{agree['eftsan_hooks']} hooks / "
                     f"{agree['eftsan_sites']} sites -- site sets "
                     f"{'IDENTICAL' if agree['sets_identical'] else 'DIFFER'}"
                     f" (see site_agreement.txt)")
        lines.append("")
    lines += [
        "NOTE: EFTSan prints a bare line number.  Files above are recovered",
        "      from the INSTRUMENTED module: each site is a line carrying an",
        "      eftsan_check_branch hook, resolved via the debug-info scope",
        "      chain (-O0, no inlining).  The RNG stays fp64 in every",
        "      variant, so double ops in an fp32 build are expected.",
        "      Ground truth is the -O0 census.", "",
    ]
    for r in records:
        lines.append(f"--- O0 : {r['flips']} flips @ {r['locations']} loc ---")
        if not r["sites"]:
            lines.append("    (no flips)")
        for site, cnt in r["sites"].items():
            lines.append(f"    {cnt:9d}  {site}")
        if r["runtime_total"] is not None:
            lines.append(f"    runtime total: {r['runtime_total']}")
        if r["ambiguous_lines"]:
            lines.append(f"    AMBIGUOUS lines: {r['ambiguous_lines']}")
        if r["unresolved_lines"]:
            lines.append(f"    UNRESOLVED: {r['unresolved_lines']}")
        if r["timed_out"]:
            lines.append("    *** RUN TIMED OUT -- counts are partial")
        lines.append("")
    (outdir / "summary.txt").write_text("\n".join(lines))
    (outdir / "summary.json").write_text(json.dumps(records, indent=2))
    with open(outdir / f"{bench}_{precision}_eftsan_summary.csv", "w") as fh:
        fh.write("location,flips\n")
        for site, cnt in records[0]["sites"].items():
            fh.write(f"{site.split('  [')[0]},{cnt}\n")


# --------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("benchmark", help="bt cg ep is lu mg sp, or 'all'")
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("--timeout", type=int, default=3600,
                    help="per-run seconds (default 3600; EFTSan slows NAS by "
                         "orders of magnitude and SP/BT are the big ones)")
    ap.add_argument("--resolve", default="branches",
                    choices=["branches", "all"])
    ap.add_argument("--link-override", action="store_true")
    ap.add_argument("--keep-bc", action="store_true")
    ap.add_argument("--keep-ll", action="store_true")
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
        print(f"FATAL: cannot locate {', '.join(missing_libs)}; is eftsan_env "
              f"active?")
        return 1
    env = with_lib_path(env, lib_dirs)
    missing = check_tools(env)
    if missing:
        print(f"FATAL: not on PATH after sourcing {SETUP_SH.name}: "
              f"{', '.join(missing)}")
        return 1

    print(f"NAS / EFTSanitizer   -O0 (enforced)   timeout {args.timeout}s")
    print(f"  benchmarks: {' '.join(benches)}")
    print(f"  precisions: {' '.join(args.precision)}")
    print(f"  bench root: {BENCH_ROOT}")
    print(f"  libs:       {' '.join(str(d) for d in lib_dirs)}")
    print("  no eta sweep: ERRORTHRESHOLD is compiled into the runtime (45)\n")

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
            binary, line_map, local_files, n_hooks, n_locs, fp_ops, agree = built
            if args.no_run:
                print(f"  built: {binary}\n")
                continue
            rec = run(bench, binary, precision, outdir, line_map, local_files,
                      env, n_hooks, args.timeout)
            write_results(bench, precision, [rec], outdir, fp_ops, agree)
            grand[(bench, precision)] = rec
            print(f"  -> {rec['flips']} flips @ {rec['locations']} loc\n")

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
              f"{r['locations']:3d} loc   {v}   exit={r['exit_code']}{t}")
    print(f"\nnas/<bench>/results/O0/<precision>/")
    return 0


if __name__ == "__main__":
    sys.exit(main())