#!/usr/bin/env python3
"""
run_amg_eftsan.py

Build AMG under EFTSanitizer branch-flip instrumentation and run it, keeping
fp32 and fp64 results completely separate.  Counterpart to
run_amg_fpchecker.py; same structure as run_lulesh_eftsan.py.

Place in: branch_flip/experiments/eftsan_experiments/

    ./run_amg_eftsan.py                     # both precisions
    ./run_amg_eftsan.py -p fp32             # fp32 only
    ./run_amg_eftsan.py -n 5 --problem 2    # the census config (default)
    ./run_amg_eftsan.py --no-run            # build and gate only

Layout produced:
    amg/
      results/O0/  fp32/  build.log  run_O0.stdout  run_O0.stderr
                          summary.txt  summary.json  build_info.txt
                          amg_fp32_eftsan_summary.csv
                   fp64/  ...
      build/O0/    amg_fp32/  amg_fp64/

CENSUS CONFIG AND GROUND TRUTH (brtrace, -O0, problem 2, n=5)

  fp32 vs fp64:  13.76% coverage.  ONE flip, at gmres.c:573 in
                 hypre_GMRESSolve -- the convergence test -- first at event
                 14,665, with structural divergence 52 events later at
                 14,717.  TP=1, TN=76, executed=77 sites, DEAD=489,
                 instrumented=566.  So GT = 1 site.

  fp64 vs ld:    100% coverage.  ZERO flips over 96,372 events.  executed=82,
                 DEAD=484.  fp64 is oracle-stable for AMG (unlike LULESH,
                 where fp64-vs-ld has 4 real flip sites).

  The fp64 GT=0 cell has a REAL TN denominator here -- 82 executed sites,
  96,372 adjudicated events -- so report a false-alarm rate (FP per 100
  executed sites, FP per million adjudicated events) rather than the
  P=0.00/R=1.00 convention, which is misleading when it can be avoided.

  Coverage matters for scoring: adjudication STOPS at the first structural
  divergence, so post-divergence events are UNADJUDICATED, not TN.  With
  13.76% coverage the fp32 FP count is an UPPER bound and the FN count a
  LOWER bound.  Put the coverage figure in the caption.

  Problem 1 (AMG-PCG on a 27-point Laplace) shows ZERO fp32/fp64 iteration
  divergence at any size tested -- well-conditioned and AMG-preconditioned.
  Problem 2 is GMRES on a modified-diagonal system; Gram-Schmidt is precision
  sensitive and fp32 needs roughly twice the iterations (n=5: 297 vs 150).
  Use problem 2.

WHY THIS IS NOT THE FPCHECKER HARNESS WITH A DIFFERENT COMPILER

  * No eta sweep.  EFTSan has no runtime knob.  Its sensitivity is
    ERRORTHRESHOLD, a COMPILE-TIME constant in $EFT_HOME/runtime/handleReal.h,
    and handleReal.h is not tracked as a make dependency -- `rm -rf obj`
    before rebuilding or the edit silently does nothing.  Keep it at 45.

  * -O0 only, enforced rather than defaulted.  Adjudicate against the -O0
    census.  There is deliberately no --opt flag.

  * The AMG Makefiles are not used at all.  EFTSan needs per-TU bitcode, a
    merge, then the pass, then a link; a Makefile that goes .c -> .o -> .a
    cannot express that.  Every .c in the tree is compiled by hand here, and
    the per-directory -I flags the sub-makes would have supplied by cd-ing
    are reconstructed as an explicit -I list.

  * AMG is C, so the frontend is clang, not clang++, and there is no name
    mangling anywhere in the pipeline.

BUILD PIPELINE (the ordering is load-bearing)

    1. clang -O0 -g -emit-llvm -c  (every .c)  -> one .bc per TU
    2. llvm-link all of them                   -> amg_merged.bc
    3. opt -load libEFTSanitizer.so -eftsan    -> amg.opt.bc
    4. clang -O0 amg.opt.bc -leftsanitizer     -> amg_<prec>.eftsan

  Step 2 MUST precede step 3.  Instrumenting TUs separately and linking the
  objects corrupts function-argument shadow bookkeeping at call sites -- the
  callee prologue's eftsan_get_arg sequence reads garbage and the program
  dies in the prologue even though the IR signatures match.  AMG has ~100
  TUs, so this is the largest merge in the suite.

  Step 3 is legacy pass manager syntax and needs an explicit -load.

  Step 4 needs -lm -lmpfr -lgmp -lstdc++ plus the runtime as -leftsanitizer.
  MPFR and GMP come from the conda env, not from $EFT_HOME/runtime/obj, and
  setup_eftsan.sh does not put them on LD_LIBRARY_PATH; this script locates
  them and both rpaths and exports them.  Without that the binary links
  cleanly and then dies at exec with exit 127.

VERIFICATION GATES (a clean build proves nothing -- AMG fails SILENTLY)

  1. eftsan symbol count -- zero means the pass did not run.
  2. sizeof(HYPRE_Real) probe.  A clean build does NOT prove -DHYPRE_SINGLE
     reached every TU; the failure is silent.  fp32 must be 4, fp64 8.
  3. Loader preflight (ldd) before the run.
  4. Iterations != 0.  Upstream's sequential MPI stubs have no FLOAT or
     LONG_DOUBLE case and no default, so at non-double precision Allreduce
     falls through, never writes recvbuf, and returns success.  Symptom:
     "Iterations = 0, Residual = 0.000000e+00, FOM = -nan" from a binary that
     built and ran cleanly.  The benchmark trees are patched; this gate
     catches a regression or an unpatched tree.
  5. instrumented branch hooks vs brtrace's instrumented=566.  On LULESH the
     two frontends agreed exactly (75 = 75); a large disagreement here means
     the two tools are not seeing the same static universe and the site sets
     are not directly comparable.

OUTPUT KEYING

  EFTSan's flip print emits a BARE LINE NUMBER, no filename:
      branch flip @ line 573
  Across ~100 TUs a bare line number is nearly meaningless -- line 573 exists
  in dozens of files.  This harness reads the INSTRUMENTED module, finds every
  call to an eftsan_check_branch hook, and resolves that call's !dbg to
  file:line via the debug-info scope chain (-O0, so no inlining).  Only lines
  that actually carry a hook are candidates.  A line still resolving to more
  than one file is flagged AMBIGUOUS -- with this many TUs, expect some.

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
EFT_HOME = Path(os.environ.get("EFT_HOME", "/usr/workspace/das9/EFTSanitizer"))
SETUP_SH = Path(os.environ.get("EFT_SETUP", EFT_HOME / "setup_eftsan.sh"))

HERE = Path(__file__).resolve().parent
# .../branch_flip/experiments/eftsan_experiments -> .../branch_flip/benchmarks
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT", HERE.parent.parent / "benchmarks" / "amg"))

BENCH_NAME = "amg"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build" / "O0"
RESULT_ROOT = WORK_ROOT / "results" / "O0"

PRECISION_FLAG = {"fp32": "-DHYPRE_SINGLE", "fp64": ""}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8}

# Directories that hold no compilable benchmark source.  test/ IS included --
# it holds amg.c, which has main().
SKIP_DIRS = {"docs", "CMakeFiles", "build", ".git", "obj"}

# AMG ships source files its own Makefiles never build -- old_gmres.c and
# par_difconv.c among them.  par_difconv.c calls hypre_map(), which nothing in
# the tree defines, so globbing *.c and linking the result fails with exactly
# five undefined references.  Including dead source is not merely untidy: those
# TUs are absent from the reference binary, so instrumenting them would add
# branch sites the brtrace census never saw and inflate the hook count.
#
# So the TU list is taken from the Makefiles, not from a glob.  Each library
# directory's Makefile names its objects; that list IS the build.
EXCLUDE_FILES = set()          # only for --exclude on top of the Makefiles
SRC_RE = re.compile(r"\b([A-Za-z0-9_+-][A-Za-z0-9_.+-]*)\.(?:c|o)\b")

# brtrace's static universe for AMG at -O0, for cross-checking the pass.
BRTRACE_INSTRUMENTED = 566

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
ITER_RE = re.compile(r"Iterations\s*=\s*(\d+)")
RESID_RE = re.compile(r"Final Relative Residual Norm\s*=\s*([0-9.eE+-]+)")


def sh(cmd, cwd=None, env=None, log=None):
    p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, text=True, errors="replace")
    if log:
        with open(log, "a") as fh:
            fh.write("$ " + " ".join(map(str, cmd)) + "\n")
            fh.write(p.stdout + "\n")
    return p.returncode, p.stdout


def sh_split(cmd, cwd=None, env=None):
    """Keeps stdout and stderr apart -- EFTSan's flip prints go to stderr and
    AMG's solver report to stdout."""
    p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE, text=True, errors="replace")
    return p.returncode, p.stdout, p.stderr


