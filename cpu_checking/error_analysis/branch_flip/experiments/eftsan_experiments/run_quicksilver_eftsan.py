#!/usr/bin/env python3
"""
run_quicksilver_eftsan.py

Build QuickSilver under EFTSanitizer branch-flip instrumentation and run it,
keeping fp32 and fp64 results completely separate.  Counterpart to
run_quicksilver_fpchecker.py; same structure as run_lulesh_eftsan.py and
run_amg_eftsan.py.

Place in: branch_flip/experiments/eftsan_experiments/

    ./run_quicksilver_eftsan.py                  # both precisions
    ./run_quicksilver_eftsan.py -p fp32          # fp32 only
    ./run_quicksilver_eftsan.py -n 4000 -x 5 -N 3   # the census config
    ./run_quicksilver_eftsan.py --no-run         # build and gate only

Layout produced:
    quicksilver/
      results/O0/  fp32/  build.log  run_O0.stdout  run_O0.stderr
                          summary.txt  summary.json  build_info.txt
                          instrumented_sites.csv
                          quicksilver_fp32_eftsan_summary.csv
                   fp64/  ...
      build/O0/    qs_fp32/  qs_fp64/

CENSUS CONFIG (measured uninstrumented; identical on gcc 13.3 and clang)

    -n 4000 -X 10 -Y 10 -Z 10 -x 5 -y 5 -z 5 -I 1 -J 1 -K 1 -N 3

  First fp32-vs-fp64 divergence at cycle 2, so 3 cycles suffice; 0.16 s
  uninstrumented.  Divergence is a sharp function of particle count, not a
  smooth one: nothing flips at n<=2800 within 6 cycles, cycle 4 at n=3200,
  cycle 2 at n=4000.  It is also NON-MONOTONIC in mesh size -- an 8^3 mesh
  never flipped at any particle count up to 5000 while 4^3 and 5^3 both did.
  For Monte Carlo that is expected: whether a particle lands near a facet
  boundary is sampling luck, not a smooth function of resolution.

  Control-flow tallies at cycle 2 (uninstrumented):
        cycle  absorb   scatter  fission  num_seg
   fp32    2     4422     4632      456    16740
   fp64    2     4421     4632      455    16738

  fp64 and ld are IDENTICAL in every control-flow tally at every configuration
  tested, so fp64 is oracle-stable -- consistent with NAS and AMG, unlike
  LULESH.

  THE RNG IS NOT A CONFOUND: QuickSilver's generator is a 64-bit integer LCG
  (MC_RNG_State.hh) and rngSpawn hashes with pseudo_des, also integer.  The
  seed stream is BIT-IDENTICAL across precisions, so particle histories start
  from identical draws and any divergence is physics arithmetic, not sampling.
  QS_RNG_NATIVE defaults to 0, mapping the seed to (0,1) in double and
  rounding once to qs_real -- that cast is the generator's sole precision
  boundary.

WHY THIS IS NOT THE FPCHECKER HARNESS WITH A DIFFERENT COMPILER

  * No eta sweep.  EFTSan has no runtime knob; sensitivity is ERRORTHRESHOLD,
    a COMPILE-TIME constant in $EFT_HOME/runtime/handleReal.h, and that header
    is not tracked as a make dependency -- `rm -rf obj` before rebuilding or
    the edit silently does nothing.  Keep it at 45.

  * -O0 only, enforced rather than defaulted.  Adjudicate against the -O0
    census.  There is deliberately no --opt flag.

  * The Makefile is not used.  EFTSan needs per-TU bitcode, a merge, then the
    pass, then a link; a Makefile that goes .cc -> .o cannot express that.
    The TU list is read FROM the Makefile (the .cc/.o names it lists) so the
    instrumented binary is built from the same TUs as the reference build.

BUILD PIPELINE (the ordering is load-bearing)

    1. clang++ -O0 -g -emit-llvm -c (each .cc)  -> one .bc per TU
    2. llvm-link all of them                    -> qs_merged.bc
    3. opt -load libEFTSanitizer.so -eftsan     -> qs.opt.bc
    4. clang++ -O0 qs.opt.bc -leftsanitizer     -> qs_<prec>.eftsan

  Step 2 MUST precede step 3.  Instrumenting TUs separately and linking the
  objects corrupts function-argument shadow bookkeeping at call sites -- the
  callee prologue's eftsan_get_arg sequence reads garbage and the program dies
  in the prologue even though the IR signatures match.

  Step 3 is legacy pass manager syntax and needs an explicit -load.

  Step 4 needs -lm -lmpfr -lgmp -lstdc++ plus -leftsanitizer.  MPFR and GMP
  come from the conda env, not from $EFT_HOME/runtime/obj, and
  setup_eftsan.sh does not put them on LD_LIBRARY_PATH; this script locates
  them and both rpaths and exports them.  Without that the binary links
  cleanly and dies at exec with exit 127.

  -include cstdint: upstream QuickSilver omits <cstdint> and does not compile
  on modern libstdc++ at all.  Unrelated to precision.  -DUSE_MPI=0 is NOT a
  QuickSilver flag; the serial path is selected by simply not defining
  HAVE_MPI, which the stock Makefile already does.

VERIFICATION GATES (a clean build proves nothing)

  1. eftsan symbol count -- zero means the pass did not run.
  2. sizeof(qs_real) probe.  A clean build does NOT prove -DQS_FP32 reached
     QS_Precision.hh; the failure is silent.  fp32 must be 4, fp64 8.
  3. Loader preflight (ldd) before the run.
  4. Control-flow tallies vs the uninstrumented reference at the census
     config.  Instrumentation must not perturb the physics; if absorb /
     scatter / fission / num_seg move, the run is not comparable to the
     census and nothing downstream is trustworthy.
  5. instrumented branch hooks -- on LULESH and AMG this matched brtrace
     exactly (75 = 75, 566 = 566).  No brtrace census exists for QuickSilver
     yet, so the number is reported for later comparison rather than checked.

OUTPUT KEYING

  EFTSan's flip print emits a BARE LINE NUMBER, no filename.  This harness
  reads the INSTRUMENTED module, finds every call to an eftsan_check_branch
  hook, and resolves that call's !dbg to file:line through the debug-info
  scope chain (-O0, so no inlining).  Only lines that actually carry a hook
  are candidates; a line still resolving to several files is flagged
  AMBIGUOUS.  instrumented_sites.csv lists the whole static universe, so a
  census site missing from it is a coverage gap rather than a false negative.

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
BENCH_ROOT = Path(os.environ.get(
    "BENCH_ROOT", HERE.parent.parent / "benchmarks" / "quicksilver"))

BENCH_NAME = "quicksilver"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build" / "O0"
RESULT_ROOT = WORK_ROOT / "results" / "O0"

PRECISION_FLAG = {"fp32": "-DQS_FP32", "fp64": ""}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8}

SKIP_DIRS = {"docs", "CMakeFiles", "build", ".git", "obj", "doc", "Examples"}
SRC_DIRS = ["src"]          # where QuickSilver keeps its .cc

# Uninstrumented reference tallies at the census config (n=4000, mesh 5^3).
# Instrumentation must not move these.
REFERENCE_TALLY = {
    "fp32": {2: (4422, 4632, 456, 16740)},
    "fp64": {2: (4421, 4632, 455, 16738)},
}
CENSUS_N, CENSUS_MESH = 4000, 5

# No brtrace census for QuickSilver yet; report the hook count for later.
BRTRACE_INSTRUMENTED = None

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
# tally rows: cycle start source rr split absorb scatter fission produce
#             collisn escape census num_seg scalar_flux ...
TALLY_RE = re.compile(r"^\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+"
                      r"(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+\d+\s+(\d+)")
SRC_RE = re.compile(r"\b([A-Za-z0-9_+-][A-Za-z0-9_.+-]*)\.(?:cc|cpp|o)\b")


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
    QuickSilver's tally table to stdout."""
    p = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE, text=True, errors="replace")
    return p.returncode, p.stdout, p.stderr


