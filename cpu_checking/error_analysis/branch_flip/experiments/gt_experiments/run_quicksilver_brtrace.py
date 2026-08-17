#!/usr/bin/env python3
"""
run_quicksilver_brtrace.py

Build the three precision variants of QuickSilver under the brtrace pass, run
each once, and diff two pairs to produce the ground-truth branch-flip census:

    fp32 vs fp64      does single precision flip branches relative to double?
    fp64 vs ld        is double itself oracle-stable against long double?

fp64 is built and run ONCE per optimisation level and its trace feeds both
diffs, so this costs three builds and three runs per level, not four.

Place in: branch_flip/experiments/gt_experiments/

    ./run_quicksilver_brtrace.py                  # -O0, census config
    ./run_quicksilver_brtrace.py --opt O0 O2      # both levels
    ./run_quicksilver_brtrace.py -n 5000 -x 4 -N 7   # the pure-facet config
    ./run_quicksilver_brtrace.py --dry-run
    ./run_quicksilver_brtrace.py --skip-build --skip-run   # re-diff

CENSUS CONFIG (from run_quicksilver_fpchecker.py, measured uninstrumented):

    -n 4000 -X 10 -Y 10 -Z 10 -x 5 -y 5 -z 5 -I 1 -J 1 -K 1 -N 3

First fp32-vs-fp64 divergence at cycle 2, so 3 cycles suffice. Divergence is a
sharp, non-monotonic function of particle count and mesh size -- 2800 particles
never flips within 6 cycles, 3200 flips at cycle 4, 4000 at cycle 2; an 8^3
mesh never flipped at any count up to 5000. For Monte Carlo that is expected:
whether a particle lands near a facet boundary is sampling luck, not a smooth
function of resolution. Do not "tune" these numbers without re-measuring.

Alternate config worth keeping: -n 5000 -x 4 -N 7 diverges in num_seg ONLY
(18482 vs 18481) with absorb/scatter/fission identical -- a pure
facet-crossing-vs-collision boundary flip with no reaction-count contamination.

RNG: QuickSilver's generator is a 64-bit integer LCG and rngSpawn hashes with
pseudo_des, also integer, so the seed stream is BIT-IDENTICAL across
fp32/fp64/ld. Particle histories start from identical draws and any divergence
is physics arithmetic, not sampling. QS_RNG_NATIVE defaults to 0, which maps
the seed to (0,1) in double and rounds once to qs_real, making that cast the
generator's sole precision boundary.

TALLY CROSS-CHECK (QuickSilver-specific, and the reason this harness is worth
more than the others): QuickSilver prints integer per-cycle tallies -- absorb,
scatter, fission, num_seg. Those are control-flow observables computed
independently of brtrace. If the trace says the trajectories diverged but the
tallies are identical, or vice versa, something is wrong with the experiment.
This script extracts them per variant and diffs them alongside the census.

Layout produced (one subtree per optimisation level):

    quicksilver/
      results/
        O0/  builds/   fp32.build.log  fp32.run.log  fp32.build_info.txt ...
             traces/   qs_fp32.out  qs_fp64.out  qs_ld.out
             fp32_vs_fp64/  report.txt  flips.csv  diff.log
             fp64_vs_ld/    report.txt  flips.csv  diff.log
             tallies.txt  summary.txt  summary.json
        O2/  ...
      build/
        O0/qs_fp32/  O0/qs_fp64/  O0/qs_ld/  O2/...

GATES (all silent failure modes, each seen at least once on this project):
  1. plugin probe -- does the pass run, and does -brtrace-fp-only filter?
  2. matched-build -- do all variants instrument the same branches?
  3. sizeof(qs_real) -- did -DQS_FP32 / -DQS_LD reach QS_Precision.hh?
  4. objdump precision -- fp32 must emit single-precision arithmetic, ld must
     emit x87, and no two variants may match. Without this a missed precision
     flag gives three identical trajectories, zero flips, and a result that
     reads as "fp32 is stable" rather than a broken experiment.

BUILD NOTES
  * -include cstdint: upstream QuickSilver omits <cstdint> and does not compile
    on modern libstdc++ at all. Unrelated to precision.
  * CPPFLAGS is cleared, and no -fopenmp is passed. Single-threaded is
    mandatory: brtrace walks the two traces in lock-step, which requires a
    deterministic event order.
  * Serial MPI comes from QuickSilver's own stubs in utilsMpi.cc, selected by
    simply not defining HAVE_MPI. There is no -DUSE_MPI=0 flag here.
"""

import argparse
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import time
from collections import defaultdict
from pathlib import Path

# --------------------------------------------------------------------
FPC_ROOT = Path(os.environ.get("FPC_ROOT", "/usr/workspace/das9/fpchecker_bf"))
BRANCH_FLIP = FPC_ROOT / "cpu_checking/error_analysis/branch_flip"
BENCH_ROOT = Path(os.environ.get("BENCH_ROOT",
                                 BRANCH_FLIP / "benchmarks/quicksilver"))


def _find_brx():
    """Locate the brtrace dir. Only a candidate holding the plugin counts, so
    an empty or half-built dir cannot shadow a complete one."""
    env = os.environ.get("BRX_ROOT")
    here = Path(__file__).resolve().parent
    candidates = ([Path(env)] if env else []) + [
        here / "brtrace", here.parent / "brtrace",
        BRANCH_FLIP / "brtrace",
        FPC_ROOT / "cpu_checking/error_analysis/brtrace",
    ]
    for c in candidates:
        if (c / "libBranchTrace_mtu.so").exists():
            return c.resolve()
    for c in candidates:
        if c.is_dir():
            return c.resolve()
    return candidates[0].resolve()


BRX_ROOT = _find_brx()

HERE = Path(__file__).resolve().parent
BENCH_NAME = "quicksilver"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

VARIANTS = {"fp32": "qs_fp32", "fp64": "qs_fp64", "ld": "qs_ld"}
PRECISION_FLAG = {"fp32": "-DQS_FP32", "fp64": "", "ld": "-DQS_LD"}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8, "ld": 16}

