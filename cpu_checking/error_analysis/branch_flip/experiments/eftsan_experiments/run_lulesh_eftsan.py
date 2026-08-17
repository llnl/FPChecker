#!/usr/bin/env python3
"""
run_lulesh_eftsan.py

Build LULESH under EFTSanitizer branch-flip instrumentation and run it,
keeping fp32 and fp64 results completely separate.  Counterpart to
run_lulesh_fpchecker.py.

Place in: branch_flip/experiments/eftsan_experiments/

    ./run_lulesh_eftsan.py                    # both precisions
    ./run_lulesh_eftsan.py -p fp32            # fp32 only
    ./run_lulesh_eftsan.py -s 10 -i 50        # bigger problem
    ./run_lulesh_eftsan.py --no-run           # build and gate only

Layout produced (mirrors the FPChecker harness, but there is only ever an
O0 subtree -- see below):
    lulesh/
      results/O0/  fp32/  build.log  run_O0.stdout  run_O0.stderr
                          summary.txt  summary.json  build_info.txt
                          lulesh_fp32_eftsan_summary.csv
                   fp64/  ...
      build/O0/    lulesh_fp32/  lulesh_fp64/

fp32 and fp64 only.  summary.txt / summary.json use the same shape as
run_lulesh_fpchecker.py -- summary.json is a LIST of run records, so a reader
can treat both tools' output identically.  FPChecker's list has three entries
(one per eta); this one has exactly one, because EFTSan has no eta.

WHY THIS IS NOT THE FPCHECKER HARNESS WITH A DIFFERENT COMPILER

  * No eta sweep.  EFTSan has no runtime knob.  Its sensitivity is
    ERRORTHRESHOLD, a COMPILE-TIME constant in $EFT_HOME/runtime/handleReal.h,
    and handleReal.h is not tracked as a make dependency -- `rm -rf obj`
    before rebuilding or the edit silently does nothing.  Keep it at 45.
    One run per precision, not three.

  * -O0 only, enforced rather than defaulted.  EFTSan's shadow bookkeeping
    does not survive optimisation.  CONSEQUENCE: these results adjudicate
    against the -O0 brtrace census, never the -O2 one.  The FPChecker LULESH
    sweeps are -O2; mixing the two repeats the -O0-vs-O2 non-comparability
    error.  There is deliberately no --opt flag.

  * The LULESH Makefile is not used at all.  EFTSan needs per-TU bitcode,
    then a merge, then the pass, then a link -- a Makefile that goes
    .cc -> .o cannot express that.  All five TUs are compiled by hand here.

BUILD PIPELINE (the ordering is load-bearing)

    1. clang++ -O0 -g -emit-llvm -c   x5      -> lulesh*.bc
    2. llvm-link the five .bc                 -> lulesh_merged.bc
    3. opt -load libEFTSanitizer.so -eftsan   -> lulesh.opt.bc
    4. clang++ -O0 lulesh.opt.bc -leftsanitizer -> lulesh_<prec>.eftsan

  Step 2 MUST precede step 3.  Instrumenting TUs separately and linking the
  objects corrupts function-argument shadow bookkeeping at call sites: the
  callee prologue's eftsan_get_arg sequence reads garbage and the program
  dies in the prologue even though the IR signatures match.  This is the one
  step no NAS benchmark ever needed -- every NAS benchmark is single-file.

  Step 3 is legacy pass manager syntax and needs an explicit -load.
  $LLVM_PASS_LIB from setup_eftsan.sh expands to TWO tokens
  ("<path>/libEFTSanitizer.so -eftsan") and must never be quoted as one.

  Step 4 needs -lm -lmpfr -lgmp -lstdc++.  The runtime is libeftsanitizer.so
  (lowercase) in $EFT_HOME/runtime/obj, linked as -leftsanitizer.  MPFR is
  EFTSan's high-precision shadow; omitting it gives undefined mpfr_* at link.

  -std=c++11 is LULESH's actual baseline.  The EFTSan toolchain is LLVM 10,
  so anything newer is a gamble; c++14 also works but buys nothing.

OUTPUT KEYING -- THE ADJUDICATION TRAP

  EFTSan's flip print emits a BARE LINE NUMBER, no filename:
      branch flip @ line 2079
  The brtrace census and FPChecker summaries are keyed on file:line, so they
  do NOT cross-match.  This harness resolves line -> file from the merged
  module's debug info (DILocation -> scope -> DIFile; at -O0 there is no
  inlining, so the scope chain resolves honestly) and emits census-shaped
  keys.  A line number occurring in more than one file is flagged AMBIGUOUS
  rather than silently attributed.

  That is exactly the lulesh.cc:263 / stl_algobase.h:263 collision: the
  census site is std::max in stl_algobase.h, and line 263 of lulesh.cc is an
  unrelated array store in CalcKinematics.  FPChecker cannot tell them apart
  because it reads DILocation.line without walking getInlinedAt().  Here the
  pair shows up as AMBIGUOUS:263 and must be adjudicated by hand or dropped.

VERIFICATION GATES (a clean build proves nothing)

  1. eftsan symbol count in the binary -- zero means the pass did not run.
  2. runtime total ("Total branch flips found N") vs parsed total -- a
     mismatch means the flip line format changed and the parser is blind.
  3. non-zero exit is reported.  139 = segfault: check the ARITY of the
     function in the backtrace before suspecting the build.  Large-arity
     reporting functions blow up in the EFTSan prologue (NAS IS died in
     c_print_results, ~18 args; fixed by replacing the CALL with a plain
     printf, leaving FP compute and branch structure byte-identical).

NOT A BUG: five- and six-figure flip counts.  EFTSan over-flags heavily --
NAS SP produced 506,717 (fp32) and 834,911 (fp64) events from max() macros.
LULESH has std::max and FABS everywhere.  Big numbers are expected.

functions.txt appearing in the build directory is an OUTPUT LOG appended to
by the opt pass, not an input instrument-list.  Editing it has no effect.
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
# Defaults -- override on the command line or via environment.
# --------------------------------------------------------------------
EFT_HOME = Path(os.environ.get("EFT_HOME", "/usr/workspace/das9/EFTSanitizer"))
SETUP_SH = Path(os.environ.get("EFT_SETUP", EFT_HOME / "setup_eftsan.sh"))

HERE = Path(__file__).resolve().parent
# .../branch_flip/experiments/eftsan_experiments -> .../branch_flip/benchmarks
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT", HERE.parent.parent / "benchmarks" / "lulesh"))

BENCH_NAME = "lulesh"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build" / "O0"
RESULT_ROOT = WORK_ROOT / "results" / "O0"

SRCS = ["lulesh.cc", "lulesh-comm.cc", "lulesh-init.cc",
        "lulesh-util.cc", "lulesh-viz.cc"]
# fp32 and fp64 only.  There is no ld path here: EFTSan's shadow is MPFR,
# so long double buys nothing it does not already have, and the oracle
# ld trajectory is brtrace's job, not the tool's.
PRECISION_FLAG = {"fp32": "-DLULESH_FP32", "fp64": ""}

# EFTSan prints one line per flip, to stderr, with no filename.
# "error" alone matches any path containing e.g. error_analysis/, which
# silently swallowed the real compiler message.  Require the colon clang emits.
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


def sh(cmd, cwd=None, env=None, log=None, append=True):
    p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, text=True, errors="replace")
    if log:
        with open(log, "a" if append else "w") as fh:
            fh.write("$ " + " ".join(map(str, cmd)) + "\n")
            fh.write(p.stdout + "\n")
    return p.returncode, p.stdout


def sh_split(cmd, cwd=None, env=None):
    """Keeps stdout and stderr apart -- EFTSan's flip prints go to stderr and
    LULESH's own output to stdout; conflating them makes the parse
    ambiguous."""
    p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE, text=True, errors="replace")
    return p.returncode, p.stdout, p.stderr


def eftsan_env():
    """setup_eftsan.sh puts LLVM 10's clang/opt/llvm-link on PATH, sets
    EFT_HOME and LLVM_PASS_LIB, and puts runtime/obj on LD_LIBRARY_PATH.
    Python cannot source a shell script, so run it under bash and harvest the
    resulting environment wholesale."""
    if not SETUP_SH.exists():
        print(f"FATAL: no setup script at {SETUP_SH}")
        print("       set EFT_SETUP=/path/to/setup_eftsan.sh")
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



# --------------------------------------------------------------------
# MPFR and GMP are EFTSan's shadow arithmetic, but they live in the conda
# env, not in $EFT_HOME/runtime/obj.  setup_eftsan.sh puts the runtime on
# LD_LIBRARY_PATH and stops there, so a binary that links cleanly still dies
# at exec with:
#     error while loading shared libraries: libmpfr.so.6
# and exit 127.  Locate them once, then both -Wl,-rpath them into the binary
# (so it runs standalone, outside this harness) and put them on
# LD_LIBRARY_PATH (so it runs even if the rpath is stripped).
# --------------------------------------------------------------------
NEEDED_LIBS = ["libmpfr.so", "libgmp.so"]


def find_lib_dirs(env):
    """Return (dirs, missing).  Searches conda prefixes, whatever is already
    on LD_LIBRARY_PATH, and the usual system locations."""
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


# --------------------------------------------------------------------
# line -> file resolution from the merged module's debug info.
#
# DILocation -> scope -> (DISubprogram | DILexicalBlock | DILexicalBlockFile)
# -> DIFile.  At -O0 there is no inlining, so this is honest.
# --------------------------------------------------------------------
DIFILE_RE = re.compile(r'^!(\d+) = !DIFile\(filename: "([^"]+)"')
SCOPE_RE = re.compile(
    r'^!(\d+) = (?:distinct )?!DI(?:Subprogram|LexicalBlock|LexicalBlockFile)'
    r'\((?P<body>.*)\)\s*$')
LOC_RE = re.compile(r'^!(\d+) = !DILocation\(line: (\d+),.*?scope: !(\d+)')
FILE_FIELD_RE = re.compile(r'\bfile: !(\d+)')
# The EFTSan branch hooks: eftsan_check_branch_f / _d / _fd / ... one call is
# emitted per instrumented FP comparison, carrying that comparison's !dbg.
BRANCH_CALL_RE = re.compile(r'call\b.*@(eftsan_check_branch\w*)')
DBG_REF_RE = re.compile(r'!dbg !(\d+)')
SCOPE_FIELD_RE = re.compile(r'\bscope: !(\d+)')


def _parse_debug_info(ll_path):
    """Return (files, scope_file, scope_parent, loc_by_id, branch_dbg_ids).

    branch_dbg_ids is the set of !dbg metadata ids attached to calls into the
    EFTSan branch hooks.  Those calls ARE the instrumented branch sites, so
    restricting the line map to them is exact: it cannot invent a candidate
    file that has no instrumented branch on that line."""
    files, scope_file, scope_parent, loc_by_id = {}, {}, {}, {}
    branch_dbg = set()
    # Hooks and distinct debug locations are DIFFERENT counts.  Several hooks
    # can share one !dbg (compound conditions, or two comparisons the front
    # end gives the same location).  LULESH and AMG happen to have one hook
    # per location; QuickSilver does not.  Report both so the number matches
    # its label.
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


def build_line_map(merged_bc, opt_bc, env, mode="branches", log=None):
    """{line_number: sorted[filenames]}.

    mode="branches" (default): map ONLY the lines carrying an EFTSan branch
    hook, read from the INSTRUMENTED module.  These are precisely the sites
    that can ever produce a flip print, so a line number resolving to several
    files here means several files genuinely have an instrumented branch on
    that line -- real ambiguity, not an artifact of scanning every DILocation
    in a five-TU merge.

    mode="all": every DILocation in the merged module.  Retained as a
    fallback and for comparison; it over-generates candidates badly (LULESH:
    528 of 1965 lines look multi-file this way).

    Returns (line_map, n_branch_sites).  On any failure returns ({}, 0) and
    the caller falls back to bare line numbers."""
    target = opt_bc if mode == "branches" else merged_bc
    ll = target.with_suffix(".ll")
    rc, _ = sh(["llvm-dis", str(target), "-o", str(ll)], env=env, log=log)
    if rc != 0 or not ll.exists():
        return {}, 0, 0

    parsed = _parse_debug_info(ll)
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
        # The hook name differs from what BRANCH_CALL_RE expects; rather than
        # silently produce an empty map, fall back and say so.
        print("  (no EFTSan branch hooks matched in the IR -- falling back to "
              "every DILocation; check BRANCH_CALL_RE against the pass)")
        return build_line_map(merged_bc, opt_bc, env, mode="all", log=log)

    return {k: sorted(v) for k, v in out.items()}, n_calls, len(branch_dbg)


# --------------------------------------------------------------------
def write_site_universe(line_map, outdir, name):
    """Dump every instrumented branch site as file:line.

    This is the static universe the tool could possibly report -- the set
    brtrace's "instrumented" count refers to.  Matching COUNTS (566 = 566)
    does not prove the SETS match, and the difference matters: a census site
    absent from this file is a coverage gap, while a census site present here
    and unreported is a genuine false negative.  Adjudication needs to tell
    those apart."""
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


VECTOR_TYPE_RE = re.compile(r"<\d+ x (?:float|double|half)>")


def diagnose_vector_types(merged_bc, env, limit=6):
    """Find vector-typed values in the merged module and the functions holding
    them.  EFTSan's pass aborts with "vectors not supported for now!".

    The usual cause is NOT vectorization -- -O0 emits none -- but SysV
    argument coercion: a small struct of floats passed or returned by value
    gets packed into <2 x float>, e.g. QuickSilver's 12-byte MC_Vector
    returned as { <2 x float>, float }.  The same struct in double precision
    is 24 bytes and travels as scalars, so only the fp32 build trips it.  The
    vector is register packing, not arithmetic: no fcmp lives inside it."""
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
    """A tool that cannot build is a RESULT, not a missing run.

    Without this the cell is simply absent from the output tree, which reads
    as "not attempted" when the truth is "attempted and the tool refused".
    Writes the same summary.txt / summary.json filenames as a successful run,
    with status set, so downstream table code finds a record either way."""
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
    lines += ["",
              "This is a tool limitation, not a missing experiment: the build",
              "was attempted and EFTSan refused it.  Report the cell as a",
              "documented failure rather than as an empty or zero entry.",
              ""]
    (outdir / "summary.txt").write_text("\n".join(lines))
    (outdir / "summary.json").write_text(json.dumps([rec], indent=2))
    return rec


def build(precision, outdir, std, link_override, keep_bc, env,
          lib_dirs, resolve_mode):
    src = BENCH_ROOT / f"lulesh_{precision}"
    dst = BUILD_ROOT / f"lulesh_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        print("         set BENCH_ROOT=/path/to/benchmarks/lulesh")
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
            print(f"    LLVM 10 clang++ with -std={std}. If the failure is a")
            print("    language-level one, try --std c++11 (LULESH's baseline).")
            return None
        bcs.append(bc)

    # ---- 2. merge BEFORE instrumentation ----------------------------
    merged = dst / "lulesh_merged.bc"
    print(f"  llvm-link {len(bcs)} modules -> {merged.name}")
    link_cmd = ["llvm-link"] + [str(b) for b in bcs]
    if link_override:
        link_cmd.append("-override")
    rc, out = sh(link_cmd + ["-o", str(merged)], cwd=dst, env=env, log=log)
    if rc != 0 or not merged.exists():
        print(f"  llvm-link FAILED -- see {log}")
        print("    C++ inline/template duplicates are the usual cause;")
        print("    retry with --link-override.")
        for line in out.splitlines()[:4]:
            print("   ", line[:150])
        return None

    # ---- 3. instrument ----------------------------------------------
    opt_bc = dst / "lulesh.opt.bc"
    print("  opt -eftsan on the merged module")
    rc, out = sh(["opt", "-load", str(pass_so), "-eftsan",
                  str(merged), "-o", str(opt_bc)], cwd=dst, env=env, log=log)
    if rc != 0 or not opt_bc.exists() or opt_bc.stat().st_size == 0:
        print(f"  INSTRUMENTATION FAILED -- see {log}")
        for l in [x for x in out.splitlines() if x.strip()][-6:]:
            print("     ", l[:150])
        if "vector" in out.lower():
            nvec, vfuncs = diagnose_vector_types(merged, env)
            print(f"    {nvec} vector-typed value(s): at -O0 this is SysV "
                  f"argument coercion (a small by-value float struct packed "
                  f"into <2 x float>), not vectorization.")
            for f in vfuncs:
                print(f"      {f}")
        if opt_bc.exists() and opt_bc.stat().st_size == 0:
            print("    0-byte output. If the OTHER precision instruments "
                  "fine, the pass is healthy and something in THIS module")
            print("    crashes it -- read the opt output above. If BOTH "
                  "precisions give 0 bytes, that is the Value::dump()")
            print("    symptom (Release LLVM has no dump()).")
        return None

    # ---- 4. link ----------------------------------------------------
    binary = dst / f"lulesh_{precision}.eftsan"
    print("  linking against the EFTSan runtime")
    link = ["clang++", "-O0", "-g", str(opt_bc),
            f"-L{EFT_HOME/'runtime/obj'}", "-leftsanitizer",
            "-lm", "-lmpfr", "-lgmp", "-lstdc++",
            f"-Wl,-rpath,{EFT_HOME/'runtime/obj'}"]
    for d in lib_dirs:                     # conda's MPFR/GMP
        link += [f"-L{d}", f"-Wl,-rpath,{d}"]
    link += ["-o", str(binary)]
    rc, out = sh(link, cwd=dst, env=env, log=log)
    if rc != 0 or not binary.exists():
        print(f"  LINK FAILED -- see {log}")
        # ld reports the referencing location on one line and the symbol on
        # the next; the location is useless without the name, so pull the
        # names out and dedupe -- 200 undefined refs are usually 2 symbols.
        syms = Counter(UNDEF_RE.findall(out))
        if syms:
            print(f"    {sum(syms.values())} undefined reference(s), "
                  f"{len(syms)} distinct symbol(s):")
            for sym, n in syms.most_common(8):
                print(f"      {n:6d}  {sym}")
            if any(s.startswith(("mpfr_", "__gmpfr_", "__gmp")) for s in syms):
                print("    -> MPFR/GMP: -lmpfr -lgmp fell off the link line.")
            elif any(s.startswith(("MPI_", "hypre_MPI_")) for s in syms):
                print("    -> MPI: the sequential stubs (utilities/mpistubs.c) "
                      "were not compiled in, or HYPRE_SEQUENTIAL is not set "
                      "for every TU.")
            elif any(s.startswith("eftsan") for s in syms):
                print("    -> EFTSan runtime: -leftsanitizer did not resolve.")
            else:
                print("    -> a defining TU is missing from the merge. Find it:")
                print(f"         grep -rln '{syms.most_common(1)[0][0]}' "
                      f"<benchmark tree> --include='*.c'")
        else:
            for line in out.splitlines():
                if ERROR_LINE_RE.search(line):
                    print("   ", line[:160])
                    break
        return None

    # ---- gates ------------------------------------------------------
    _, nm_out = sh(["nm", str(binary)], env=env)
    nsym = sum(1 for l in nm_out.splitlines() if "eftsan" in l.lower())
    print(f"  eftsan symbol count = {nsym}")

    print(f"  resolving line numbers from debug info (mode={resolve_mode})")
    line_map, n_branch, n_locs = build_line_map(merged, opt_bc, env,
                                        mode=resolve_mode, log=log)
    collisions = sum(1 for v in line_map.values() if len(v) > 1)
    print(f"  instrumented branch hooks = {n_branch} "
          f"({n_locs} distinct debug locations)")
    print(f"  lines mapped = {len(line_map)} "
          f"({collisions} occur in more than one file)")
    universe = write_site_universe(line_map, outdir, "instrumented_sites.csv")
    print(f"  site universe -> {universe.name}")

    (outdir / "build_info.txt").write_text(
        f"benchmark    = LULESH\n"
        f"precision    = {precision}\n"
        f"opt          = -O0  (enforced; EFTSan does not survive -O1+)\n"
        f"compiler     = clang++ (LLVM 10 via {SETUP_SH.name}), -std={std}\n"
        f"TUs compiled = {len(bcs)}  ({' '.join(SRCS)})\n"
        f"merge        = llvm-link BEFORE opt -eftsan (arg-shadow ordering)\n"
        f"pass         = {pass_so} -eftsan\n"
        f"runtime      = {runtime}\n"
        f"cflags       = {' '.join(cflags)}\n"
        f"link libs    = -leftsanitizer -lm -lmpfr -lgmp -lstdc++\n"
        f"eftsan_syms  = {nsym}\n"
        f"lines_mapped = {len(line_map)} ({collisions} multi-file), "
        f"mode={resolve_mode}\n"
        f"branch_hooks = {n_branch}  (eftsan_check_branch call sites)\n"
        f"debug_locs   = {n_locs}  (distinct !dbg; several hooks may share "
        f"one source location)\n"
        f"openmp       = disabled (no -fopenmp)\n"
        f"ERRORTHRESHOLD is a compile-time constant of the runtime (45);\n"
        f"there is no eta equivalent and no sweep.\n"
        f"Adjudicate against the -O0 census, NOT the -O2 one.\n")

    # Preflight the loader.  A binary that links cleanly can still fail to
    # start if the run environment lacks LD_LIBRARY_PATH -- that shows up as
    # exit 127 with empty logs, which reads like a crash but is not one.
    _, ldd_out = sh(["ldd", str(binary)], env=env)
    unfound = [l.strip() for l in ldd_out.splitlines() if "not found" in l]
    if unfound:
        print("  *** LOADER GATE FAILED -- the binary cannot start:")
        for l in unfound[:4]:
            print("     ", l[:120])
        print("      This is exit 127, not a crash. Check that "
              f"{SETUP_SH.name} exports LD_LIBRARY_PATH for the runtime,")
        print("      MPFR and GMP; the -Wl,-rpath only covers the runtime.")
        return None
    print(f"  loader gate: all shared libraries resolve")

    if nsym == 0:
        print("  *** UNINSTRUMENTED BINARY -- the pass did not take. "
              "Aborting this precision.")
        return None

    if not keep_bc:
        for b in bcs:
            b.unlink(missing_ok=True)

    # Everything shipped with the benchmark counts as "in-TU"; anything else
    # a resolved site names is a system/STL header.  This is the EFTSan
    # equivalent of the FPChecker harness's demangle+std:: test, but done on
    # the resolved filename, which is stronger evidence than a symbol name.
    local_files = {p.name for p in dst.iterdir()
                   if p.suffix in (".cc", ".h", ".hpp", ".hh")}

    return binary, line_map, local_files, n_branch


# --------------------------------------------------------------------
def run(binary, precision, size, iters, outdir, line_map, local_files,
        env, n_branch):
    """env MUST be the setup_eftsan.sh environment, not os.environ.  The
    runtime and MPFR are found via LD_LIBRARY_PATH set by that script; running
    with the ambient environment gives exit 127 and an empty log -- the loader
    fails before main()."""
    print(f"  running -s {size} -i {iters}")
    rc, out, err = sh_split(["stdbuf", "-i0", "-o0", "-e0", str(binary),
                             "-s", str(size), "-i", str(iters), "-p"],
                            cwd=binary.parent, env=env)
    stdout_log = outdir / "run_O0.stdout"
    stderr_log = outdir / "run_O0.stderr"
    stdout_log.write_text(out)
    stderr_log.write_text(err)

    lines = Counter()
    for stream in (err, out):
        for m in FLIP_RE.finditer(stream):
            lines[(int(m.group(1)), int(m.group(2)))] += 1
    parsed = sum(lines.values())

    tm = TOTAL_RE.search(err) or TOTAL_RE.search(out)
    runtime_total = int(tm.group(1)) if tm else None
    # The FPChecker harness's UNPARSED count has the same job: flips the
    # runtime emitted that the parser did not turn into a site.
    unparsed = (runtime_total - parsed) if runtime_total is not None else 0

    sites, foreign, ambiguous, unresolved = Counter(), Counter(), [], []
    for (fhash, ln), n in lines.most_common():
        cands = [f for f in line_map.get(ln, []) if fnv1a(f) == fhash]
        if not cands:
            cands = line_map.get(ln, [])
        if len(cands) == 1:
            key = f"{cands[0]}:{ln}"
            sites[key] = n
            if cands[0] not in local_files:
                foreign[key] = n
        elif len(cands) > 1:
            key = f"AMBIGUOUS:{ln}  [{' | '.join(cands)}]"
            sites[key] = n
            ambiguous.append(ln)
        else:
            sites[f"line:{ln}"] = n
            unresolved.append(ln)

    if rc != 0:
        why = {127: "loader failure -- a shared library was not found; the "
                    "run environment is missing LD_LIBRARY_PATH",
               139: "segfault -- check the ARITY of the function in the "
                    "backtrace before suspecting the build",
               134: "abort -- usually an MPFR assertion in the shadow"}
        print(f"  *** non-zero exit ({rc}): "
              f"{why.get(rc, 'see the stderr log')}")
        for line in (err or out).splitlines()[:3]:
            if line.strip():
                print("     ", line[:150])
        if not (err or out).strip():
            print("      (both streams empty -- the process died before "
                  "producing output)")

    return {
        "benchmark": "lulesh",
        "precision": precision,
        "opt": "O0",
        "threshold": 45,          # ERRORTHRESHOLD, compiled into the runtime
        "branch_hooks": n_branch,  # static instrumented FP comparisons =
                                   # the TN denominator, comparable to
                                   # brtrace's "instrumented" count
        "size": size,
        "iterations": iters,
        "flips": parsed,
        "runtime_total": runtime_total,
        "locations": len(sites),
        "sites": dict(sites.most_common()),
        "foreign_sites": dict(foreign),
        "foreign_flips": sum(foreign.values()),
        "ambiguous_lines": sorted(ambiguous),
        "ambiguous_flips": sum(n for k, n in sites.items()
                               if k.startswith("AMBIGUOUS:")),
        "unresolved_lines": sorted(unresolved),
        "unresolved_flips": sum(n for (_fh, ln), n in lines.items()
                                if ln in unresolved),
        "unparsed_lines": unparsed,
        "exit_code": rc,
        "log": str(stderr_log.relative_to(HERE)),
        "stdout_log": str(stdout_log.relative_to(HERE)),
    }


def write_results(precision, records, outdir):
    """Same report shape as run_lulesh_fpchecker.py: header, one totals line
    per run, a NOTE block, then a per-run site table.  EFTSan has no eta, so
    there is exactly one run per precision -- but the list-of-records
    structure is kept so downstream readers can treat both tools' summary.json
    identically."""
    r0 = records[0]
    lines = [f"LULESH {precision} -- EFTSanitizer branch flips",
             f"(-{r0['opt']} compile+link, merged module, serial build, "
             f"s={r0['size']}, i={r0['iterations']})", ""]
    for r in records:
        own = (r["flips"] - r["foreign_flips"] - r["ambiguous_flips"]
               - r["unresolved_flips"])
        lines.append(f"{r['flips']} flips @ {r['locations']} loc"
                     f"   (ERRORTHRESHOLD={r['threshold']}, no sweep)"
                     f"   [in-TU {own}, STL {r['foreign_flips']}, "
                     f"ambiguous {r['ambiguous_flips']}, "
                     f"unresolved {r['unresolved_flips']}]")
    lines += [
        "",
        f"{r0['branch_hooks']} static instrumented FP comparisons in the "
        f"module (the TN denominator).",
        "",
        "NOTE: EFTSan prints a bare line number with no filename.  Files above",
        "      are recovered from the INSTRUMENTED module: each site is a line",
        "      carrying an eftsan_check_branch hook, resolved to its file via",
        "      the debug-info scope chain (-O0, so no inlining).  Only lines",
        "      that actually carry a hook are candidates, which is why 263",
        "      resolves to stl_algobase.h (std::max) and not to the unrelated",
        "      line 263 in lulesh.cc.  AMBIGUOUS, if it appears, means two",
        "      files genuinely both have an instrumented branch on that line.",
        "      Ground truth is the -O0 census, NOT the -O2 one.",
        "",
    ]
    for r in records:
        lines.append(f"--- {r['opt']} : {r['flips']} flips @ "
                     f"{r['locations']} loc ---")
        if not r["sites"]:
            lines.append("    (no flips -- check the log; if the run produced "
                         "output, the flip-line pattern may differ)")
        for site, n in r["sites"].items():
            lines.append(f"    {n:9d}  {site}")
        if r["runtime_total"] is not None:
            lines.append(f"    runtime total: {r['runtime_total']}")
        if r["ambiguous_lines"]:
            lines.append(f"    AMBIGUOUS lines: {r['ambiguous_lines']}")
        if r["unresolved_lines"]:
            lines.append(f"    UNRESOLVED (no debug info): "
                         f"{r['unresolved_lines']}")
        if r["unparsed_lines"]:
            lines.append(f"    UNPARSED flip lines: {r['unparsed_lines']}")
        lines.append(f"    exit code: {r['exit_code']}")
        lines.append("")
    (outdir / "summary.txt").write_text("\n".join(lines))
    (outdir / "summary.json").write_text(json.dumps(records, indent=2))

    # Extra, not in the FPChecker harness: location,flips in the same shape as
    # the brtrace census summaries, so the two sides diff directly.
    with open(outdir / f"lulesh_{precision}_eftsan_summary.csv", "w") as fh:
        fh.write("location,flips\n")
        for site, n in records[0]["sites"].items():
            fh.write(f"{site.split('  [')[0]},{n}\n")


# --------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("-s", "--size", type=int, default=5)
    ap.add_argument("-i", "--iter", type=int, default=20)
    ap.add_argument("--std", default="c++11",
                    help="LULESH baseline is c++11; the EFTSan toolchain is "
                         "LLVM 10, so newer standards are a gamble")
    ap.add_argument("--link-override", action="store_true",
                    help="pass -override to llvm-link (duplicate C++ "
                         "inline/template definitions)")
    ap.add_argument("--resolve", default="branches",
                    choices=["branches", "all"],
                    help="how to map EFTSan's bare line numbers to files. "
                         "branches (default): only lines carrying an EFTSan "
                         "branch hook in the instrumented IR -- exact. "
                         "all: every DILocation in the merged module -- "
                         "over-generates candidates.")
    ap.add_argument("--keep-bc", action="store_true",
                    help="keep the per-TU .bc files")
    ap.add_argument("--no-run", action="store_true", help="build and gate only")
    args = ap.parse_args()

    env = eftsan_env()
    if env is None:
        return 1

    lib_dirs, missing_libs = find_lib_dirs(env)
    if missing_libs:
        print(f"FATAL: cannot locate {', '.join(missing_libs)} -- EFTSan's "
              f"shadow needs MPFR and GMP.")
        print("       They normally come from the conda env; check that "
              "eftsan_env is active.")
        return 1
    env = with_lib_path(env, lib_dirs)

    missing = check_tools(env)
    if missing:
        print(f"FATAL: not on PATH after sourcing {SETUP_SH.name}: "
              f"{', '.join(missing)}")
        return 1

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)

    print(f"LULESH / EFTSanitizer   s={args.size} i={args.iter}   "
          f"-O0 (enforced)")
    print(f"  precisions: {' '.join(args.precision)}")
    print(f"  EFT_HOME:   {EFT_HOME}")
    print(f"  bench:      {BENCH_ROOT}")
    print(f"  workdir:    {WORK_ROOT}")
    print(f"  libs:       {' '.join(str(d) for d in lib_dirs)}")
    print("  no eta sweep: ERRORTHRESHOLD is compiled into the runtime (45)\n")

    overall = {}
    for precision in args.precision:
        print(f"===== {precision} =====")
        outdir = RESULT_ROOT / precision
        outdir.mkdir(parents=True, exist_ok=True)

        built = build(precision, outdir, args.std, args.link_override,
                      args.keep_bc, env, lib_dirs, args.resolve)
        if built is None:
            continue
        binary, line_map, local_files, n_branch = built
        if args.no_run:
            print(f"  built: {binary}\n")
            continue

        # One record per run.  FPChecker has three (one per eta); EFTSan has
        # one, because its threshold is compiled into the runtime.
        records = [run(binary, precision, args.size, args.iter, outdir,
                       line_map, local_files, env, n_branch)]
        write_results(precision, records, outdir)
        overall[precision] = records

        for r in records:
            own = (r["flips"] - r["foreign_flips"] - r["ambiguous_flips"]
                   - r["unresolved_flips"])
            print(f"  -> {r['flips']} flips @ {r['locations']} loc "
                  f"(runtime total {r['runtime_total']})  "
                  f"[in-TU {own}, STL {r['foreign_flips']}, "
                  f"ambiguous {r['ambiguous_flips']}, "
                  f"unresolved {r['unresolved_flips']}]")
        print()

    if args.no_run:
        return 0
    if not overall:
        print("No results produced.")
        return 1

    print("===== results =====")
    for precision, records in overall.items():
        for r in records:
            amb = (f"   [{len(r['ambiguous_lines'])} ambiguous]"
                   if r["ambiguous_lines"] else "")
            print(f"  {precision:5s} {r['flips']:9d} flips @ "
                  f"{r['locations']:3d} loc   exit={r['exit_code']}{amb}")
    print(f"\n{BENCH_NAME}/results/O0/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())