def eftsan_env():
    """setup_eftsan.sh puts LLVM 10's clang++/opt/llvm-link on PATH and sets
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
    for t in ("clang++", "opt", "llvm-link", "llvm-dis", "llvm-nm"):
        rc, _ = sh([t, "--version"], env=env)
        if rc != 0:
            missing.append(t)
    return missing


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
    the INSTRUMENTED module -- exactly the sites that can produce a flip
    print.  mode="all": every DILocation in the merged module, which
    over-generates candidates badly."""
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
        print("  (no EFTSan branch hooks matched in the IR -- falling back to "
              "every DILocation; check BRANCH_CALL_RE against the pass)")
        return build_line_map(merged_bc, opt_bc, env, mode="all", log=log,
                              keep_ll=keep_ll)  # returns the 3-tuple

    return {k: sorted(v) for k, v in out.items()}, n_calls, len(branch_dbg)


def write_site_universe(line_map, outdir, name="instrumented_sites.csv"):
    """Every instrumented branch site as file:line -- the static universe the
    tool could possibly report.  A census site absent from this file is a
    COVERAGE GAP; present and unreported is a FALSE NEGATIVE.  Adjudication
    needs to tell those apart, and matching counts alone does not."""
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


# --------------------------------------------------------------------
# Archive semantics: drop TUs whose undefined symbols nothing in the merge
# defines.  A whole-module merge exposes dangling references that the real
# build never hits, because its linker only extracts the members it needs.
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
    """Returns (kept, dropped, blocked).  A TU defining main() is never
    dropped: if main needs the symbol, the problem is real."""
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
    """Source stems a Makefile names.  QuickSilver's Makefile lists objects
    explicitly, but the .c=.o substitution idiom is handled too, so accept
    both .cc/.cpp and .o tokens.  Recipe lines and pattern rules are skipped
    so the rule template is not mistaken for a file list."""
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
    """The TU list QuickSilver's own Makefile builds.

    Prefers the Makefile's object list so the instrumented binary is made of
    the same TUs as the reference build.  Falls back to every .cc in the
    source directory if no Makefile parses, and says so.

    Returns (sources, dropped, fallback_dirs)."""
    excluded = set(extra_exclude)
    srcs, dropped, fallback = [], [], []

    dirs = [tree / d for d in SRC_DIRS if (tree / d).is_dir()]
    if not dirs:                      # flat layout
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
    """sizeof(qs_real) probe.  A clean build does not prove -DQS_FP32 reached
    QS_Precision.hh; the failure is silent."""
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