PAIRS = [("fp32_vs_fp64", "fp32", "fp64"),
         ("fp64_vs_ld", "fp64", "ld")]

BIN_REL = Path("qs")

SITE_RE = re.compile(
    r"^\[BranchTrace\]\s+(\S+)\s+\(mod\s+(\d+)\):\s+instrumented\s+(\d+)\s+branch sites")
# QuickSilver per-cycle tally rows:
#   cycle start source rr split absorb scatter fission produce collisn escape
#   census num_seg scalar_flux ...
# These integer counts are control-flow observables derived independently of
# brtrace, so they cross-check the trace: divergence in one should show in the
# other.
TALLY_RE = re.compile(r"^\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+"
                      r"(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s+\d+\s+(\d+)")


def parse_tallies(text):
    out = []
    for line in text.splitlines():
        m = TALLY_RE.match(line)
        if m:
            out.append({"cycle": int(m.group(1)), "absorb": int(m.group(3)),
                        "scatter": int(m.group(4)), "fission": int(m.group(5)),
                        "num_seg": int(m.group(6))})
    return out


def diff_tallies(a, b):
    """Rows where two variants' tallies differ, with the first cycle noted."""
    rows = []
    for x, y in zip(a, b):
        if x != y:
            rows.append((x, y))
    return rows

PRECISION_MARKERS = [
    ("single", r"\b(mulss|addss|subss|divss|cvtss2sd)\b"),
    ("double", r"\b(mulsd|addsd|subsd|divsd|cvtsd2ss)\b"),
    ("x87/ld", r"\b(fldt|fstpt|fmulp|faddp|fdivp)\b"),
]

BUILD_HINTS = [
    (r"Unknown command line argument '?-+brtrace-fp-only",
     "the plugin did not register -brtrace-fp-only. -fpass-plugin alone loads "
     "too late for\n          -mllvm parsing; -Xclang -load -Xclang <so> is "
     "also required (this script passes both).\n          If it still fails, "
     "rebuild: cd <brx> && ./build_mtu.sh"),
    (r"unable to load plugin|cannot open shared object|undefined symbol",
     "the plugin could not be loaded -- almost certainly built against a "
     "different LLVM\n          than the active clang. Compare "
     "`llvm-config --version` with `clang --version`,\n          then rebuild: "
     "cd <brx> && ./build_mtu.sh"),
    (r"No such file or directory.*brtrace_runtime_mtu\.o",
     "the brtrace runtime object is missing -- run ./build_mtu.sh."),
    (r"cstdint|uint64_t.*does not name a type|was not declared",
     "upstream QuickSilver omits <cstdint> and will not compile on modern "
     "libstdc++.\n          -include cstdint is passed by default; if you "
     "overrode --cxxflags, put it back."),
    (r"QS_Precision\.hh: No such file",
     "include path wrong -- check the -I in the width probe matches the tree "
     "layout."),
]


# ------------------------------------------------------------------ helpers

def log(msg=""):
    print(msg, flush=True)


def die(msg):
    sys.stderr.write("ERROR: %s\n" % msg)
    sys.exit(1)


def human(n):
    for unit in ("B", "KB", "MB", "GB", "TB"):
        if n < 1024 or unit == "TB":
            return "%.1f %s" % (n, unit)
        n /= 1024.0


def sh(cmd, cwd=None, env=None, logfile=None, dry=False, quiet=False):
    if not quiet:
        log("    $ %s" % cmd)
        if cwd:
            log("      (cwd %s)" % cwd)
    if dry:
        return 0, ""
    t0 = time.time()
    p = subprocess.run(cmd, cwd=str(cwd) if cwd else None, env=env, shell=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True, errors="replace")
    dt = time.time() - t0
    if logfile:
        Path(logfile).write_text(
            "$ %s\n(cwd %s)\n\n%s\n[exit %d in %.1fs]\n"
            % (cmd, cwd or os.getcwd(), p.stdout, p.returncode, dt))
    if not quiet:
        log("      -> exit %d in %.1fs" % (p.returncode, dt))
    return p.returncode, p.stdout


def parse_site_counts(text):
    """module basename -> (module_id, total site count).

    Summed per basename rather than overwritten: with a multi-directory build
    the same basename can legitimately appear more than once in the log, and
    silently keeping the last one would hide exactly the collision the
    basename check is looking for.
    """
    acc = {}
    for line in text.splitlines():
        m = SITE_RE.match(line.strip())
        if m:
            base = os.path.basename(m.group(1))
            mid, n = int(m.group(2)), int(m.group(3))
            if base in acc:
                acc[base] = (mid, acc[base][1] + n)
            else:
                acc[base] = (mid, n)
    return acc


# ---------------------------------------------- AMG-specific: basename check

def basename_collision_check(src, allow):
    """Find .c files sharing a basename across subdirectories.

    module_id is FNV of the BASENAME (deliberately, so qs_fp32/x.cc and
    qs_fp64/x.cc align). If the tree has subdirectories, two distinct
    sources with the same basename share a module_id, both number site_id from
    zero, and their sites alias into each other. Nothing downstream detects
    it: the builds succeed, counts agree, the diff runs.
    """
    by_base = defaultdict(list)
    for pat in ("*.cc", "*.cpp", "*.cxx", "*.c"):
        for p in Path(src).rglob(pat):
            by_base[p.name].append(p.relative_to(src))
    dupes = {b: v for b, v in by_base.items() if len(v) > 1}
    if not dupes:
        log("    no basename collisions across %d source file(s)"
            % len(by_base))
        return
    log("    %d colliding basename(s):" % len(dupes))
    for b in sorted(dupes):
        log("      %-28s %s" % (b, ", ".join(str(x) for x in sorted(dupes[b]))))
    msg = ("%d source basename(s) appear in more than one directory.\n"
           "       brtrace derives module_id from the basename, so these share "
           "a module_id,\n       both number site_id from zero, and their "
           "sites alias -- flips would be\n       attributed to the wrong "
           "file. Exclude them, rename them, or patch the\n       pass to hash "
           "the path relative to the tree root." % len(dupes))
    if allow:
        sys.stderr.write("WARNING: %s\n" % msg)
    else:
        die(msg)