def eftsan_env():
    """setup_eftsan.sh puts LLVM 10's clang/opt/llvm-link on PATH and sets
    EFT_HOME / LLVM_PASS_LIB.  Python cannot source a shell script, so run it
    under bash and harvest the resulting environment wholesale."""
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
# MPFR and GMP are EFTSan's shadow arithmetic but live in the conda env, not
# in $EFT_HOME/runtime/obj.  setup_eftsan.sh stops at the runtime, so a
# cleanly linked binary still dies at exec with exit 127 and
# "error while loading shared libraries: libmpfr.so.6".
# --------------------------------------------------------------------
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
    for t in ("clang", "opt", "llvm-link", "llvm-dis", "llvm-nm"):
        rc, _ = sh([t, "--version"], env=env)
        if rc != 0:
            missing.append(t)
    return missing


# --------------------------------------------------------------------
# line -> file resolution, restricted to instrumented branch sites.
# DILocation -> scope -> (DISubprogram | DILexicalBlock | DILexicalBlockFile)
# -> DIFile.  At -O0 there is no inlining, so the chain is honest.
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


def _parse_debug_info(ll_path):
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


def build_line_map(merged_bc, opt_bc, env, mode="branches", log=None,
                   keep_ll=False):
    """Returns (line_map, n_branch_sites).

    mode="branches" (default): only lines carrying an EFTSan branch hook in
    the INSTRUMENTED module.  Those are exactly the sites that can produce a
    flip print, so a multi-file line here is real ambiguity, not an artifact
    of scanning every DILocation across ~100 merged TUs.

    mode="all": every DILocation in the merged module.  Over-generates badly
    at AMG's TU count; kept as a fallback."""
    target = opt_bc if mode == "branches" else merged_bc
    ll = target.with_suffix(".ll")
    rc, _ = sh(["llvm-dis", str(target), "-o", str(ll)], env=env, log=log)
    if rc != 0 or not ll.exists():
        return {}, 0, 0

    parsed = _parse_debug_info(ll)
    if not keep_ll:
        # AMG's .ll is large; the .bc is the artifact worth keeping.
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
        print("  (no EFTSan branch hooks matched in the IR -- falling back to "
              "every DILocation; check BRANCH_CALL_RE against the pass)")
        return build_line_map(merged_bc, opt_bc, env, mode="all", log=log,
                              keep_ll=keep_ll)  # returns the 3-tuple

    return {k: sorted(v) for k, v in out.items()}, n_calls, len(branch_dbg)