# --------------------------------------------------------------------
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


def build(precision, outdir, std, link_override, keep_bc, env, lib_dirs,
          resolve_mode, keep_ll, extra_exclude):
    src = BENCH_ROOT / f"qs_{precision}"
    dst = BUILD_ROOT / f"qs_{precision}"
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not src.is_dir():
        print(f"  FATAL: no source tree at {src}")
        print("         set BENCH_ROOT=/path/to/benchmarks/quicksilver")
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
              f"{', '.join(fallback)} -- compiling every source there; "
              f"verify against the real build")
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
        print(f"    LLVM 10 clang++ with -std={std}. QuickSilver's baseline is")
        print("    c++11; if the failure is language-level, that is the knob.")
        return None
    print(f"  compiled {len(bcs)} TUs")

    # ---- 2-4. merge, instrument, link -------------------------------
    merged = dst / "qs_merged.bc"
    opt_bc = dst / "qs.opt.bc"
    binary = dst / f"qs_{precision}.eftsan"
    pruned_total = []

    def attempt(modules):
        print(f"  llvm-link {len(modules)} modules -> {merged.name}")
        link_cmd = ["llvm-link"] + [str(b) for b in modules]
        if link_override:
            link_cmd.append("-override")
        rc, out = sh(link_cmd + ["-o", str(merged)], cwd=dst, env=env, log=log)
        if rc != 0 or not merged.exists():
            return False, set(), ("llvm-link FAILED. Duplicate C++ inline or "
                                  "template definitions are the usual cause; "
                                  "retry with --link-override.\n" +
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
                  "defining main() -- a real missing definition, not dead "
                  "archive code.")
            break
        if not dead:
            break
        print(f"  {len(dead)} TU(s) reference symbols nothing in the merge "
              f"defines; dropping and relinking:")
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
            for sym, _ in syms.most_common(8):
                print(f"      {sym}")
            if any(x.startswith(("mpfr_", "__gmpfr_", "__gmp")) for x in syms):
                print("    -> MPFR/GMP fell off the link line.")
        return None
    bcs = modules

    # ---- gates ------------------------------------------------------
    _, nm_out = sh(["nm", str(binary)], env=env)
    nsym = sum(1 for l in nm_out.splitlines() if "eftsan" in l.lower())
    print(f"  eftsan symbol count = {nsym}")

    width, ok_w = check_width(dst, precision, env, outdir)
    print(f"  sizeof(qs_real) = {width} "
          f"({'ok' if ok_w else 'EXPECTED ' + str(EXPECTED_WIDTH[precision])})")

    print(f"  resolving line numbers from debug info (mode={resolve_mode})")
    line_map, n_branch, n_locs = build_line_map(merged, opt_bc, env, mode=resolve_mode,
                                        log=log, keep_ll=keep_ll)
    collisions = sum(1 for v in line_map.values() if len(v) > 1)
    ref = (f" (brtrace instrumented = {BRTRACE_INSTRUMENTED})"
           if BRTRACE_INSTRUMENTED else " (no brtrace census yet)")
    print(f"  instrumented branch hooks = {n_branch} "
          f"({n_locs} distinct debug locations){ref}")
    print(f"  lines mapped = {len(line_map)} "
          f"({collisions} occur in more than one file)")
    universe = write_site_universe(line_map, outdir)
    print(f"  site universe -> {universe.name}")

    (outdir / "build_info.txt").write_text(
        f"benchmark    = QuickSilver\n"
        f"precision    = {precision}\n"
        f"opt          = -O0  (enforced; EFTSan does not survive -O1+)\n"
        f"compiler     = clang++ (LLVM 10 via {SETUP_SH.name}), -std={std}\n"
        f"TUs compiled = {len(bcs)}\n"
        f"TUs skipped  = {len(dropped)} "
        f"({', '.join(map(str, dropped)) or 'none'})\n"
        f"TUs pruned   = {len(pruned_total)} "
        f"({', '.join(map(str, pruned_total)) or 'none'})\n"
        f"TU list from = Makefile object list"
        f"{' (fallback glob: ' + ', '.join(fallback) + ')' if fallback else ''}\n"
        f"merge        = llvm-link BEFORE opt -eftsan (arg-shadow ordering)\n"
        f"pass         = {pass_so} -eftsan\n"
        f"runtime      = {runtime}\n"
        f"cflags       = "
        f"{' '.join(c for c in cflags if not c.startswith('-I'))}\n"
        f"includes     = {sum(1 for c in cflags if c.startswith('-I'))} dirs\n"
        f"link libs    = -leftsanitizer -lm -lmpfr -lgmp -lstdc++\n"
        f"eftsan_syms  = {nsym}\n"
        f"qs_real      = {width} bytes "
        f"(expected {EXPECTED_WIDTH[precision]})\n"
        f"lines_mapped = {len(line_map)} ({collisions} multi-file), "
        f"mode={resolve_mode}\n"
        f"branch_hooks = {n_branch}  (eftsan_check_branch call sites)\n"
        f"debug_locs   = {n_locs}  (distinct !dbg; several hooks may share "
        f"one source location)\n"
        f"mpi          = serial (QuickSilver's own stubs in utilsMpi.cc)\n"
        f"openmp       = disabled\n"
        f"rng          = integer LCG, bit-identical across precisions\n"
        f"ERRORTHRESHOLD is a compile-time constant of the runtime (45);\n"
        f"there is no eta equivalent and no sweep.\n"
        f"Adjudicate against the -O0 census.\n")

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
        print("  *** WRONG qs_real WIDTH -- the precision flag did not reach "
              "the headers. Aborting this precision.")
        return None

    if not keep_bc:
        for b in bcs:
            b.unlink(missing_ok=True)

    local_files = {p.name for p in dst.rglob("*")
                   if p.suffix in (".cc", ".cpp", ".hh", ".h", ".hpp")}
    return binary, line_map, local_files, n_branch