# -------------------------------------------- QS-specific: qs_real width

def width_check(tree, variant, cc, dry=False):
    """Compile a probe against this tree's headers and report sizeof(qs_real).

    A clean QuickSilver build does not prove -DQS_FP32 / -DQS_LD reached
    QS_Precision.hh -- that failure is silent, and produces three trajectories
    at the same precision.
    """
    if dry:
        return None
    probe = Path(tree) / "_width_probe.cc"
    probe.write_text('#include <cstdio>\n#include "QS_Precision.hh"\n'
                     'int main(){ printf("%zu\\n", sizeof(qs_real)); '
                     'return 0; }\n')
    out_bin = Path(tree) / "_width_probe"
    flag = PRECISION_FLAG[variant]
    # Both -I roots, since the tree may be flat or have the sources under src/.
    cmd = ("%s -O0 -std=c++11 -include cstdint %s -I%s -I%s/src %s -o %s"
           % (cc, flag, tree, tree, probe, out_bin))
    rc, _ = sh(cmd, quiet=True)
    if rc != 0:
        return None
    rc, out = sh(str(out_bin), quiet=True)
    try:
        return int(out.strip())
    except ValueError:
        return None
    finally:
        for f in (probe, out_bin):
            try:
                f.unlink()
            except OSError:
                pass


# -------------------------------------------------------------------- probe

PROBE_SRC = """
double g(double a, double b) {
  double s = 0;
  for (int i = 0; i < 3; i++) {      // integer-controlled
    if (a > b) s += a; else s += b;  // fp-controlled
  }
  if (a == b) s += 1.0;              // fp-controlled
  return s;
}
int main() { return (int)g(1.0, 2.0); }
"""


def probe_plugin(cfg, opt, fp_only, dry=False):
    """Compile a tiny file both ways: does the pass run, and does
    -brtrace-fp-only actually filter? Seconds here beats an hour of AMG."""
    if dry:
        return
    import tempfile
    log("  [probe] verifying the pass runs and -brtrace-fp-only takes effect")
    with tempfile.TemporaryDirectory() as td:
        (Path(td) / "probe.cc").write_text(PROBE_SRC)
        results = {}
        modes = [("all-branches",
                  "-%s -g -fpass-plugin=%s" % (opt, cfg["plugin"]))]
        if fp_only:
            modes.append(("fp-only", cfg["cflags"](opt)))
        for mode, flags in modes:
            rc, out = sh("%s %s -c probe.cc -o probe.o" % (cfg["cc"], flags),
                         cwd=td, quiet=True)
            n = sum(c[1] for c in parse_site_counts(out).values())
            results[mode] = (rc, n, out)
            log("    %-14s exit %d, %d site(s)" % (mode, rc, n))

        for mode, (rc, n, outp) in results.items():
            if rc != 0:
                for line in outp.strip().splitlines()[-15:]:
                    log("    %s" % line[:200])
                for pat, hint in BUILD_HINTS:
                    if re.search(pat, outp, re.I):
                        log("    HINT: %s" % hint)
                die("probe failed in %s mode -- fix before building AMG" % mode)
            if n == 0:
                die("probe compiled in %s mode but emitted no [BranchTrace] "
                    "banner.\n       The plugin is not running. Rebuild it "
                    "against the active LLVM:\n       cd %s && ./build_mtu.sh"
                    % (mode, BRX_ROOT))

        if fp_only and "all-branches" in results:
            n_all, n_fp = results["all-branches"][1], results["fp-only"][1]
            if n_fp >= n_all:
                log("    WARNING: fp-only instrumented %d site(s), "
                    "all-branches %d." % (n_fp, n_all))
                log("             The flag is accepted but not filtering; "
                    "counts would be over")
                log("             ALL branches despite the caption.")
            else:
                log("    ok -- fp-only filters (%d -> %d sites on the probe)"
                    % (n_all, n_fp))
    log()


# -------------------------------------------------------------------- build