# --------------------------------------------------------------------
# --------------------------------------------------------------------
# Archive semantics.
#
# AMG's real build produces .a archives and links test/amg against them, so
# the linker extracts only the members something actually references.  A TU
# with a dangling reference that nothing pulls in never reaches the binary --
# par_difconv.c calls hypre_map(), which no file in the tree defines, and the
# real build links fine because par_difconv.o is never extracted.
#
# A whole-module merge has no such escape: every TU is present, so the
# dangling reference becomes a link error.  Rather than hand-maintain a list
# of dead files, reproduce the archive behaviour -- drop exactly the TUs whose
# undefined symbols nothing in the merge defines, then relink.
# --------------------------------------------------------------------
def tu_symbols(bc, env):
    """(defined, undefined) symbol sets for one bitcode module."""
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
    """Drop TUs referencing a symbol no TU in the merge defines.

    `missing` comes from the linker, so libc / MPFR / runtime symbols never
    reach here -- only genuinely unresolvable names do.  A TU defining main()
    is never dropped: if main is what needs the symbol, the problem is real
    and the caller must fail rather than paper over it.

    Returns (kept, dropped, blocked)."""
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
    """Source stems a Makefile names, as a set like {"gmres", "pcg"}.

    hypre's Makefiles list SOURCES and derive objects by substitution:

        FILES =\\
         gmres.c\\
         pcg.c
        OBJS = ${FILES:.c=.o}

    so scanning for ".o" alone finds only the substitution itself and comes up
    empty.  Accept both .c and .o tokens.  Recipe lines (tab-indented) and
    pattern rules (%.o: %.c) are skipped so the rule template is not mistaken
    for a file list; the ":.c=.o" idiom yields no token because the characters
    before the extension are punctuation."""
    try:
        text = mkfile.read_text(errors="replace")
    except OSError:
        return set()
    text = re.sub(r"\\\s*\n", " ", text)      # join backslash continuations
    stems = set()
    for line in text.splitlines():
        line = line.split("#", 1)[0]
        if ".c" not in line and ".o" not in line:
            continue
        if line.startswith("\t") or "%" in line:
            continue
        for stem in SRC_RE.findall(line):
            stems.add(stem)
    return stems