# --------------------------------------------------------------------
def run(binary, precision, opts, outdir, line_map, local_files, env, n_branch):
    """env MUST be the setup_eftsan.sh environment plus the MPFR/GMP dirs, or
    the loader fails before main() and the run exits 127."""
    args = ["-n", str(opts["n"]),
            "-X", "10", "-Y", "10", "-Z", "10",
            "-x", str(opts["mesh"]), "-y", str(opts["mesh"]),
            "-z", str(opts["mesh"]),
            "-I", "1", "-J", "1", "-K", "1",
            "-N", str(opts["cycles"])]
    print(f"  running -n {opts['n']} mesh {opts['mesh']}^3 "
          f"N={opts['cycles']}")
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

    # Control-flow observables: integer tallies per cycle.
    tallies = []
    for l in out.splitlines():
        m = TALLY_RE.match(l)
        if m:
            tallies.append({"cycle": int(m.group(1)),
                            "absorb": int(m.group(3)),
                            "scatter": int(m.group(4)),
                            "fission": int(m.group(5)),
                            "num_seg": int(m.group(6))})

    # Gate: instrumentation must not perturb the physics.  Only meaningful at
    # the census config, where the uninstrumented reference is known.
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
                    print(f"  *** TALLY MISMATCH -- instrumentation perturbed "
                          f"the physics. {tally_note}")
                    print("      This run is NOT comparable to the census.")
                else:
                    print(f"  tally gate: cycle {t['cycle']} matches the "
                          f"uninstrumented reference")
        if tally_ok is None:
            print("  (no reference cycle in the tally output -- gate skipped)")

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
        "benchmark": "quicksilver",
        "precision": precision,
        "opt": "O0",
        "threshold": 45,
        "branch_hooks": n_branch,
        "n": opts["n"], "mesh": opts["mesh"], "cycles": opts["cycles"],
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
        "tallies": tallies,
        "tally_matches_reference": tally_ok,
        "tally_note": tally_note,
        "exit_code": rc,
        "log": str(stderr_log.relative_to(HERE)),
        "stdout_log": str(stdout_log.relative_to(HERE)),
    }