def build(variant, opt, cfg, outdir, dry=False):
    src = BENCH_ROOT / VARIANTS[variant]
    dst = BUILD_ROOT / opt / VARIANTS[variant]
    if not src.is_dir():
        die("no source tree at %s" % src)

    if not dry:
        dst.parent.mkdir(parents=True, exist_ok=True)
        if dst.exists():
            shutil.rmtree(dst)
        shutil.copytree(src, dst)
        # Stale objects, archives, a prebuilt binary, and side tables from an
        # earlier run would all be picked up silently.
        for pat in ("**/*.o", "**/*.a", "**/*.brsites", "**/*.brmods",
                    "qs", "src/qs"):
            for junk in dst.glob(pat):
                if junk.is_file():
                    junk.unlink()

    flag = PRECISION_FLAG[variant]
    cflags = ("%s %s %s" % (cfg["cflags"](opt), cfg["cflags_extra"], flag)).strip()
    cflags = re.sub(r"\s+", " ", cflags)
    # The runtime object goes on the link line. CPPFLAGS is cleared so the
    # stock Makefile cannot reintroduce -fopenmp or -DHAVE_MPI behind our
    # back: OpenMP would make the event order non-deterministic and MPI would
    # change the branch structure.
    lflags = "-g -%s %s -lm" % (opt, cfg["runtime"])
    cmd = ("make -j%d CXX=%s CXXFLAGS=%s CPPFLAGS= LDFLAGS=%s"
           % (cfg["jobs"], shlex.quote(cfg["cc"]),
              shlex.quote(cflags), shlex.quote(lflags)))
    logf = outdir / "builds" / ("%s.build.log" % variant)
    log("  %s" % variant)
    rc, out = sh(cmd, cwd=dst, logfile=logf, dry=dry)
    if dry:
        return dst / BIN_REL, {}

    binary = dst / BIN_REL
    if not binary.exists():
        for line in out.splitlines():
            if "error:" in line.lower():
                log("    %s" % line[:180])
                break
        log("    --- last 20 line(s) of build output ---")
        for line in out.strip().splitlines()[-20:]:
            log("    %s" % line[:200])
        log("    ---")
        for pat, hint in BUILD_HINTS:
            if re.search(pat, out, re.I):
                log("    HINT: %s" % hint)
        die("build failed for %s at -%s -- see %s" % (variant, opt, logf))

    counts = parse_site_counts(out)
    if not counts:
        die("no [BranchTrace] banner in the %s -%s build -- the pass did not "
            "run.\n       See %s." % (variant, opt, logf))
    total = sum(n for _, n in counts.values())
    log("      instrumented %d sites across %d TU(s)" % (total, len(counts)))
    top = sorted(counts.items(), key=lambda kv: -kv[1][1])[:8]
    for mod, (mid, n) in top:
        log("        %-28s mod %-11d %5d sites" % (mod, mid, n))
    if len(counts) > len(top):
        log("        ... %d more TU(s)" % (len(counts) - len(top)))

    # Gate: did the precision flag reach the headers?
    width = width_check(dst, variant, cfg["probe_cc"], dry)
    exp = EXPECTED_WIDTH[variant]
    log("      sizeof(qs_real) = %s (expected %d)%s"
        % (width, exp, "" if width == exp else "   *** MISMATCH"))

    (outdir / "builds" / ("%s.build_info.txt" % variant)).write_text(
        "variant        = %s\n"
        "opt            = -%s   (compile AND link)\n"
        "CXX            = %s\n"
        "CXXFLAGS       = %s\n"
        "LDFLAGS        = %s\n"
        "precision flag = %s\n"
        "scope          = %s\n"
        "src            = %s\n"
        "build_dir      = %s\n"
        "sites_total    = %d across %d TU(s)\n"
        "qs_real        = %s bytes (expected %d)\n"
        "openmp         = disabled (no -fopenmp); the run is single-threaded,\n"
        "                 which brtrace requires -- with >1 thread the\n"
        "                 interleaving of __brtrace_log calls differs run to\n"
        "                 run and traces cannot be walked in lock-step.\n"
        % (variant, opt, cfg["cc"], cflags, lflags, flag or "(none, default)",
           cfg["scope"], src, dst, total, len(counts), width, exp))

    if width != exp:
        die("sizeof(qs_real) is %s for %s, expected %d -- the precision flag "
            "did not\n       reach QS_Precision.hh. Every trajectory would be "
            "at the same precision\n       and the diff would report zero "
            "flips, which reads as a clean result.\n       Expected flag: %s"
            % (width, variant, exp, PRECISION_FLAG[variant] or "(none)"))
    return binary, counts


# ---------------------------------------------------------------- run checks

def check_matched(counts_by_variant, variants, allow, dry=False):
    if dry or len(counts_by_variant) < 2:
        return
    log("  [matched-build check]")
    allmods = set()
    for v in counts_by_variant:
        allmods |= set(counts_by_variant[v])
    bad = []
    for mod in sorted(allmods):
        row, vals = [], set()
        for v in variants:
            c = counts_by_variant.get(v, {}).get(mod)
            row.append("n/a" if c is None else str(c[1]))
            if c is not None:
                vals.add(c[1])
        if len(vals) != 1:
            bad.append(mod)
            log("    %-28s %s   MISMATCH"
                % (mod, "  ".join("%6s" % r for r in row)))
    log("    (columns: %s)" % ", ".join(variants))
    if bad:
        msg = ("per-module site counts differ across variants for: %s.\n"
               "       Site ids will not correspond, so every diff would be "
               "garbage." % ", ".join(bad))
        if allow:
            sys.stderr.write("WARNING: %s\n" % msg)
        else:
            die(msg)
    else:
        log("    all %d module(s) agree -- site ids correspond across variants"
            % len(allmods))


def precision_check(binaries, variants, allow, dry=False):
    """Disassemble and count precision-revealing instructions.

    matched-build confirms the variants have the SAME branch structure. It
    cannot confirm they have DIFFERENT arithmetic, and if a precision flag is
    missed they will not: builds succeed, counts agree, traces are the same
    length, both diffs report zero flips, and the result reads as a clean
    'fp32 is stable' rather than a broken experiment.
    """
    if dry or len(binaries) < 2:
        return
    if not shutil.which("objdump"):
        log("  [precision check] skipped -- objdump not on PATH")
        return
    log("  [precision check]")
    profiles = {}
    for v in variants:
        b = Path(binaries[v])
        if not b.exists():
            continue
        _, out = sh("objdump -d %s" % shlex.quote(str(b)), quiet=True)
        prof = tuple(len(re.findall(pat, out)) for _, pat in PRECISION_MARKERS)
        profiles[v] = prof
        log("    %-5s %s" % (v, "  ".join(
            "%s=%-8d" % (PRECISION_MARKERS[i][0], prof[i])
            for i in range(len(prof)))))

    problems = []
    for a in profiles:
        for b in profiles:
            if a < b and profiles[a] == profiles[b]:
                problems.append("%s and %s emit identical arithmetic profiles"
                                % (a, b))
    if "fp32" in profiles and profiles["fp32"][0] == 0:
        problems.append("fp32 emits NO single-precision arithmetic")
    if "ld" in profiles and profiles["ld"][2] == 0:
        problems.append("ld emits NO x87 instructions -- not long double")
    if problems:
        msg = ("the variants are not at different precisions:\n       - "
               + "\n       - ".join(problems))
        if allow:
            sys.stderr.write("WARNING: %s\n" % msg)
        else:
            die(msg)
    else:
        log("    ok -- all variants emit distinct arithmetic")
    log()


# ---------------------------------------------------------------- reporting

_DEMANGLE_CACHE = {}


