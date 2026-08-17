#!/usr/bin/env python3
"""
run_lulesh_brtrace.py

Build the three precision variants of LULESH under the brtrace pass, run each
once, and diff two pairs to produce the ground-truth branch-flip census:

    fp32 vs fp64      does single precision flip branches relative to double?
    fp64 vs ld        is double itself oracle-stable against long double?

fp64 is built and run ONCE per optimisation level and its trace feeds both
diffs, so this costs three builds and three runs per level, not four.

Place in: branch_flip/gt_experiments/lulesh/

    ./run_lulesh_brtrace.py                      # both -O0 and -O2
    ./run_lulesh_brtrace.py --opt O0             # one level only
    ./run_lulesh_brtrace.py -s 10 -i 50          # the published census size
    ./run_lulesh_brtrace.py --dry-run            # print every command, run none
    ./run_lulesh_brtrace.py --skip-build --skip-run   # re-diff existing traces

Layout produced (one subtree per optimisation level, so -O0 and -O2 runs never
overwrite each other -- same convention as run_lulesh_fpchecker.py):

    lulesh/
      results/
        O0/  builds/   fp32.build.log  fp32.run.log  fp32.build_info.txt ...
             traces/   lulesh_fp32.out  lulesh_fp64.out  lulesh_ld.out
             fp32_vs_fp64/  report.txt  flips.csv  diff.log
             fp64_vs_ld/    report.txt  flips.csv  diff.log
             summary.txt  summary.json
        O2/  ...
      build/
        O0/lulesh_fp32/  O0/lulesh_fp64/  O0/lulesh_ld/  O2/...

Sources are COPIED out of BENCH_ROOT into build/<opt>/ before compiling. The
pass writes .brsites/.brmods next to each module, so building in place would
have the two optimisation levels overwrite each other's side tables and would
leave generated files scattered through the benchmark tree.

MATCHED-BUILD DISCIPLINE
------------------------
Site ids only correspond if every variant instruments the same branches. The
pass prints one banner per TU ("instrumented K branch sites"); this script
parses them and refuses to diff if any module's K differs across variants,
because a mismatch produces a diff that runs to completion and reports numbers
that mean nothing. Override with --allow-site-mismatch only if you know why.

NOTE on -O2: the brtrace pass registers at PipelineStartEP, so instrumentation
happens BEFORE any optimisation. Static site counts and .brsites attribution
should therefore be identical at -O0 and -O2, and this script cross-checks that
when both levels are run. What -O2 does change is EVENT counts, because the
optimiser may unroll (duplicating a site's calls) or delete dead paths. So -O2
is usable for site-level ground truth, but its event counts are not comparable
to -O0's.

This differs from FPChecker, where -O2 genuinely corrupts attribution:
FPChecker reads DILocation.line without walking getInlinedAt(), which is the
origin of the lulesh.cc:263 vs stl_algobase.h:263 collision. brtrace records
locString at instrumentation time, before inlining, so it does not share that
failure mode.
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
from pathlib import Path

# --------------------------------------------------------------------
# Defaults -- override on the command line or via environment.
# The tree moved fpchecker_new -> fpchecker_bf, so verify these paths.
# --------------------------------------------------------------------
FPC_ROOT = Path(os.environ.get("FPC_ROOT", "/usr/workspace/das9/fpchecker_bf"))
BRANCH_FLIP = FPC_ROOT / "cpu_checking/error_analysis/branch_flip"
BENCH_ROOT = Path(os.environ.get("BENCH_ROOT", BRANCH_FLIP / "benchmarks/lulesh"))


def _find_brx():
    """Locate the brtrace dir.

    It gets kept in different places depending on whether it is being treated
    as a tool or as part of an experiment, so search rather than hardcode. A
    candidate only counts if the plugin is actually present -- an empty or
    half-built brtrace dir should not win over a complete one further down the
    list.
    """
    env = os.environ.get("BRX_ROOT")
    here = Path(__file__).resolve().parent
    candidates = ([Path(env)] if env else []) + [
        here / "brtrace",            # alongside this script
        here.parent / "brtrace",     # one level up
        BRANCH_FLIP / "brtrace",     # the canonical tool location
        FPC_ROOT / "cpu_checking/error_analysis/brtrace",   # pre-move layout
    ]
    for c in candidates:
        if (c / "libBranchTrace_mtu.so").exists():
            return c.resolve()
    # Nothing complete found; return the first that at least exists, so the
    # error message points somewhere useful.
    for c in candidates:
        if c.is_dir():
            return c.resolve()
    return candidates[0].resolve()


BRX_ROOT = _find_brx()

HERE = Path(__file__).resolve().parent
BENCH_NAME = "lulesh"
# Everything for this benchmark lives under <script dir>/lulesh/ so other
# benchmarks (amg, quicksilver) can sit alongside it later -- same convention
# as run_lulesh_fpchecker.py. Override with --work.
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

SRCS = ["lulesh.cc", "lulesh-comm.cc", "lulesh-init.cc",
        "lulesh-util.cc", "lulesh-viz.cc"]

# variant -> source subdirectory under BENCH_ROOT
VARIANTS = {"fp32": "lulesh_fp32", "fp64": "lulesh_fp64", "ld": "lulesh_ld"}

# Extra defines per variant. The lulesh_fp32 tree selects float via a macro,
# not by being unconditionally single precision -- run_lulesh_fpchecker.py
# passes -DLULESH_FP32 for exactly this reason. Without it the fp32 tree
# compiles as double and every diff comes back with zero flips, which looks
# like a clean result instead of a broken build.
# LULESH now selects precision by compile-time switch in lulesh.h,
# so ld needs its macro too -- without it the ld tree builds as fp64
# and fp64_vs_ld reports zero flips, which reads as "fp64 is
# oracle-stable" rather than as a broken build.
DEFINES = {"fp32": "-DLULESH_FP32", "fp64": "", "ld": "-DLULESH_LD"}

# Instruction mnemonics that reveal the arithmetic precision actually emitted.
# x86-64 uses SSE scalar single/double, and x87 for 80-bit long double.
PRECISION_MARKERS = [
    ("single", r"\b(mulss|addss|subss|divss|cvtss2sd)\b"),
    ("double", r"\b(mulsd|addsd|subsd|divsd|cvtsd2ss)\b"),
    ("x87/ld", r"\b(fldt|fstpt|fmulp|faddp|fdivp)\b"),
]

# (label, A, B). A is the lower-precision side; B is the reference, and it
# supplies the static site universe for the diff.
PAIRS = [("fp32_vs_fp64", "fp32", "fp64"),
         ("fp64_vs_ld", "fp64", "ld")]

BIN = "lulesh.brx"

# [BranchTrace] lulesh.cc (mod 3838089238): instrumented 173 branch sites (fp-only)
SITE_RE = re.compile(
    r"^\[BranchTrace\]\s+(\S+)\s+\(mod\s+(\d+)\):\s+instrumented\s+(\d+)\s+branch sites")



# (regex, hint) applied to failing build output. These cover the failure modes
# that produce unhelpful or keyword-free messages.
BUILD_HINTS = [
    (r"Unknown command line argument '?-+brtrace-fp-only",
     "the plugin loaded but does not register -brtrace-fp-only, or it did not "
     "load at all.\n          Rebuild it against the active LLVM: "
     "cd <brx> && ./build_mtu.sh\n          Or run with --all-branches to drop "
     "the flag and see if the rest works."),
    (r"unable to load plugin|cannot open shared object|undefined symbol",
     "the plugin could not be loaded. It was almost certainly built against a "
     "different\n          LLVM than the active clang++. Check "
     "`llvm-config --version` against `clang++ --version`,\n          then "
     "rebuild: cd <brx> && ./build_mtu.sh"),
    (r"No such file or directory.*brtrace_runtime_mtu\.o",
     "the brtrace runtime object is missing -- run ./build_mtu.sh in the "
     "brtrace dir."),
    (r"long double|__float128",
     "a long-double construct failed to compile. Check the lulesh_ld tree "
     "builds standalone first."),
    (r"error: no member named|error: use of undeclared",
     "source-level error -- the variant tree may not be a clean precision-token "
     "derivation of fp64."),
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


def sh(cmd, cwd=None, env=None, logfile=None, dry=False):
    """Run a command, tee combined output to `logfile`, return (rc, text)."""
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
    log("      -> exit %d in %.1fs" % (p.returncode, dt))
    return p.returncode, p.stdout


def parse_site_counts(text):
    """module basename -> (module_id, site_count), from the pass banners."""
    out = {}
    for line in text.splitlines():
        m = SITE_RE.match(line.strip())
        if m:
            out[os.path.basename(m.group(1))] = (int(m.group(2)), int(m.group(3)))
    return out



# -------------------------------------------------------------------- probe

PROBE_SRC = """
// Two FP-controlled branches and one integer-controlled loop branch, so the
// site count differs measurably between fp-only and all-branches mode.
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
    """Compile a tiny file both ways and report the site counts.

    Two things go wrong silently and this catches both before an hour of
    LULESH builds: the plugin failing to register -brtrace-fp-only, and the
    flag being accepted but not actually filtering anything.
    """
    if dry:
        return
    import tempfile
    log("  [probe] verifying the pass runs and -brtrace-fp-only takes effect")
    with tempfile.TemporaryDirectory() as td:
        src = Path(td) / "probe.cc"
        src.write_text(PROBE_SRC)
        results = {}
        for mode, flags in (("all-branches",
                             "-%s -g -fpass-plugin=%s" % (opt, cfg["plugin"])),
                            ("fp-only", cfg["cflags"](opt))):
            if mode == "fp-only" and not fp_only:
                continue
            cmd = "%s %s -c probe.cc -o probe.o" % (cfg["cxx"], flags)
            p = subprocess.run(cmd, shell=True, cwd=td, text=True,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, errors="replace")
            counts = parse_site_counts(p.stdout)
            n = sum(c[1] for c in counts.values())
            results[mode] = (p.returncode, n, p.stdout)
            log("    %-14s exit %d, %d site(s)" % (mode, p.returncode, n))

        for mode, (rc, n, outp) in results.items():
            if rc != 0:
                log("    --- probe output (%s) ---" % mode)
                for line in outp.strip().splitlines()[-15:]:
                    log("    %s" % line[:200])
                log("    ---")
                for pat, hint in BUILD_HINTS:
                    if re.search(pat, outp, re.I):
                        log("    HINT: %s" % hint)
                die("probe failed in %s mode -- fix this before building "
                    "LULESH" % mode)
            if n == 0:
                die("probe compiled in %s mode but the pass emitted no "
                    "[BranchTrace] banner.\n       The plugin is not running. "
                    "Check that it was built against the\n       active LLVM: "
                    "cd %s && ./build_mtu.sh" % (mode, BRX_ROOT))

        if fp_only and "all-branches" in results:
            n_all = results["all-branches"][1]
            n_fp = results["fp-only"][1]
            if n_fp >= n_all:
                log("    WARNING: fp-only instrumented %d site(s), all-branches "
                    "%d." % (n_fp, n_all))
                log("             The flag is being accepted but is not "
                    "filtering. Every count")
                log("             would be over ALL branches despite the "
                    "caption saying otherwise.")
            else:
                log("    ok -- fp-only filters (%d -> %d sites on the probe)"
                    % (n_all, n_fp))
    log()