def collect_sources(tree, extra_exclude=()):
    """The TU list AMG's own Makefiles build, per directory.

    A directory with a Makefile contributes exactly the .c files whose .o it
    names.  A directory without one falls back to every .c it contains, and
    says so.  This keeps the instrumented binary made of the same TUs as the
    reference build -- which is the whole point of matched-build discipline.

    Returns (sources, dropped, fallback_dirs)."""
    excluded = set(extra_exclude) | set(EXCLUDE_FILES)
    srcs, dropped, fallback = [], [], []

    for d in sorted(p for p in tree.iterdir()
                    if p.is_dir() and p.name not in SKIP_DIRS):
        present = sorted(d.glob("*.c"))
        if not present:
            continue
        mk = next((d / n for n in ("Makefile", "makefile", "GNUmakefile")
                   if (d / n).exists()), None)
        if mk is None:
            fallback.append(d.name)
            wanted = present
        else:
            stems = makefile_objects(mk)
            wanted = [c for c in present if c.stem in stems]
            dropped += [str(c.relative_to(tree)) for c in present
                        if c.stem not in stems]
            if not wanted:
                # A Makefile that names no objects from this directory means
                # the parse missed something; do not silently drop the dir.
                fallback.append(d.name)
                wanted = present
                dropped = [x for x in dropped
                           if not x.startswith(d.name + "/")]
        for c in wanted:
            if c.name in excluded:
                dropped.append(str(c.relative_to(tree)))
            else:
                srcs.append(c)

    # .c files sitting at the tree root, if any
    for c in sorted(tree.glob("*.c")):
        if c.name not in excluded:
            srcs.append(c)

    return srcs, sorted(dropped), fallback


def include_flags(tree):
    """The sub-makes supply per-directory -I by cd-ing into each directory.
    Compiling by hand, reconstruct that as an explicit list: the tree root
    plus every source directory."""
    incs = [f"-I{tree}"]
    for d in sorted(p for p in tree.iterdir()
                    if p.is_dir() and p.name not in SKIP_DIRS):
        incs.append(f"-I{d}")
    return incs


def check_width(tree, precision, env, outdir):
    """A clean AMG build does NOT prove -DHYPRE_SINGLE reached the headers.
    The failure is silent, so probe sizeof(HYPRE_Real) directly."""
    probe = outdir / "_width_probe.c"
    probe.write_text('#include <stdio.h>\n#include "HYPRE_utilities.h"\n'
                     'int main(void){printf("%zu\\n", sizeof(HYPRE_Real));'
                     ' return 0;}\n')
    out_bin = outdir / "_width_probe"
    cmd = ["clang", "-O0", "-w", "-DHYPRE_SEQUENTIAL=1"]
    if PRECISION_FLAG[precision]:
        cmd.append(PRECISION_FLAG[precision])
    cmd += [f"-I{tree}", f"-I{tree/'utilities'}", str(probe),
            "-o", str(out_bin)]
    rc, _ = sh(cmd, env=env)
    if rc != 0:
        return None, False
    rc, out = sh([str(out_bin)], env=env)
    try:
        w = int(out.strip())
    except ValueError:
        return None, False
    return w, w == EXPECTED_WIDTH[precision]


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