def demangle(sym):
    """AMG is C, so there is nothing to demangle -- but the side tables carry
    whatever the pass recorded, and running it through c++filt is harmless and
    keeps output identical in shape to the LULESH harness."""
    if not sym:
        return ""
    if sym not in _DEMANGLE_CACHE:
        try:
            out = subprocess.run(["c++filt", sym], stdout=subprocess.PIPE,
                                 text=True, timeout=10).stdout.strip()
        except Exception:
            out = sym
        _DEMANGLE_CACHE[sym] = out or sym
    return _DEMANGLE_CACHE[sym]


SYSPATH_RE = re.compile(r"/(usr/include|include/c\+\+|bits|gcc)/")


def pretty_site(raw):
    m = re.match(r"^(.*?):(\d+)\s*(?:\[(.*)\])?\s*$", raw.strip())
    if not m:
        return raw, "", False
    path, line, sym = m.group(1), m.group(2), m.group(3) or ""
    return ("%s:%s" % (os.path.basename(path), line), demangle(sym),
            bool(SYSPATH_RE.search(path)))


SITE_LINE_RE = re.compile(
    r"^\s+(\S.*?)\s+\(([\d,]+) flips"
    r"(?: of ([\d,]+) executions)?"
    r"(?:, first @ event#([\d,]+))?\)\s*$")
DIVNOTE_RE = re.compile(
    r"^\s*NOTE: the last site to start flipping.*?(?=\n\s*\n|\n\s*TP\s)",
    re.S | re.M)


def extract_sites(text):
    out = []
    for line in text.splitlines():
        m = SITE_LINE_RE.match(line)
        if m:
            short, func, foreign = pretty_site(m.group(1))
            out.append({"raw": m.group(1), "loc": short, "func": func,
                        "foreign": foreign,
                        "flips": int(m.group(2).replace(",", "")),
                        "executions": (int(m.group(3).replace(",", ""))
                                       if m.group(3) else None),
                        "first": (int(m.group(4).replace(",", ""))
                                  if m.group(4) else None)})
    return out


def extract_divergence_note(text):
    m = DIVNOTE_RE.search(text)
    return ([ln.strip() for ln in m.group(0).strip().splitlines() if ln.strip()]
            if m else [])


def extract_headline(text):
    got = []

    def find(pat, label, fmt=None):
        m = re.search(pat, text, re.M)
        if m:
            got.append((label, fmt(m) if fmt else m.group(1)))

    find(r"^\s*lock-step compared\s+([\d,]+) events\s+\(([\d.]+)% of longer",
         "adjudicated events",
         lambda m: "%s   (%s%% of longer trace)" % (m.group(1), m.group(2)))
    find(r"^\s*unadjudicated\s+([\d,]+) events", "unadjudicated events")
    find(r"^\s*cause: (.+)$", "cause")
    m = re.search(r"^\s*TOTAL\s+([\d,]+)\s+([\d,]+)\s+([\d,]+)\s*$", text, re.M)
    if not m:
        m = re.search(r"^\s*TOTAL\s+TP ([\d,]+)\s+TN ([\d,]+)", text, re.M)
    if m:
        got.append(("TP / TN events", "%s / %s" % (m.group(1), m.group(2))))
    find(r"^\s*flips @ ([\d,]+) line", "flipping lines")
    find(r"^\s*TP\s+sites that flipped at least once\s+(\S+)", "TP sites")
    find(r"^\s*TN\s+sites executed, never flipped\s+(\S+)", "TN sites")
    find(r"^\s*TOTAL sites executed\s+(\S+)", "sites executed")
    find(r"^\s*DEAD\s+instrumented, never executed\s+(\S+)", "DEAD sites")
    find(r"^\s*TOTAL sites instrumented\s+(\S+)", "sites instrumented")
    return got


def flips_at_loc(rec):
    sites = rec["sites"]
    total = sum(x["flips"] for x in sites)
    sysn = sum(x["flips"] for x in sites if x["foreign"])
    tail = ("   [in-TU %d, system %d]" % (total - sysn, sysn)) if sysn else ""
    return "%d flips @ %d loc%s" % (total, len(sites), tail)


def site_line(x, width=8):
    bits = "%*d  %s" % (width, x["flips"], x["loc"])
    if x["func"]:
        bits += "  [%s]" % x["func"]
    if x["executions"]:
        bits += "   (of %s)" % "{:,}".format(x["executions"])
    if x["first"] is not None:
        bits += "   first@%s" % "{:,}".format(x["first"])
    if x["foreign"]:
        bits += "   [system]"
    return bits


def write_results(opt, args, cfg, pair_records, variant_info, outdir):
    lines = ["QuickSilver -- brtrace dual-trajectory ground-truth census",
             "(-%s compile+link, single-threaded, n=%d particles, "
             "mesh=%d^3, N=%d cycles)" % (opt, args.n, args.x, args.N),
             "scope: %s" % cfg["scope"], ""]
    for r in pair_records:
        lines.append("%s  (%s vs %s)" % (r["label"], r["a"], r["b"]))
        lines.append("    %s" % flips_at_loc(r))
        lines.append("")
        for k, v in r["headline"]:
            lines.append("    %-28s %s" % (k, v))
        if r["sites"]:
            lines.append("")
            lines.append("    flip sites:")
            for x in r["sites"]:
                lines.append("    " + site_line(x))
        if r.get("divergence_note"):
            lines.append("")
            for ln in r["divergence_note"]:
                lines.append("    " + ln)
        lines.append("    report: %s" % r["report"])
        lines.append("")
    lines.append("control-flow tallies (final cycle)")
    for v in sorted(variant_info):
        t = variant_info[v].get("tallies")
        if t:
            last = t[-1]
            lines.append("    %-5s cycle %d: absorb=%d scatter=%d fission=%d "
                         "num_seg=%d"
                         % (v, last["cycle"], last["absorb"], last["scatter"],
                            last["fission"], last["num_seg"]))
    lines.append("    (full per-cycle table in tallies.txt)")
    lines.append("")
    lines.append("trace sizes")
    for v in sorted(variant_info):
        info = variant_info[v]
        if "events" in info:
            lines.append("    %-5s %14s events   %s"
                         % (v, "{:,}".format(info["events"]),
                            human(info["bytes"])))
    lines.append("")
    lines.append("NOTE: counts are over %s." % cfg["scope"])
    lines.append("      DEAD sites and unadjudicated events are NOT counted as")
    lines.append("      TN -- dead code was never reached, and post-divergence")
    lines.append("      events have no oracle verdict.")
    (outdir / "summary.txt").write_text("\n".join(lines) + "\n")
    (outdir / "summary.json").write_text(json.dumps(
        {"opt": opt, "n": args.n, "mesh": args.x, "cycles": args.N,
         "scope": cfg["scope"], "variants": variant_info,
         "pairs": pair_records}, indent=2, default=str))
    return "\n".join(lines)