# -------------------------------------------------------------------- build

def build(variant, opt, cfg, outdir, dry=False):
    """Copy the source tree, compile it instrumented, return (binary, counts).

    Compiled with a single direct clang++ invocation rather than through the
    tree's Makefile. Matched-build discipline requires the three variants to
    differ ONLY in precision tokens; a per-tree Makefile flag difference would
    change branch structure and silently break site-id correspondence. Driving
    the compiler directly makes these flags authoritative for all three.
    """
    src = BENCH_ROOT / VARIANTS[variant]
    dst = BUILD_ROOT / opt / VARIANTS[variant]
    if not src.is_dir():
        die("no source tree at %s" % src)
    missing = [s for s in SRCS if not (src / s).exists()]
    if missing:
        die("%s is missing sources: %s" % (src, ", ".join(missing)))

    if not dry:
        dst.parent.mkdir(parents=True, exist_ok=True)
        if dst.exists():
            shutil.rmtree(dst)
        shutil.copytree(src, dst)
        # Stale objects and a prebuilt lulesh2.0 ship in these trees; drop them
        # so nothing uninstrumented can be picked up at link time.
        for junk in (list(dst.glob("*.o")) + list(dst.glob("lulesh2.0")) +
                     list(dst.glob("*.brsites")) + list(dst.glob("*.brmods"))):
            junk.unlink()

    define = cfg["defines"].get(variant, "")
    cmd = "%s %s %s %s %s %s -lm -o %s" % (
        cfg["cxx"], cfg["cflags"](opt), cfg["cxxflags"], define,
        " ".join(SRCS), cfg["runtime"], BIN)
    cmd = re.sub(r"\s+", " ", cmd)
    logf = outdir / "builds" / ("%s.build.log" % variant)
    log("  %s" % variant)
    rc, out = sh(cmd, cwd=dst, logfile=logf, dry=dry)
    if dry:
        return dst / BIN, {}

    if rc != 0 or not (dst / BIN).exists():
        # Show the actual output, not a guessed-at "error" line. LLVM option
        # parsing failures and plugin load failures do not contain the word
        # "error" at all, so a keyword scan silently prints nothing and hides
        # the only useful information.
        tail = out.strip().splitlines()
        log("    --- last %d line(s) of build output ---" % min(len(tail), 25))
        for line in tail[-25:]:
            log("    %s" % line[:200])
        log("    ---")
        for pat, hint in BUILD_HINTS:
            if re.search(pat, out, re.I):
                log("    HINT: %s" % hint)
        die("build failed for %s at -%s -- see %s" % (variant, opt, logf))

    # A clean build proves nothing about instrumentation; the banner does.
    counts = parse_site_counts(out)
    if not counts:
        die("no [BranchTrace] banner in the %s -%s build -- the pass did not "
            "run.\n       See %s. Usually -fpass-plugin did not take, or the "
            "plugin was\n       built against a different LLVM than %s."
            % (variant, opt, logf, cfg["cxx"]))

    total = sum(n for _, n in counts.values())
    log("      instrumented %d sites across %d TU(s)" % (total, len(counts)))
    for mod in sorted(counts):
        log("        %-20s mod %-11d %5d sites"
            % (mod, counts[mod][0], counts[mod][1]))

    (outdir / "builds" / ("%s.build_info.txt" % variant)).write_text(
        "variant     = %s\n"
        "opt         = -%s   (single compile+link invocation)\n"
        "cxx         = %s\n"
        "cflags      = %s\n"
        "cxxflags    = %s\n"
        "define      = %s\n"
        "runtime     = %s\n"
        "scope       = %s\n"
        "src         = %s\n"
        "build_dir   = %s\n"
        "sites_total = %d\n"
        "sites_by_tu = %s\n"
        "openmp      = disabled (no -fopenmp); the run is single-threaded,\n"
        "              which brtrace requires -- with >1 thread the\n"
        "              interleaving of __brtrace_log calls differs run to run\n"
        "              and the traces cannot be walked in lock-step.\n"
        % (variant, opt, cfg["cxx"], cfg["cflags"](opt), cfg["cxxflags"],
           define or "(none)", cfg["runtime"], cfg["scope"], src, dst, total,
           ", ".join("%s=%d" % (m, counts[m][1]) for m in sorted(counts))))
    return dst / BIN, counts