def build(precision, outdir, link_override, keep_bc, env, lib_dirs,
          resolve_mode, keep_ll, extra_exclude):
    src = BENCH_ROOT / f"amg_{precision}"
    dst = BUILD_ROOT / f"amg_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        print("         set BENCH_ROOT=/path/to/benchmarks/amg")
        return None
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    for pat in ("*.o", "*/*.o", "*.a", "*/*.a", "*.bc", "*/*.bc", "test/amg"):
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
        print(f"  {len(dropped)} file(s) present but not built by AMG's "
              f"Makefiles (skipped): {', '.join(dropped)}")
    if fallback:
        print(f"  *** no parseable Makefile in {', '.join(fallback)} -- "
              f"compiling every .c there; verify against the real build")
    if not srcs:
        print(f"  FATAL: no .c sources found under {dst}")
        return None

    log = outdir / "build.log"
    if log.exists():
        log.unlink()

    cflags = ["-std=c11", "-O0", "-g", "-w", "-DHYPRE_SEQUENTIAL=1",
              "-fno-vectorize", "-fno-slp-vectorize"]
    if PRECISION_FLAG[precision]:
        cflags.append(PRECISION_FLAG[precision])
    cflags += include_flags(dst)

    # ---- 1. per-TU bitcode ------------------------------------------
    print(f"  compiling {len(srcs)} TUs to bitcode (-O0)")
    bcs, failed = [], []
    for s in srcs:
        bc = s.with_suffix(".bc")
        rc, out = sh(["clang"] + cflags +
                     ["-emit-llvm", "-c", str(s), "-o", str(bc)],
                     cwd=dst, env=env, log=log)
        if rc != 0 or not bc.exists():
            failed.append((s.relative_to(dst), out))
            continue
        bcs.append(bc)
    if failed:
        print(f"  COMPILE FAILED on {len(failed)}/{len(srcs)} TUs "
              f"-- see {log}")
        for f, out in failed[:3]:
            first = next((l for l in out.splitlines()
                          if ERROR_LINE_RE.search(l)), "")
            first = first.split("error:", 1)[-1].strip() or first
            print(f"    {f}: {first[:120]}")
        print("    A missing header usually means include_flags() did not "
              "pick up a source directory; check SKIP_DIRS.")
        return None
    print(f"  compiled {len(bcs)} TUs")

    # ---- 2-4. merge, instrument, link -------------------------------
    # Wrapped in a retry: a link failure caused purely by TUs the reference
    # build never extracted from its archives is recoverable by dropping
    # those TUs (see prune_dangling).  Anything else is a real failure.
    merged = dst / "amg_merged.bc"
    opt_bc = dst / "amg.opt.bc"
    binary = dst / f"amg_{precision}.eftsan"
    pruned_total = []

    def attempt(modules):
        """Returns (ok, undefined_symbols, message)."""
        print(f"  llvm-link {len(modules)} modules -> {merged.name}")
        link_cmd = ["llvm-link"] + [str(b) for b in modules]
        if link_override:
            link_cmd.append("-override")
        rc, out = sh(link_cmd + ["-o", str(merged)], cwd=dst, env=env, log=log)
        if rc != 0 or not merged.exists():
            return False, set(), ("llvm-link FAILED. Duplicate definitions "
                                  "across TUs are the usual cause; retry with "
                                  "--link-override.\n" +
                                  "\n".join(out.splitlines()[:4]))

        print("  opt -eftsan on the merged module")
        rc, out = sh(["opt", "-load", str(pass_so), "-eftsan",
                      str(merged), "-o", str(opt_bc)],
                     cwd=dst, env=env, log=log)
        if rc != 0 or not opt_bc.exists() or opt_bc.stat().st_size == 0:
            tail = [l for l in out.splitlines() if l.strip()][-6:]
            msg = "INSTRUMENTATION FAILED.  Last opt output:\n" + \
                  "\n".join("      " + l[:150] for l in tail)
            if "vector" in out.lower():
                nvec, vfuncs = diagnose_vector_types(merged, env)
                msg += (f"\n    {nvec} vector-typed value(s) in the module. "
                        f"At -O0 these are SysV argument coercion, not "
                        f"vectorization: a small by-value struct of floats "
                        f"packed into <2 x float>. The same struct in double "
                        f"precision is larger and travels as scalars, which "
                        f"is why only this precision fails. No fcmp lives "
                        f"inside them, so skipping them costs no coverage.")
                if vfuncs:
                    msg += "\n    First functions holding them:"
                    for f in vfuncs:
                        msg += f"\n      {f}"
            if opt_bc.exists() and opt_bc.stat().st_size == 0:
                msg += ("\n    0-byte output. If the OTHER precision "
                        "instruments fine, the pass itself is healthy and "
                        "something in THIS module crashes it -- read the opt "
                        "output above, not the pass source. If BOTH "
                        "precisions produce 0 bytes, it is the Value::dump() "
                        "symptom (a Release LLVM has no dump(); the pass must "
                        "use print(errs())).")
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
        if blocked:
            print("  *** the unresolvable symbol is referenced by the TU "
                  "defining main() -- this is a real missing definition, not "
                  "dead archive code.")
            break
        if not dead:
            break
        print(f"  {len(dead)} TU(s) reference symbols nothing in the merge "
              f"defines -- the reference build never extracted these from its "
              f"archives either. Dropping and relinking:")
        for b in dead:
            print(f"      {b.relative_to(dst)}  "
                  f"(unresolved: {', '.join(sorted(undef)[:3])})")
        pruned_total += [str(b.relative_to(dst)) for b in dead]
        modules = kept
        ok, undef, msg = attempt(modules)

    if not ok:
        if "vector" in msg.lower():
            nvec, vfuncs = diagnose_vector_types(merged, env)
            write_failure_record(
                precision, outdir, "opt -eftsan",
                "EFTSan cannot instrument this module: the pass calls exit(0) "
                "on any vector-typed load.",
                f"{nvec} vector-typed value(s) in the merged module.\n"
                f"At -O0 these are SysV argument coercion, not vectorization: "
                f"a small by-value struct of floats is packed into "
                f"<2 x float>.  The same struct in double precision is larger "
                f"and travels as scalars, so only this precision fails.\n"
                f"EFTSanitizer.cpp handleLoad() aborts before reaching its own "
                f"vector-handling branch, and exits with status 0, so opt "
                f"appears to succeed while writing an empty module.\n"
                f"No compiler flag avoids the coercion without changing FP "
                f"semantics (-mno-sse switches to x87 80-bit intermediates).\n"
                f"Functions holding vector values: "
                f"{', '.join(vfuncs) if vfuncs else '(none identified)'}",
                extra={"vector_values": nvec, "vector_functions": vfuncs})
            print(f"  NO RESULT for {precision}: EFTSan refuses this module "
                  f"(vector-typed loads). Recorded in {outdir.name}/"
                  f"summary.txt as a tool failure.")
            return None
        print(f"  BUILD FAILED -- see {log}")
        for line in msg.splitlines():
            print("   ", line[:160])
        if undef:
            syms = Counter(undef)
            print(f"    {len(syms)} distinct undefined symbol(s):")
            for sym, n in syms.most_common(8):
                print(f"      {sym}")
            if any(x.startswith(("mpfr_", "__gmpfr_", "__gmp")) for x in syms):
                print("    -> MPFR/GMP fell off the link line.")
            elif any(x.startswith(("MPI_", "hypre_MPI_")) for x in syms):
                print("    -> the sequential stubs (utilities/mpistubs.c) are "
                      "not in the merge.")
        return None
    bcs = modules

    # ---- gates ------------------------------------------------------
    _, nm_out = sh(["nm", str(binary)], env=env)
    nsym = sum(1 for l in nm_out.splitlines() if "eftsan" in l.lower())
    print(f"  eftsan symbol count = {nsym}")

    width, ok_w = check_width(dst, precision, env, outdir)
    print(f"  sizeof(HYPRE_Real) = {width} "
          f"({'ok' if ok_w else 'EXPECTED ' + str(EXPECTED_WIDTH[precision])})")

    print(f"  resolving line numbers from debug info (mode={resolve_mode})")
    line_map, n_branch, n_locs = build_line_map(merged, opt_bc, env, mode=resolve_mode,
                                        log=log, keep_ll=keep_ll)
    collisions = sum(1 for v in line_map.values() if len(v) > 1)
    print(f"  instrumented branch hooks = {n_branch} "
          f"({n_locs} distinct debug locations; brtrace = "
          f"{BRTRACE_INSTRUMENTED})")
    if n_branch and abs(n_branch - BRTRACE_INSTRUMENTED) > \
            0.1 * BRTRACE_INSTRUMENTED:
        print("    *** the two frontends disagree on the static universe by "
              ">10%; site sets may not be directly comparable")
    print(f"  lines mapped = {len(line_map)} "
          f"({collisions} occur in more than one file)")
    universe = write_site_universe(line_map, outdir, "instrumented_sites.csv")
    print(f"  site universe -> {universe.name}")

    (outdir / "build_info.txt").write_text(
        f"benchmark    = AMG\n"
        f"precision    = {precision}\n"
        f"opt          = -O0  (enforced; EFTSan does not survive -O1+)\n"
        f"compiler     = clang (LLVM 10 via {SETUP_SH.name}), C not C++\n"
        f"TUs compiled = {len(bcs)}\n"
        f"TUs skipped  = {len(dropped)} "
        f"({', '.join(map(str, dropped)) or 'none'})\n"
        f"TUs pruned   = {len(pruned_total)} "
        f"({', '.join(map(str, pruned_total)) or 'none'}); "
        f"dead archive members, "
        f"never extracted by the reference build\n"
        f"TU list from = per-directory Makefiles"
        f"{' (fallback glob: ' + ', '.join(fallback) + ')' if fallback else ''}\n"
        f"merge        = llvm-link BEFORE opt -eftsan (arg-shadow ordering)\n"
        f"pass         = {pass_so} -eftsan\n"
        f"runtime      = {runtime}\n"
        f"cflags       = {' '.join(c for c in cflags if not c.startswith('-I'))}\n"
        f"includes     = {sum(1 for c in cflags if c.startswith('-I'))} dirs\n"
        f"link libs    = -leftsanitizer -lm -lmpfr -lgmp -lstdc++\n"
        f"eftsan_syms  = {nsym}\n"
        f"HYPRE_Real   = {width} bytes "
        f"(expected {EXPECTED_WIDTH[precision]})\n"
        f"lines_mapped = {len(line_map)} ({collisions} multi-file), "
        f"mode={resolve_mode}\n"
        f"branch_hooks = {n_branch}  (eftsan_check_branch call sites; "
        f"brtrace: {BRTRACE_INSTRUMENTED})\n"
        f"debug_locs   = {n_locs}  (distinct !dbg; several hooks may share "
        f"one source location)\n"
        f"openmp       = disabled (no -fopenmp)\n"
        f"mpi          = HYPRE_SEQUENTIAL stubs\n"
        f"ERRORTHRESHOLD is a compile-time constant of the runtime (45);\n"
        f"there is no eta equivalent and no sweep.\n"
        f"Adjudicate against the -O0 census (fp32 GT=1 site @ gmres.c:573,\n"
        f"13.76% coverage; fp64 GT=0 at 100% coverage, 82 executed sites).\n")

    # Loader preflight: a cleanly linked binary can still fail to start.
    _, ldd_out = sh(["ldd", str(binary)], env=env)
    unfound = [l.strip() for l in ldd_out.splitlines() if "not found" in l]
    if unfound:
        print("  *** LOADER GATE FAILED -- the binary cannot start:")
        for l in unfound[:4]:
            print("     ", l[:120])
        return None
    print("  loader gate: all shared libraries resolve")

    if nsym == 0:
        print("  *** UNINSTRUMENTED BINARY -- the pass did not take. "
              "Aborting this precision.")
        return None
    if not ok_w:
        print("  *** WRONG HYPRE_Real WIDTH -- the precision flag did not "
              "reach the headers. Aborting this precision.")
        return None

    if not keep_bc:
        for b in bcs:
            b.unlink(missing_ok=True)

    local_files = {p.name for p in dst.rglob("*")
                   if p.suffix in (".c", ".h")}
    return binary, line_map, local_files, n_branch