# ------------------------------------------------- tally cross-check (QS-only)

def tally_report(pair_records, variant_info, outdir):
    """Compare QuickSilver's integer per-cycle tallies across variants.

    These counts come out of the physics, not out of brtrace, so they are an
    independent witness. The two should agree about whether the trajectories
    parted: a trace divergence with identical tallies (or identical tallies
    with a trace divergence) means the experiment is measuring something other
    than what it claims.
    """
    lines = ["QuickSilver control-flow tallies", ""]
    have = {v: variant_info[v]["tallies"] for v in variant_info
            if variant_info.get(v, {}).get("tallies")}
    if not have:
        lines.append("  (no tally rows parsed -- cross-check unavailable)")
        (outdir / "tallies.txt").write_text("\n".join(lines) + "\n")
        return

    for v in sorted(have):
        lines.append("  %s" % v)
        lines.append("    cycle   absorb  scatter  fission  num_seg")
        for t in have[v]:
            lines.append("    %5d %8d %8d %8d %8d"
                         % (t["cycle"], t["absorb"], t["scatter"],
                            t["fission"], t["num_seg"]))
        lines.append("")

    log("  [tally cross-check]")
    for label, a, b in PAIRS:
        if a not in have or b not in have:
            continue
        rows = diff_tallies(have[a], have[b])
        rec = next((r for r in pair_records if r["label"] == label), None)
        trace_diverged = bool(rec and any(
            k == "cause" for k, _ in rec.get("headline", [])))
        if rows:
            first = rows[0][0]["cycle"]
            msg = ("%s: tallies differ from cycle %d (%d cycle(s) differ)"
                   % (label, first, len(rows)))
            lines.append("  %s" % msg)
            for x, y in rows:
                lines.append("    cycle %d: absorb %d/%d  scatter %d/%d  "
                             "fission %d/%d  num_seg %d/%d"
                             % (x["cycle"], x["absorb"], y["absorb"],
                                x["scatter"], y["scatter"],
                                x["fission"], y["fission"],
                                x["num_seg"], y["num_seg"]))
        else:
            msg = "%s: tallies IDENTICAL across all cycles" % label
            lines.append("  %s" % msg)
        log("    %s" % msg)

        # The two witnesses should agree.
        if rows and not trace_diverged:
            log("    [warn] tallies differ but the trace never structurally "
                "diverged.")
            log("           Expected: a physics divergence should eventually "
                "change which")
            log("           branch executes. Possible causes: the divergence "
                "is downstream of")
            log("           every instrumented site, or -brtrace-fp-only "
                "filtered out the")
            log("           branch that carries it.")
            lines.append("    [warn] tallies differ but the trace stayed in "
                         "lock-step")
        if trace_diverged and not rows:
            log("    [warn] the trace structurally diverged but the tallies "
                "are identical.")
            log("           The path changed without changing any reaction "
                "count -- possible,")
            log("           but check that both runs used the same problem "
                "arguments.")
            lines.append("    [warn] trace diverged but tallies identical")
    lines.append("")
    (outdir / "tallies.txt").write_text("\n".join(lines) + "\n")
    log()


# --------------------------------------------------------------- one opt run