def write_results(precision, records, outdir):
    """Same report shape as run_quicksilver_fpchecker.py.  summary.json is a
    LIST of run records; FPChecker's has three (one per eta), this has one,
    because EFTSan's threshold is compiled into the runtime."""
    r0 = records[0]
    lines = [f"QuickSilver {precision} -- EFTSanitizer branch flips",
             f"(-{r0['opt']} compile+link, merged module, serial build, "
             f"n={r0['n']} mesh={r0['mesh']}^3 N={r0['cycles']})", ""]
    for r in records:
        own = (r["flips"] - r["foreign_flips"] - r["ambiguous_flips"]
               - r["unresolved_flips"])
        lines.append(f"{r['flips']} flips @ {r['locations']} loc"
                     f"   (ERRORTHRESHOLD={r['threshold']}, no sweep)"
                     f"   [in-TU {own}, foreign {r['foreign_flips']}, "
                     f"ambiguous {r['ambiguous_flips']}, "
                     f"unresolved {r['unresolved_flips']}]")
    lines += [
        "",
        f"{r0['branch_hooks']} static instrumented FP comparisons "
        f"(see instrumented_sites.csv).",
        "",
        "NOTE: EFTSan prints a bare line number with no filename.  Files above",
        "      are recovered from the INSTRUMENTED module: each site is a line",
        "      carrying an eftsan_check_branch hook, resolved to its file via",
        "      the debug-info scope chain (-O0, so no inlining).  AMBIGUOUS",
        "      means two files genuinely both have an instrumented branch on",
        "      that line.  A census site absent from instrumented_sites.csv is",
        "      a coverage gap, not a false negative.",
        "      Ground truth is the -O0 census, NOT the -O2 one.",
        "",
    ]
    if r0["tallies"]:
        lines.append("control-flow tallies (must match the uninstrumented run):")
        lines.append("  cycle   absorb  scatter  fission  num_seg")
        for t in r0["tallies"]:
            lines.append(f"  {t['cycle']:5d} {t['absorb']:8d} {t['scatter']:8d}"
                         f" {t['fission']:8d} {t['num_seg']:8d}")
        if r0["tally_matches_reference"] is True:
            lines.append("  -> matches the uninstrumented reference")
        elif r0["tally_matches_reference"] is False:
            lines.append(f"  -> MISMATCH: {r0['tally_note']}")
            lines.append("     Instrumentation perturbed the physics; this run")
            lines.append("     is not comparable to the census.")
        lines.append("")
    for r in records:
        lines.append(f"--- {r['opt']} : {r['flips']} flips @ "
                     f"{r['locations']} loc ---")
        if not r["sites"]:
            lines.append("    (no flips)")
        for site, cnt in r["sites"].items():
            lines.append(f"    {cnt:9d}  {site}")
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

    with open(outdir / f"quicksilver_{precision}_eftsan_summary.csv", "w") as fh:
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
    ap.add_argument("-n", type=int, default=4000,
                    help="particles (default 4000; first divergence at "
                         "cycle 2, and divergence is a sharp function of "
                         "this, not a smooth one)")
    ap.add_argument("-x", "--mesh", type=int, default=5,
                    help="mesh cells per axis (default 5; divergence is "
                         "NON-monotonic in mesh -- 8^3 never flipped)")
    ap.add_argument("-N", "--cycles", type=int, default=3)
    ap.add_argument("--std", default="c++11",
                    help="QuickSilver's baseline; the EFTSan toolchain is "
                         "LLVM 10, so newer standards are a gamble")
    ap.add_argument("--link-override", action="store_true",
                    help="pass -override to llvm-link (duplicate C++ "
                         "inline/template definitions)")
    ap.add_argument("--resolve", default="branches",
                    choices=["branches", "all"],
                    help="how to map EFTSan's bare line numbers to files. "
                         "branches (default): only lines carrying an EFTSan "
                         "branch hook in the instrumented IR -- exact.")
    ap.add_argument("--exclude", nargs="*", default=[], metavar="FILE.cc",
                    help="additional source files to skip")
    ap.add_argument("--keep-bc", action="store_true",
                    help="keep the per-TU .bc files")
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

    opts = {"n": args.n, "mesh": args.mesh, "cycles": args.cycles}
    print(f"QuickSilver / EFTSanitizer   n={args.n} mesh={args.mesh}^3 "
          f"N={args.cycles}   -O0 (enforced)")
    print(f"  precisions: {' '.join(args.precision)}")
    print(f"  EFT_HOME:   {EFT_HOME}")
    print(f"  bench:      {BENCH_ROOT}")
    print(f"  workdir:    {WORK_ROOT}")
    print(f"  libs:       {' '.join(str(d) for d in lib_dirs)}")
    print("  no eta sweep: ERRORTHRESHOLD is compiled into the runtime (45)")
    if args.n != CENSUS_N or args.mesh != CENSUS_MESH:
        print(f"  NOTE: off the census config (n={CENSUS_N}, "
              f"mesh={CENSUS_MESH}^3) -- the tally gate is skipped")
    print()

    overall = {}
    for precision in args.precision:
        print(f"===== {precision} =====")
        outdir = RESULT_ROOT / precision
        outdir.mkdir(parents=True, exist_ok=True)

        built = build(precision, outdir, args.std, args.link_override,
                      args.keep_bc, env, lib_dirs, args.resolve, args.keep_ll,
                      args.exclude)
        if built is None:
            continue
        binary, line_map, local_files, n_branch = built
        if args.no_run:
            print(f"  built: {binary}\n")
            continue

        records = [run(binary, precision, opts, outdir, line_map, local_files,
                       env, n_branch)]
        write_results(precision, records, outdir)
        overall[precision] = records

        for r in records:
            own = (r["flips"] - r["foreign_flips"] - r["ambiguous_flips"]
                   - r["unresolved_flips"])
            print(f"  -> {r['flips']} flips @ {r['locations']} loc "
                  f"(runtime total {r['runtime_total']})  "
                  f"[in-TU {own}, foreign {r['foreign_flips']}, "
                  f"ambiguous {r['ambiguous_flips']}]")
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
                  f"{r['locations']:3d} loc   {tg}   exit={r['exit_code']}")

    # Cross-precision control-flow divergence, the same check the FPChecker
    # harness does: divergence should appear at cycle 2 at the census config.
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
                print("  none -- tallies identical. At the census config that "
                      "is itself a finding: instrumentation changed the run.")

    print(f"\n{BENCH_NAME}/results/O0/<precision>/ : "
          f"{', '.join(sorted(overall))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())