# --------------------------------------------------------- precision check

def precision_check(binaries, variants, allow, dry=False):
    """Disassemble each binary and count precision-revealing instructions.

    A matched-build check confirms the three variants have the SAME branch
    structure. It cannot confirm they have DIFFERENT arithmetic -- and if a
    precision-selecting define is missed, they will not. That failure is
    invisible in every other signal: builds succeed, site counts agree, traces
    are the same length, and both diffs report zero flips, which reads as a
    clean result rather than a broken experiment.

    So: check the emitted instructions directly. fp32 must contain scalar
    single-precision arithmetic; ld must contain x87; and no two variants may
    have identical profiles.
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
        p = subprocess.run("objdump -d %s" % shlex.quote(str(b)), shell=True,
                           text=True, stdout=subprocess.PIPE,
                           stderr=subprocess.DEVNULL, errors="replace")
        prof = tuple(len(re.findall(pat, p.stdout))
                     for _, pat in PRECISION_MARKERS)
        profiles[v] = prof
        log("    %-5s %s"
            % (v, "  ".join("%s=%-7d" % (PRECISION_MARKERS[i][0], prof[i])
                            for i in range(len(prof)))))

    problems = []
    for a in profiles:
        for b in profiles:
            if a < b and profiles[a] == profiles[b]:
                problems.append("%s and %s emit identical arithmetic "
                                "instruction profiles" % (a, b))
    if "fp32" in profiles and profiles["fp32"][0] == 0:
        problems.append("fp32 emits NO single-precision arithmetic -- the "
                        "precision define did not take")
    if "ld" in profiles and profiles["ld"][2] == 0:
        problems.append("ld emits NO x87 instructions -- it is not actually "
                        "long double")

    if problems:
        msg = ("the variants are not at different precisions:\n       - "
               + "\n       - ".join(problems)
               + "\n\n       Every diff would report zero flips, which looks "
                 "like a clean result\n       rather than a broken build. "
                 "Check the precision-selecting macro in\n       each tree's "
                 "lulesh.h and set it with --define, e.g.\n"
                 "           --define fp32=-DLULESH_FP32 ld=-DLULESH_LD")
        if allow:
            sys.stderr.write("WARNING: %s\n" % msg)
        else:
            die(msg)
    else:
        log("    ok -- all variants emit distinct arithmetic")
    log()


# ------------------------------------------------------- matched-build check

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
        ok = len(vals) == 1
        log("    %-20s %s   %s"
            % (mod, "  ".join("%6s" % r for r in row),
               "ok" if ok else "MISMATCH"))
        if not ok:
            bad.append(mod)
    log("    (columns: %s)" % ", ".join(variants))
    if bad:
        msg = ("per-module site counts differ across variants for: %s.\n"
               "       Site ids will not correspond, so every diff would be "
               "garbage.\n       Usual causes: a variant whose source diverges "
               "in branch structure\n       (not just precision tokens), or "
               "-brtrace-fp-only applied\n       inconsistently across builds."
               % ", ".join(bad))
        if allow:
            sys.stderr.write("WARNING: %s\n" % msg)
        else:
            die(msg)
    else:
        log("    all modules agree -- site ids correspond across variants")


# ---------------------------------------------------------------- summarise

def extract_headline(text):
    """Pull the numbers worth putting in a cross-pair summary.

    Anchored regexes, not substring scans: the diff's stdout also carries a
    legacy line "lock-step compared to event N" that a loose match grabs in
    preference to the report's "lock-step compared  N events (P% ...)".
    """
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


# Site lines in the diff report's SITE-BASED section look like
#     lulesh.cc:2079  [EvalEOSForElems]   (412 flips of 12,345 executions)
# or, under --fast where the execution census is skipped,
#     lulesh.cc:2079  [EvalEOSForElems]   (412 flips)
SITE_LINE_RE = re.compile(
    r"^\s+(\S.*?)\s+\(([\d,]+) flips"
    r"(?: of ([\d,]+) executions)?"
    r"(?:, first @ event#([\d,]+))?\)\s*$")


def extract_sites(text):
    """(location, flips, executions_or_None) for each flipping site."""
    out = []
    for line in text.splitlines():
        m = SITE_LINE_RE.match(line)
        if m:
            raw = m.group(1)
            short, func, foreign = pretty_site(raw)
            out.append({"raw": raw, "loc": short, "func": func,
                        "foreign": foreign,
                        "flips": int(m.group(2).replace(",", "")),
                        "executions": (int(m.group(3).replace(",", ""))
                                       if m.group(3) else None),
                        "first": (int(m.group(4).replace(",", ""))
                                  if m.group(4) else None)})
    return out


DIVNOTE_RE = re.compile(
    r"^\s*NOTE: the last site to start flipping.*?(?=\n\s*\n|\n\s*TP\s)",
    re.S | re.M)


def extract_divergence_note(text):
    """The report's causal note, if the diff emitted one.

    It only appears when the last site to begin flipping did so within a
    couple of events of the divergence -- i.e. when there is a defensible
    causal claim rather than a coincidence.
    """
    m = DIVNOTE_RE.search(text)
    if not m:
        return []
    return [ln.strip() for ln in m.group(0).strip().splitlines() if ln.strip()]


def flips_at_loc(rec):
    """The one-line headline: 'N flips @ M loc [in-TU a, STL b]'."""
    sites = rec["sites"]
    total = sum(x["flips"] for x in sites)
    stl = sum(x["flips"] for x in sites if x["foreign"])
    tail = ("   [in-TU %d, STL %d]" % (total - stl, stl)) if stl else ""
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
        bits += "   [STL]"
    return bits


# ------------------------------------------------------ location prettifying
#
# brtrace records the file exactly as the compiler saw it, so an STL site
# arrives as a 200-character conda include path with a mangled symbol. Shorten
# the path and demangle the symbol, and flag STL/system sites separately --
# they are real flips, but they are not in the benchmark's own source, and the
# distinction matters when comparing against a source-level tool.
#
# (This is also the collision FPChecker cannot resolve: it reports std::max as
# lulesh.cc:263 -- correct line, wrong file -- because it reads DILocation.line
# without walking getInlinedAt(). brtrace records the location before inlining,
# so it names stl_algobase.h correctly.)

def _scan_depth(text):
    """Yield (index, char, depth) with depth counting <> and () nesting."""
    depth = 0
    for i, c in enumerate(text):
        if c in "<(":
            depth += 1
        elif c in ">)":
            depth -= 1
        yield i, c, depth


def _strip_arglist(name):
    """Drop the trailing (...) argument list, ignoring nested parens."""
    start = None
    for i, c, depth in _scan_depth(name):
        if c == "(" and depth == 1:
            start = i
            break
    return name[:start].strip() if start is not None else name.strip()


def _strip_return_type(name):
    """Drop a leading return type.

    Split on the last space at nesting depth 0, so template arguments that
    themselves contain spaces -- std::max<double, std::less<double> > -- do not
    fool it. 'double const& std::max<double>' -> 'std::max<double>'.
    """
    cut = -1
    for i, c, depth in _scan_depth(name):
        if c == " " and depth == 0:
            cut = i
    return name[cut + 1:] if cut >= 0 else name


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
        out = _strip_arglist(out)
        out = _strip_return_type(out)
        _DEMANGLE_CACHE[sym] = out or sym
    return _DEMANGLE_CACHE[sym]


FOREIGN_RE = re.compile(r"\b(std|__gnu_cxx|__cxxabiv1)::")
SYSPATH_RE = re.compile(r"/(include/c\+\+|bits|usr/include|gcc)/")


def pretty_site(raw):
    """'<long path>:263  [_ZSt3maxIdERKT_S2_S2_]' ->
       ('stl_algobase.h:263', 'std::max<double>', True)"""
    m = re.match(r"^(.*?):(\d+)\s*(?:\[(.*)\])?\s*$", raw.strip())
    if not m:
        return raw, "", False
    path, line, sym = m.group(1), m.group(2), m.group(3) or ""
    func = demangle(sym)
    foreign = bool(FOREIGN_RE.search(func)) or bool(SYSPATH_RE.search(path))
    return "%s:%s" % (os.path.basename(path), line), func, foreign


def write_results(opt, args, cfg, pair_records, variant_info, outdir):
    lines = ["LULESH -- brtrace dual-trajectory ground-truth census",
             "(-%s compile+link, single-threaded, s=%d, i=%d)"
             % (opt, args.size, args.iter),
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
        if r["sites"]:
            if any(x["foreign"] for x in r["sites"]):
                lines.append("")
                lines.append("    [STL] sites are real flips in system "
                             "headers, not in the benchmark's")
                lines.append("    own source. FPChecker reports these against "
                             "the including TU (e.g.")
                lines.append("    lulesh.cc:263 for stl_algobase.h:263), so "
                             "adjudicate on file+symbol.")
        lines.append("    report: %s" % r["report"])
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
        {"opt": opt, "size": args.size, "iterations": args.iter,
         "scope": cfg["scope"], "variants": variant_info,
         "pairs": pair_records}, indent=2, default=str))
    return "\n".join(lines)


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

    # --- build
    if args.skip_build:
        log("  [build] skipped (--skip-build)")
        for v in variants:
            binaries[v] = BUILD_ROOT / opt / VARIANTS[v] / BIN
    else:
        log("  [build]")
        for v in variants:
            b, c = build(v, opt, cfg, outdir, dry=dry)
            binaries[v] = b
            if c:
                counts_by_variant[v] = c
                variant_info[v] = {
                    "sites_total": sum(n for _, n in c.values()),
                    "sites_by_tu": {m: c[m][1] for m in c}}
        log()
        check_matched(counts_by_variant, variants, args.allow_site_mismatch, dry)
        log()
        precision_check(binaries, variants, args.allow_same_precision, dry)

    # --- run
    traces = {v: outdir / "traces" / ("lulesh_%s.out" % v) for v in variants}
    if args.skip_run:
        log("  [run] skipped (--skip-run)")
        for v in variants:
            if not dry and traces[v].exists():
                sz = traces[v].stat().st_size
                variant_info.setdefault(v, {}).update(
                    {"trace": str(traces[v]), "bytes": sz, "events": sz // 12})
    else:
        log("  [run]")
        for v in variants:
            binp = Path(binaries[v])
            if not dry and not binp.exists():
                die("%s not found -- build it first (drop --skip-build)" % binp)
            env = os.environ.copy()
            # Mandatory: the diff walks both traces in lock-step, which only
            # works if event order is deterministic.
            env["OMP_NUM_THREADS"] = "1"
            env["BRTRACE_OUT"] = str(traces[v])
            cmd = "%s./%s -s %d -i %d" % (
                (args.launcher + " ") if args.launcher else "",
                BIN, args.size, args.iter)
            log("  %s -> %s" % (v, traces[v]))
            runlog = outdir / "builds" / ("%s.run.log" % v)
            rc, _ = sh(cmd, cwd=binp.parent, env=env, logfile=runlog, dry=dry)
            if not dry:
                if rc != 0:
                    die("run failed for %s at -%s (exit %d) -- see %s"
                        % (v, opt, rc, runlog))
                sz = traces[v].stat().st_size
                log("      %s  (%s events)"
                    % (human(sz), "{:,}".format(sz // 12)))
                variant_info.setdefault(v, {}).update(
                    {"trace": str(traces[v]), "bytes": sz, "events": sz // 12})
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
        # The reference side supplies the static site universe. Its .brsites
        # files sit in its BUILD dir, written there at compile time.
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
        if not dry and rc in (0, 1):
            log("      -> %s" % flips_at_loc({"sites": extract_sites(out)}))
        # rc==1 means flips and/or divergence were found, which is the expected
        # outcome for fp32-vs-fp64. Only a crash is an error.
        if rc not in (0, 1) and not dry:
            tail = out.strip().splitlines()
            log("    --- last %d line(s) of diff output ---"
                % min(len(tail), 20))
            for line in tail[-20:]:
                log("    %s" % line[:200])
            log("    ---")
            if rc == 2:
                log("    HINT: exit 2 is argparse rejecting an argument -- "
                    "usually an older")
                log("          brtrace_diff_mtu.py without --report/--fast.")
            die("diff failed for %s (exit %d) -- see %s"
                % (label, rc, pdir / "diff.log"))
        if not dry:
            pair_records.append({
                "label": label, "a": a, "b": b, "opt": opt,
                "report": str(report), "csv": str(csv), "exit": rc,
                "headline": extract_headline(out),
                "sites": extract_sites(out),
                "divergence_note": extract_divergence_note(out)})
    log()

    if dry:
        return None, counts_by_variant
    text = write_results(opt, args, cfg, pair_records, variant_info, outdir)
    log(text)
    log()
    return pair_records, counts_by_variant


# --------------------------------------------------------- cross-opt check

def cross_opt_check(all_counts, order):
    """The pass runs at PipelineStartEP, before any optimisation, so static
    site counts should be identical across levels. If they are not, the pass is
    seeing different IR than expected and the levels are not comparable."""
    levels = [o for o in order if o in all_counts]
    if len(levels) < 2:
        return
    log("=" * 74)
    log("cross-optimisation site check")
    log("=" * 74)
    mods = set()
    for o in levels:
        for v in all_counts[o]:
            mods |= set(all_counts[o][v])
    differing = []
    for mod in sorted(mods):
        row, vals = [], set()
        for o in levels:
            per = {all_counts[o][v][mod][1]
                   for v in all_counts[o] if mod in all_counts[o][v]}
            val = per.pop() if len(per) == 1 else None
            row.append("?" if val is None else str(val))
            if val is not None:
                vals.add(val)
        same = len(vals) == 1
        log("  %-20s %s   %s"
            % (mod, "  ".join("%6s" % r for r in row),
               "same" if same else "DIFFERS"))
        if not same:
            differing.append(mod)
    log("  (columns: %s)" % ", ".join(levels))
    if differing:
        log("  NOTE: site counts differ across levels for %s."
            % ", ".join(differing))
        log("        Expected identical, since the pass runs at "
            "PipelineStartEP.")
        log("        Site-level results are NOT comparable across levels.")
    else:
        log("  identical across levels -- site-level results are directly")
        log("  comparable. Event counts still are not: -O2 may unroll,")
        log("  duplicating a site's calls, or delete dead paths.")
    log()


# -------------------------------------------------------------------- main

def main():
    global BRX_ROOT, BENCH_ROOT, WORK_ROOT, BUILD_ROOT, RESULT_ROOT
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--opt", nargs="+", default=["O0", "O2"],
                    choices=["O0", "O1", "O2", "O3"],
                    help="optimisation level(s); each gets its own subtree "
                         "(default: O0 O2)")
    ap.add_argument("-s", "--size", type=int, default=5,
                    help="LULESH -s (default 5, matching "
                         "run_lulesh_fpchecker.py; the published census "
                         "used 10)")
    ap.add_argument("-i", "--iter", type=int, default=20,
                    help="LULESH -i (default 20, matching "
                         "run_lulesh_fpchecker.py; the published census "
                         "used 50)")
    ap.add_argument("--brx", default=str(BRX_ROOT),
                    help="brtrace dir (plugin, runtime, tools/)")
    ap.add_argument("--bench", default=str(BENCH_ROOT),
                    help="dir holding lulesh_fp32/ lulesh_fp64/ lulesh_ld/")
    ap.add_argument("--work", default=str(WORK_ROOT),
                    help="where build/ and results/ go "
                         "(default: <script dir>/%s)" % BENCH_NAME)
    ap.add_argument("--cxx", default="clang++")
    ap.add_argument("--cxxflags", default="-DUSE_MPI=0 -I. -std=c++11")
    ap.add_argument("--define", nargs="*", default=[], metavar="VAR=FLAGS",
                    help="extra compile flags per variant, e.g. "
                         "fp32=-DLULESH_FP32 ld=-DLULESH_LD "
                         "(default: %s)"
                         % " ".join("%s=%s" % (k, v or "''")
                                    for k, v in DEFINES.items()))
    ap.add_argument("--allow-same-precision", action="store_true",
                    help="proceed even if two variants emit identical "
                         "arithmetic (the diff will be meaningless -- don't)")
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
    ap.add_argument("--fast", action="store_true",
                    help="pass --fast to the diff (skips the per-site "
                         "execution census; TN counts become unavailable)")
    ap.add_argument("--progress", type=int, default=25,
                    help="diff progress every N million events (0=off)")
    ap.add_argument("--skip-probe", action="store_true",
                    help="skip the pre-build plugin probe")
    ap.add_argument("--skip-build", action="store_true")
    ap.add_argument("--skip-run", action="store_true")
    ap.add_argument("--allow-site-mismatch", action="store_true",
                    help="proceed even if per-module site counts differ "
                         "(the diff will be meaningless -- don't)")
    ap.add_argument("--dry-run", action="store_true",
                    help="print every command without executing anything")
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
            parts.append("       That directory does not exist. Pass --brx "
                         "<your brtrace dir>,")
            parts.append("       or set BRX_ROOT in the environment.")
        elif plugin in missing or runtime in missing:
            parts.append("       Build them:")
            parts.append("           cd %s && ./build_mtu.sh" % BRX_ROOT)
            parts.append("       (needs llvm-config on PATH -- activate the "
                         "conda env first)")
        else:
            parts.append("       Pass --brx <your brtrace dir>, or set "
                         "BRX_ROOT.")
        parts.append("")
        parts.append("       Searched, in order: $BRX_ROOT, <script dir>/"
                     "brtrace, <script dir>/../brtrace,")
        parts.append("       branch_flip/brtrace, "
                     "error_analysis/brtrace.")
        die("\n".join(parts))

    # The driver passes --report/--fast, which only exist in the updated diff
    # tool. An older copy accepts none of them and exits 2 (argparse) AFTER
    # every build and run has completed, which is an expensive way to find out.
    need = ["--report"] + (["--fast"] if args.fast else [])
    try:
        helptext = subprocess.run("python3 %s --help" % shlex.quote(str(diff)),
                                  shell=True, text=True, timeout=60,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT).stdout
    except Exception as e:
        die("could not run the diff tool: %s\n       %s" % (diff, e))
    absent = [f for f in need if f not in helptext]
    if absent:
        die("the diff tool at\n         %s\n"
            "       does not support: %s\n\n"
            "       That is the pre-TP/TN version. Copy the updated\n"
            "       brtrace_diff_mtu.py into %s\n"
            "       and rerun. Nothing else in the brtrace tree needs to "
            "change --\n       the pass, runtime and .so are untouched, so "
            "existing traces stay valid."
            % (diff, ", ".join(absent), diff.parent))

    fp_only = not args.all_branches

    def cflags(opt):
        # -brtrace-fp-only is a cl::opt living inside the plugin, so it only
        # exists once the plugin's shared object has been dlopened.
        #
        # -fpass-plugin alone is NOT enough: clang parses -mllvm arguments in
        # ExecuteCompilerInvocation (cl::ParseCommandLineOptions), but does not
        # load -fpass-plugin libraries until the backend runs, much later. The
        # option therefore does not exist yet at parse time and clang dies with
        #     Unknown command line argument '-brtrace-fp-only'
        # ...Did you mean '--dot-mcfg-only'?
        #
        # -Xclang -load -Xclang <so> goes through the FrontendOpts::Plugins
        # path, which IS dlopened before -mllvm parsing, so the cl::opt is
        # registered in time. Both flags are needed: -load registers the
        # option, -fpass-plugin actually schedules the pass. Same path in both,
        # so dlopen returns one handle and the option registers once.
        #
        # It MUST be identical across variants or the site numbering diverges.
        base = "-%s -g -fpass-plugin=%s" % (opt, plugin)
        if fp_only:
            base += " -Xclang -load -Xclang %s -mllvm -brtrace-fp-only" % plugin
        return base

    defines = dict(DEFINES)
    for spec in args.define:
        if "=" not in spec:
            die("--define needs VAR=FLAGS, got %r" % spec)
        k, v = spec.split("=", 1)
        if k not in VARIANTS:
            die("--define: unknown variant %r (known: %s)"
                % (k, ", ".join(VARIANTS)))
        defines[k] = v

    cfg = {"cxx": args.cxx, "cxxflags": args.cxxflags, "cflags": cflags,
           "defines": defines,
           "runtime": str(runtime), "diff": diff, "plugin": str(plugin),
           "scope": ("FP-CONTROLLED branches (-brtrace-fp-only)" if fp_only
                     else "ALL conditional branches")}

    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)

    log("LULESH / brtrace   s=%d i=%d   opt: %s"
        % (args.size, args.iter, " ".join(args.opt)))
    log("  scope:    %s" % cfg["scope"])
    log("  brx:      %s" % BRX_ROOT)
    log("  bench:    %s" % BENCH_ROOT)
    log("  work:     %s" % WORK_ROOT)
    log("  variants: %s" % ", ".join(
        "%s%s" % (v, (" [%s]" % defines[v]) if defines.get(v) else "")
        for v in args.variants))
    log("  pairs:    %s" % ", ".join(args.pairs))
    if dry:
        log("  MODE:     DRY RUN -- nothing will be executed")
    log()

    if not (args.skip_probe or args.skip_build):
        probe_plugin(cfg, args.opt[0], fp_only, dry=dry)

    all_pairs, all_counts = {}, {}
    for opt in args.opt:
        recs, counts = run_one_opt(opt, args, cfg, dry=dry)
        if recs is not None:
            all_pairs[opt] = recs
        if counts:
            all_counts[opt] = counts

    cross_opt_check(all_counts, args.opt)

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
            log("  -%-3s %-14s  %-18s TP sites %-5s TN sites %-6s DEAD %-4s"
                % (opt, r["label"], flips_at_loc(r),
                   head.get("TP sites", "?"), head.get("TN sites", "?"),
                   head.get("DEAD sites", "?")))
            for x in r["sites"]:
                log("        " + site_line(x))
    log("\nresults under %s/<opt>/" % RESULT_ROOT)
    return 0


if __name__ == "__main__":
    sys.exit(main())