def run_one_opt(opt, args, cfg, dry=False):
    outdir = RESULT_ROOT / opt
    (outdir / "builds").mkdir(parents=True, exist_ok=True)
    (outdir / "traces").mkdir(parents=True, exist_ok=True)

    log("=" * 74)
    log("  -%s" % opt)
    log("=" * 74)

    variants = args.variants
    binaries, counts_by_variant, variant_info = {}, {}, {}

    if args.skip_build:
        log("  [build] skipped (--skip-build)")
        for v in variants:
            binaries[v] = BUILD_ROOT / opt / VARIANTS[v] / BIN_REL
    else:
        log("  [build]")
        for v in variants:
            b, c = build(v, opt, cfg, outdir, dry=dry)
            binaries[v] = b
            if c:
                counts_by_variant[v] = c
                variant_info[v] = {
                    "sites_total": sum(n for _, n in c.values()),
                    "tus": len(c)}
        log()
        check_matched(counts_by_variant, variants, args.allow_site_mismatch, dry)
        log()
        precision_check(binaries, variants, args.allow_same_precision, dry)

    # --- run
    traces = {v: outdir / "traces" / ("qs_%s.out" % v) for v in variants}
    if args.skip_run:
        log("  [run] skipped (--skip-run)")
        for v in variants:
            if not dry and traces[v].exists():
                sz = traces[v].stat().st_size
                variant_info.setdefault(v, {}).update(
                    {"trace": str(traces[v]), "bytes": sz, "events": sz // 12})
    else:
        log("  [run]")
        runargs = ("-n %d -X 10 -Y 10 -Z 10 -x %d -y %d -z %d "
                   "-I 1 -J 1 -K 1 -N %d"
                   % (args.n, args.x, args.x, args.x, args.N))
        for v in variants:
            binp = Path(binaries[v])
            if not dry and not binp.exists():
                die("%s not found -- build it first (drop --skip-build)" % binp)
            env = os.environ.copy()
            env["OMP_NUM_THREADS"] = "1"
            env["BRTRACE_OUT"] = str(traces[v])
            cmd = "%s./%s %s" % ((args.launcher + " ") if args.launcher else "",
                                 BIN_REL.name, runargs)
            log("  %s -> %s" % (v, traces[v]))
            runlog = outdir / "builds" / ("%s.run.log" % v)
            rc, out = sh(cmd, cwd=binp.parent, env=env, logfile=runlog, dry=dry)
            if dry:
                continue
            if rc != 0:
                die("run failed for %s at -%s (exit %d) -- see %s"
                    % (v, opt, rc, runlog))
            tal = parse_tallies(out)
            sz = traces[v].stat().st_size
            log("      %s  (%s events)   %d tally cycle(s)"
                % (human(sz), "{:,}".format(sz // 12), len(tal)))
            variant_info.setdefault(v, {}).update(
                {"trace": str(traces[v]), "bytes": sz, "events": sz // 12,
                 "tallies": tal})
            if not tal:
                log("      WARNING: no tally rows parsed from %s -- the "
                    "cross-check will be skipped" % runlog)
        log()

    # --- diff
    log("  [diff]")
    pair_records = []
    for label, a, b in PAIRS:
        if label not in args.pairs:
            continue
        if a not in variants or b not in variants:
            log("  %s: skipped (needs variants %s and %s)" % (label, a, b))
            continue
        if not dry:
            for t in (traces[a], traces[b]):
                if not t.exists():
                    die("missing trace %s -- rerun without --skip-run" % t)
        pdir = outdir / label
        pdir.mkdir(parents=True, exist_ok=True)
        report, csv = pdir / "report.txt", pdir / "flips.csv"
        # The reference side supplies the static site universe. AMG scatters
        # .brsites across build subdirectories, so point at the tree root and
        # let the diff glob recursively.
        mods = BUILD_ROOT / opt / VARIANTS[b]
        cmd = ("python3 %s %s %s --mods %s --csv %s --report %s"
               % (shlex.quote(str(cfg["diff"])), shlex.quote(str(traces[a])),
                  shlex.quote(str(traces[b])), shlex.quote(str(mods)),
                  shlex.quote(str(csv)), shlex.quote(str(report))))
        if args.progress:
            cmd += " --progress %d" % args.progress
        if args.fast:
            cmd += " --fast"
        log("  %s  (%s vs %s)" % (label, a, b))
        rc, out = sh(cmd, logfile=pdir / "diff.log", dry=dry)
        if rc not in (0, 1) and not dry:
            for line in out.strip().splitlines()[-20:]:
                log("    %s" % line[:200])
            if rc == 2:
                log("    HINT: exit 2 is argparse -- usually an older "
                    "brtrace_diff_mtu.py without --report/--fast.")
            die("diff failed for %s (exit %d) -- see %s"
                % (label, rc, pdir / "diff.log"))
        if not dry:
            log("      -> %s" % flips_at_loc({"sites": extract_sites(out)}))
            pair_records.append({
                "label": label, "a": a, "b": b, "opt": opt,
                "report": str(report), "csv": str(csv), "exit": rc,
                "headline": extract_headline(out),
                "sites": extract_sites(out),
                "divergence_note": extract_divergence_note(out)})
    log()

    if dry:
        return None, counts_by_variant

    tally_report(pair_records, variant_info, outdir)
    text = write_results(opt, args, cfg, pair_records, variant_info, outdir)
    log(text)
    log()
    return pair_records, counts_by_variant


# -------------------------------------------------------------------- main

def main():
    global BRX_ROOT, BENCH_ROOT, WORK_ROOT, BUILD_ROOT, RESULT_ROOT
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--opt", nargs="+", default=["O0"],
                    choices=["O0", "O1", "O2", "O3"],
                    help="optimisation level(s); each gets its own subtree "
                         "(default: O0)")
    ap.add_argument("-n", type=int, default=4000,
                    help="particles (default 4000: first divergence at cycle "
                         "2, the earliest measured). Divergence is a SHARP, "
                         "non-monotonic function of this -- 2800 never flips "
                         "in 6 cycles, 3200 flips at cycle 4. Re-measure "
                         "before changing.")
    ap.add_argument("-x", type=int, default=5,
                    help="mesh cells per axis (default 5; an 8^3 mesh never "
                         "flipped at any particle count up to 5000)")
    ap.add_argument("-N", type=int, default=3,
                    help="cycles (default 3; first divergence is at cycle 2)")
    ap.add_argument("--brx", default=str(BRX_ROOT))
    ap.add_argument("--bench", default=str(BENCH_ROOT),
                    help="dir holding qs_fp32/ qs_fp64/ qs_ld/")
    ap.add_argument("--work", default=str(WORK_ROOT))
    ap.add_argument("--cc", default="clang++")
    ap.add_argument("--probe-cc", default="clang++",
                    help="plain compiler for the qs_real width probe")
    ap.add_argument("--cflags-extra", default="-std=c++11 -include cstdint",
                    help="extra compile flags. -include cstdint is REQUIRED: "
                         "upstream QuickSilver omits it and will not compile "
                         "on modern libstdc++.")
    ap.add_argument("-j", "--jobs", type=int, default=1,
                    help="make parallelism (default 1; parallel builds were "
                         "observed to change instrumentation)")
    ap.add_argument("--all-branches", action="store_true",
                    help="instrument every conditional branch instead of only "
                         "FP-controlled ones")
    ap.add_argument("--variants", nargs="+", default=["fp32", "fp64", "ld"],
                    choices=["fp32", "fp64", "ld"])
    ap.add_argument("--pairs", nargs="+",
                    default=["fp32_vs_fp64", "fp64_vs_ld"],
                    choices=["fp32_vs_fp64", "fp64_vs_ld"])
    ap.add_argument("--launcher", default="",
                    help="prefix for run commands, e.g. 'flux run -n1'")
    ap.add_argument("--fast", action="store_true")
    ap.add_argument("--progress", type=int, default=25)
    ap.add_argument("--skip-probe", action="store_true")
    ap.add_argument("--skip-build", action="store_true")
    ap.add_argument("--skip-run", action="store_true")
    ap.add_argument("--allow-site-mismatch", action="store_true")
    ap.add_argument("--allow-same-precision", action="store_true")
    ap.add_argument("--allow-basename-collision", action="store_true",
                    help="proceed even if two sources share a basename "
                         "(sites would alias -- don't)")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    BRX_ROOT = Path(args.brx).resolve()
    BENCH_ROOT = Path(args.bench).resolve()
    WORK_ROOT = Path(args.work).resolve()
    BUILD_ROOT = WORK_ROOT / "build"
    RESULT_ROOT = WORK_ROOT / "results"
    dry = args.dry_run

    plugin = BRX_ROOT / "libBranchTrace_mtu.so"
    runtime = BRX_ROOT / "brtrace_runtime_mtu.o"
    diff = BRX_ROOT / "tools" / "brtrace_diff_mtu.py"
    missing = [f for f in (plugin, runtime, diff) if not f.exists()]
    if missing:
        parts = ["brtrace is incomplete at %s\n" % BRX_ROOT]
        for f in (plugin, runtime, diff):
            parts.append("         %-8s %s"
                         % ("ok" if f.exists() else "MISSING",
                            f.name if f.parent == BRX_ROOT
                            else str(f.relative_to(BRX_ROOT))))
        parts.append("")
        if not BRX_ROOT.is_dir():
            parts.append("       That directory does not exist. Pass --brx, or "
                         "set BRX_ROOT.")
        else:
            parts.append("       Build them:")
            parts.append("           cd %s && ./build_mtu.sh" % BRX_ROOT)
        die("\n".join(parts))

    need = ["--report"] + (["--fast"] if args.fast else [])
    _, helptext = sh("python3 %s --help" % shlex.quote(str(diff)), quiet=True)
    absent = [f for f in need if f not in helptext]
    if absent:
        die("the diff tool at\n         %s\n       does not support: %s\n\n"
            "       That is the pre-TP/TN version. Copy the updated\n"
            "       brtrace_diff_mtu.py into %s and rerun."
            % (diff, ", ".join(absent), diff.parent))

    fp_only = not args.all_branches

    def cflags(opt):
        # -brtrace-fp-only is a cl::opt inside the plugin. -fpass-plugin alone
        # is not enough: clang parses -mllvm in ExecuteCompilerInvocation but
        # does not dlopen -fpass-plugin libraries until the backend runs, so
        # the option does not exist yet and clang dies with
        #   Unknown command line argument '-brtrace-fp-only'
        # -Xclang -load -Xclang goes through FrontendOpts::Plugins, which IS
        # loaded before -mllvm parsing. Both are needed: -load registers the
        # option, -fpass-plugin schedules the pass.
        base = "-%s -g -fpass-plugin=%s" % (opt, plugin)
        if fp_only:
            base += " -Xclang -load -Xclang %s -mllvm -brtrace-fp-only" % plugin
        return base

    cfg = {"cc": args.cc, "probe_cc": args.probe_cc, "cflags": cflags,
           "cflags_extra": args.cflags_extra, "jobs": args.jobs,
           "runtime": str(runtime), "diff": diff, "plugin": str(plugin),
           "scope": ("FP-CONTROLLED branches (-brtrace-fp-only)" if fp_only
                     else "ALL conditional branches")}

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)

    log("QuickSilver / brtrace   n=%d  mesh=%d^3  N=%d cycles   opt: %s"
        % (args.n, args.x, args.N, " ".join(args.opt)))
    log("  scope:    %s" % cfg["scope"])
    log("  brx:      %s" % BRX_ROOT)
    log("  bench:    %s" % BENCH_ROOT)
    log("  work:     %s" % WORK_ROOT)
    log("  variants: %s" % ", ".join(
        "%s%s" % (v, (" [%s]" % PRECISION_FLAG[v]) if PRECISION_FLAG[v] else "")
        for v in args.variants))
    log("  pairs:    %s" % ", ".join(args.pairs))
    if dry:
        log("  MODE:     DRY RUN -- nothing will be executed")
    log()

    if args.n < 3000:
        log("  NOTE: below ~3000 particles nothing flipped within 6 cycles at")
        log("        any mesh tested. Divergence is sharp and non-monotonic in")
        log("        particle count, so a smaller problem is not a milder one.")
        log()

    if not args.skip_build:
        # Static scan of the source tree -- worth running even under --dry-run,
        # since it is exactly the kind of thing you want to know before
        # committing to a build.
        log("  [basename collision check]")
        basename_collision_check(BENCH_ROOT / VARIANTS[args.variants[0]],
                                 args.allow_basename_collision)
        log()

    if not (args.skip_probe or args.skip_build):
        probe_plugin(cfg, args.opt[0], fp_only, dry=dry)

    all_pairs = {}
    for opt in args.opt:
        recs, _ = run_one_opt(opt, args, cfg, dry=dry)
        if recs is not None:
            all_pairs[opt] = recs

    if dry:
        log("dry run complete -- nothing executed")
        return 0
    if not all_pairs:
        log("No results produced.")
        return 1

    log("=" * 74)
    log("all results")
    log("=" * 74)
    for opt in args.opt:
        for r in all_pairs.get(opt, []):
            head = dict(r["headline"])
            log("  -%-3s %-14s  %-24s TP sites %-5s TN sites %-6s DEAD %-4s"
                % (opt, r["label"], flips_at_loc(r),
                   head.get("TP sites", "?"), head.get("TN sites", "?"),
                   head.get("DEAD sites", "?")))
            for x in r["sites"][:10]:
                log("        " + site_line(x))
            if len(r["sites"]) > 10:
                log("        ... %d more site(s)" % (len(r["sites"]) - 10))
    log("\nresults under %s/<opt>/" % RESULT_ROOT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