# --------------------------------------------------------------------
def run(binary, precision, problem, n, outdir, line_map, local_files, env,
        n_branch):
    """env MUST be the setup_eftsan.sh environment (plus the MPFR/GMP dirs),
    not os.environ: the loader needs LD_LIBRARY_PATH or the run exits 127
    before main()."""
    print(f"  running -problem {problem} -n {n} {n} {n}")
    args = ["-problem", str(problem), "-n", str(n), str(n), str(n),
            "-P", "1", "1", "1"]
    rc, out, err = sh_split(["stdbuf", "-i0", "-o0", "-e0", str(binary)] + args,
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
    unparsed = (runtime_total - parsed) if runtime_total is not None else 0

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

    it = ITER_RE.search(out)
    iters = int(it.group(1)) if it else None
    rs = RESID_RE.search(out)

    if iters == 0:
        print("  *** WARNING: Iterations = 0 -- the solver produced nothing. "
              "That is the mpistubs FLOAT/LONG_DOUBLE fall-through; the tree "
              "is unpatched or the patch regressed. Results are meaningless.")
    if rc != 0:
        why = {127: "loader failure -- a shared library was not found",
               139: "segfault -- check the ARITY of the function in the "
                    "backtrace before suspecting the build",
               134: "abort -- usually an MPFR assertion in the shadow"}
        print(f"  *** non-zero exit ({rc}): {why.get(rc, 'see the stderr log')}")
        for line in (err or out).splitlines()[:3]:
            if line.strip():
                print("     ", line[:150])

    return {
        "benchmark": "amg",
        "precision": precision,
        "opt": "O0",
        "threshold": 45,
        "branch_hooks": n_branch,
        "problem": problem,
        "n": n,
        "flips": parsed,
        "runtime_total": runtime_total,
        "locations": len(sites),
        "sites": dict(sites.most_common()),
        "foreign_sites": dict(foreign),
        "foreign_flips": sum(foreign.values()),
        "ambiguous_lines": sorted(ambiguous),
        "ambiguous_flips": sum(c for k, c in sites.items()
                               if k.startswith("AMBIGUOUS:")),
        "unresolved_lines": sorted(unresolved),
        "unresolved_flips": sum(c for (_fh, ln), c in lines.items()
                                if ln in unresolved),
        "unparsed_lines": unparsed,
        "iterations": iters,
        "residual": rs.group(1) if rs else None,
        "exit_code": rc,
        "log": str(stderr_log.relative_to(HERE)),
        "stdout_log": str(stdout_log.relative_to(HERE)),
    }


def write_results(precision, records, outdir):
    """Same report shape as run_amg_fpchecker.py.  summary.json is a LIST of
    run records; FPChecker's has three entries (one per eta), this has one,
    because EFTSan's threshold is compiled into the runtime."""
    r0 = records[0]
    lines = [f"AMG {precision} -- EFTSanitizer branch flips",
             f"(-{r0['opt']} compile+link, merged module, serial build, "
             f"problem {r0['problem']}, n={r0['n']}^3)", ""]
    for r in records:
        own = (r["flips"] - r["foreign_flips"] - r["ambiguous_flips"]
               - r["unresolved_flips"])
        lines.append(f"{r['flips']} flips @ {r['locations']} loc"
                     f"   (ERRORTHRESHOLD={r['threshold']}, no sweep)"
                     f"   [in-TU {own}, foreign {r['foreign_flips']}, "
                     f"ambiguous {r['ambiguous_flips']}, "
                     f"unresolved {r['unresolved_flips']}]"
                     f"   [iters={r['iterations']}]")
    lines += [
        "",
        f"{r0['branch_hooks']} static instrumented FP comparisons "
        f"(brtrace: {BRTRACE_INSTRUMENTED}).",
        "",
        "NOTE: EFTSan prints a bare line number with no filename.  Files above",
        "      are recovered from the INSTRUMENTED module: each site is a line",
        "      carrying an eftsan_check_branch hook, resolved to its file via",
        "      the debug-info scope chain (-O0, so no inlining).  Only lines",
        "      that actually carry a hook are candidates.  AMBIGUOUS means two",
        "      files genuinely both have an instrumented branch on that line.",
        "      Ground truth is the -O0 census: fp32 GT = 1 site "
        "(gmres.c:573,",
        "      13.76% coverage, so FP is an upper bound); fp64 GT = 0 flips at",
        "      100% coverage over 82 executed sites / 96,372 events -- report",
        "      a false-alarm rate there, not P=0.00/R=1.00.",
        "",
    ]
    for r in records:
        lines.append(f"--- {r['opt']} : {r['flips']} flips @ "
                     f"{r['locations']} loc ---")
        if not r["sites"]:
            lines.append("    (no flips)")
        for site, cnt in r["sites"].items():
            lines.append(f"    {cnt:9d}  {site}")
        lines.append(f"    iterations: {r['iterations']}   "
                     f"residual: {r['residual']}")
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

    with open(outdir / f"amg_{precision}_eftsan_summary.csv", "w") as fh:
        fh.write("location,flips\n")
        for site, cnt in records[0]["sites"].items():
            fh.write(f"{site.split('  [')[0]},{cnt}\n")


# --------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", "--precision", nargs="+", default=["fp32", "fp64"],
                    choices=["fp32", "fp64"])
    ap.add_argument("--problem", type=int, default=2,
                    help="2 = GMRES modified diagonal (the census target); "
                         "1 = AMG-PCG Laplace, which shows no fp32/fp64 "
                         "divergence at any size tested")
    ap.add_argument("-n", type=int, default=5,
                    help="grid points per axis (default 5, the census size)")
    ap.add_argument("--link-override", action="store_true",
                    help="pass -override to llvm-link (duplicate definitions)")
    ap.add_argument("--resolve", default="branches",
                    choices=["branches", "all"],
                    help="how to map EFTSan's bare line numbers to files. "
                         "branches (default): only lines carrying an EFTSan "
                         "branch hook in the instrumented IR -- exact. "
                         "all: every DILocation in the merged module.")
    ap.add_argument("--exclude", nargs="*", default=[], metavar="FILE.c",
                    help="additional source files to skip, on top of the "
                         "per-directory Makefile TU lists")
    ap.add_argument("--keep-bc", action="store_true",
                    help="keep the per-TU .bc files (~100 of them)")
    ap.add_argument("--keep-ll", action="store_true",
                    help="keep the disassembled .ll (large)")
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

    print(f"AMG / EFTSanitizer   problem {args.problem}  n={args.n}^3   "
          f"-O0 (enforced)")
    print(f"  precisions: {' '.join(args.precision)}")
    print(f"  EFT_HOME:   {EFT_HOME}")
    print(f"  bench:      {BENCH_ROOT}")
    print(f"  workdir:    {WORK_ROOT}")
    print(f"  libs:       {' '.join(str(d) for d in lib_dirs)}")
    print("  no eta sweep: ERRORTHRESHOLD is compiled into the runtime (45)")
    if args.problem == 1:
        print("  NOTE: problem 1 shows no fp32/fp64 divergence at any size "
              "tested; problem 2 is the census target.")
    print()

    overall = {}
    for precision in args.precision:
        print(f"===== {precision} =====")
        outdir = RESULT_ROOT / precision
        outdir.mkdir(parents=True, exist_ok=True)

        built = build(precision, outdir, args.link_override, args.keep_bc,
                      env, lib_dirs, args.resolve, args.keep_ll,
                      args.exclude)
        if built is None:
            continue
        binary, line_map, local_files, n_branch = built
        if args.no_run:
            print(f"  built: {binary}\n")
            continue

        records = [run(binary, precision, args.problem, args.n, outdir,
                       line_map, local_files, env, n_branch)]
        write_results(precision, records, outdir)
        overall[precision] = records

        for r in records:
            own = (r["flips"] - r["foreign_flips"] - r["ambiguous_flips"]
                   - r["unresolved_flips"])
            print(f"  -> {r['flips']} flips @ {r['locations']} loc "
                  f"(runtime total {r['runtime_total']})  "
                  f"[in-TU {own}, foreign {r['foreign_flips']}, "
                  f"ambiguous {r['ambiguous_flips']}]  "
                  f"iters={r['iterations']}")
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
                  f"{r['locations']:3d} loc   iters={r['iterations']}   "
                  f"exit={r['exit_code']}")
    print(f"\n{BENCH_NAME}/results/O0/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())