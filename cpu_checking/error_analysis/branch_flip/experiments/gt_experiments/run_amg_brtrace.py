#!/usr/bin/env python3
"""
run_amg_brtrace.py

Build the three precision variants of AMG under the brtrace pass, run each
once, and diff two pairs to produce the ground-truth branch-flip census:

    fp32 vs fp64      does single precision flip branches relative to double?
    fp64 vs ld        is double itself oracle-stable against long double?

fp64 is built and run ONCE per optimisation level and its trace feeds both
diffs, so this costs three builds and three runs per level, not four.

Place in: branch_flip/experiments/gt_experiments/

    ./run_amg_brtrace.py                      # -O0 (see --opt)
    ./run_amg_brtrace.py --opt O0 O2          # both levels
    ./run_amg_brtrace.py -n 3                 # smaller/faster
    ./run_amg_brtrace.py --dry-run            # print every command, run none
    ./run_amg_brtrace.py --skip-build --skip-run   # re-diff existing traces
    ./run_amg_brtrace.py --no-brx-build       # keep the existing plugin .so

The brtrace plugin and runtime are CLEANED AND REBUILT on every run, before
anything else happens, and llvm-config is checked against the compiler first.
They are two compiler invocations and take seconds; a stale
libBranchTrace_mtu.so costs a whole census, because it satisfies every
existence check and then instruments according to whatever the pass source
said at the last build. --skip-build implies --no-brx-build: compiling nothing
means the .brsites already on disk came from the previous pass, so replacing
that pass would make the side tables and the binaries disagree.

CENSUS CONFIG: problem 2, n=5, matching run_amg_fpchecker.py. Problem 1
(AMG-PCG on a 27-point Laplace) shows zero fp32/fp64 iteration divergence at
any size tested. Problem 2 is GMRES on a modified-diagonal system, where
Gram-Schmidt is precision sensitive and fp32 needs roughly twice the
iterations of fp64.

Layout produced (one subtree per optimisation level):

    amg/
      results/
        O0/  builds/   fp32.build.log  fp32.run.log  fp32.build_info.txt ...
             traces/   amg_fp32.out  amg_fp64.out  amg_ld.out
             fp32_vs_fp64/  report.txt  flips.csv  sites.txt  diff.log
             fp64_vs_ld/    report.txt  flips.csv  sites.txt  diff.log
             summary.txt  summary.json
        O2/  ...
      build/
        O0/amg_fp32/  O0/amg_fp64/  O0/amg_ld/  O2/...

sites.txt is the per-site census -- one row per static site, classed TP / TN /
DEAD -- and is the file adjudicate_cell.py scores tool output against.
report.txt is for reading; sites.txt is for scoring. Do not generate it under
--fast: that skips the execution census, so TN and DEAD collapse together and
the adjudicator refuses the file.

DIFFERENCES FROM THE LULESH BRTRACE HARNESS
  * AMG is C, so the frontend is clang, not clang++, and there is no name
    mangling to undo.
  * The build goes through make (CC / INCLUDE_CFLAGS / INCLUDE_LFLAGS) because
    AMG spans many directories. Command-line assignments propagate to the
    sub-makes, so they stay authoritative over Makefile.include.
  * .brsites/.brmods land next to each module, i.e. scattered across the build
    subdirectories. --mods therefore points at the build tree ROOT; the diff
    globs for them recursively.
  * BASENAME COLLISION CHECK (AMG-specific, and the reason this matters here
    and not for LULESH). The pass derives module_id from the FNV hash of the
    source BASENAME, deliberately, so that amg_fp32/foo.c and amg_fp64/foo.c
    align. But AMG is multi-directory: if two .c files in different
    subdirectories share a basename, they collide into one module_id, both
    number site_id from zero, and their sites alias. This script scans for
    that before building, because nothing downstream would notice.
  * Three gates carried over from run_amg_fpchecker.py, because AMG fails
    SILENTLY in ways LULESH does not:
      1. sizeof(HYPRE_Real) probe -- a clean build does not prove the
         -DHYPRE_SINGLE / -DHYPRE_LONG_DOUBLE flag reached every TU.
      2. iterations != 0 -- upstream's sequential MPI stubs have no
         FLOAT/LONG_DOUBLE case and no default, so at non-double precision
         Allreduce falls through, never writes recvbuf, and returns success.
         Symptom: "Iterations = 0, Residual = 0.000000e+00, FOM = -nan" from a
         binary that built and ran cleanly.
      3. objdump precision check -- fp32 must emit single-precision
         arithmetic, ld must emit x87, and no two variants may match. Without
         this, a missed precision flag yields three identical trajectories,
         zero flips, and a result that reads as "fp32 is stable".
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
BENCH_ROOT = Path(os.environ.get("BENCH_ROOT", BRANCH_FLIP / "benchmarks/amg"))


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
BENCH_NAME = "amg"
WORK_ROOT = HERE / BENCH_NAME
BUILD_ROOT = WORK_ROOT / "build"
RESULT_ROOT = WORK_ROOT / "results"

VARIANTS = {"fp32": "amg_fp32", "fp64": "amg_fp64", "ld": "amg_ld"}
PRECISION_FLAG = {"fp32": "-DHYPRE_SINGLE", "fp64": "",
                  "ld": "-DHYPRE_LONG_DOUBLE"}
EXPECTED_WIDTH = {"fp32": 4, "fp64": 8, "ld": 16}

PAIRS = [("fp32_vs_fp64", "fp32", "fp64"),
         ("fp64_vs_ld", "fp64", "ld")]

BIN_REL = Path("test") / "amg"

SITE_RE = re.compile(
    r"^\[BranchTrace\]\s+(\S+)\s+\(mod\s+(\d+)\):\s+instrumented\s+(\d+)\s+branch sites")
ITER_RE = re.compile(r"Iterations\s*=\s*(\d+)")
RESID_RE = re.compile(r"Final Relative Residual Norm\s*=\s*([0-9.eE+-]+)")

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
    (r"HYPRE_utilities\.h: No such file",
     "include path wrong -- check the -I in --cflags matches the tree layout."),
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

    module_id is FNV of the BASENAME (deliberately, so amg_fp32/x.c and
    amg_fp64/x.c align). In a multi-directory build that means two distinct
    sources with the same basename share a module_id, both number site_id from
    zero, and their sites alias into each other. Nothing downstream detects
    it: the builds succeed, counts agree, the diff runs.
    """
    by_base = defaultdict(list)
    for p in Path(src).rglob("*.c"):
        by_base[p.name].append(p.relative_to(src))
    dupes = {b: v for b, v in by_base.items() if len(v) > 1}
    if not dupes:
        log("    no basename collisions across %d .c file(s)" % len(by_base))
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


# ------------------------------------------- AMG-specific: HYPRE_Real width

def width_check(tree, variant, cc, dry=False):
    """Compile a probe against this tree's headers and report
    sizeof(HYPRE_Real). A clean AMG build does not prove the precision flag
    reached the headers -- that failure is silent."""
    if dry:
        return None
    probe = Path(tree) / "_width_probe.c"
    probe.write_text('#include <stdio.h>\n#include "HYPRE_utilities.h"\n'
                     'int main(void){ printf("%zu\\n", sizeof(HYPRE_Real)); '
                     'return 0; }\n')
    out_bin = Path(tree) / "_width_probe"
    flag = PRECISION_FLAG[variant]
    cmd = ("%s -O0 -DHYPRE_SEQUENTIAL=1 %s -I%s/utilities -I%s %s -o %s"
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
  int i;
  for (i = 0; i < 3; i++) {          /* integer-controlled */
    if (a > b) s += a; else s += b;  /* fp-controlled */
  }
  if (a == b) s += 1.0;              /* fp-controlled */
  return s;
}
int main(void) { return (int)g(1.0, 2.0); }
"""


def probe_plugin(cfg, opt, fp_only, dry=False):
    """Compile a tiny file both ways: does the pass run, and does
    -brtrace-fp-only actually filter? Seconds here beats an hour of AMG."""
    if dry:
        return
    import tempfile
    log("  [probe] verifying the pass runs and -brtrace-fp-only takes effect")
    with tempfile.TemporaryDirectory() as td:
        (Path(td) / "probe.c").write_text(PROBE_SRC)
        results = {}
        modes = [("all-branches",
                  "-%s -g -fpass-plugin=%s" % (opt, cfg["plugin"]))]
        if fp_only:
            modes.append(("fp-only", cfg["cflags"](opt)))
        for mode, flags in modes:
            rc, out = sh("%s %s -c probe.c -o probe.o" % (cfg["cc"], flags),
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
                    "test/amg"):
            for junk in dst.glob(pat):
                if junk.is_file():
                    junk.unlink()

    flag = PRECISION_FLAG[variant]
    cflags = ("%s %s %s" % (cfg["cflags"](opt), cfg["cflags_extra"], flag)).strip()
    cflags = re.sub(r"\s+", " ", cflags)
    lflags = "-lm %s" % cfg["runtime"]
    cmd = ("make -j%d CC=%s INCLUDE_CFLAGS=%s INCLUDE_LFLAGS=%s"
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
    log("      sizeof(HYPRE_Real) = %s (expected %d)%s"
        % (width, exp, "" if width == exp else "   *** MISMATCH"))

    (outdir / "builds" / ("%s.build_info.txt" % variant)).write_text(
        "variant        = %s\n"
        "opt            = -%s   (compile AND link)\n"
        "CC             = %s\n"
        "INCLUDE_CFLAGS = %s\n"
        "INCLUDE_LFLAGS = %s\n"
        "precision flag = %s\n"
        "scope          = %s\n"
        "src            = %s\n"
        "build_dir      = %s\n"
        "sites_total    = %d across %d TU(s)\n"
        "HYPRE_Real     = %s bytes (expected %d)\n"
        "openmp         = disabled (no -fopenmp); the run is single-threaded,\n"
        "                 which brtrace requires -- with >1 thread the\n"
        "                 interleaving of __brtrace_log calls differs run to\n"
        "                 run and traces cannot be walked in lock-step.\n"
        % (variant, opt, cfg["cc"], cflags, lflags, flag or "(none, default)",
           cfg["scope"], src, dst, total, len(counts), width, exp))

    if width != exp:
        die("sizeof(HYPRE_Real) is %s for %s, expected %d -- the precision "
            "flag did not\n       reach the headers. Every trajectory would be "
            "at the same precision and\n       the diff would report zero "
            "flips." % (width, variant, exp))
    return binary, counts


# ----------------------------------------------------------- brtrace build

def _llvm_version(cmd):
    """Version string from a tool's --version output, or None."""
    _, out = sh("%s --version" % cmd, quiet=True)
    m = re.search(r"(\d+\.\d+\.\d+)", out or "")
    return m.group(1) if m else None


def check_llvm_match(cc):
    """The plugin must be built against the same LLVM the compiler runs.

    A mismatch is the most expensive failure here: the plugin builds cleanly,
    then clang dlopens it and either dies with an undefined symbol or segfaults
    partway through the build. Both read as a broken benchmark rather than a
    broken toolchain, so check before spending the time.
    """
    lv = _llvm_version("llvm-config")
    cv = _llvm_version(shlex.quote(cc))
    log("    llvm-config %s   %s %s" % (lv or "?", cc, cv or "?"))
    if lv is None:
        die("llvm-config is not on PATH -- activate the conda env "
            "(fpchecker_env) first.")
    if cv is None:
        log("    WARNING: could not read a version from %s; skipping the "
            "match check" % cc)
        return
    if lv != cv:
        die("LLVM version mismatch: llvm-config is %s but %s is %s.\n"
            "       The plugin would be built against %s headers and then "
            "loaded by a\n       %s driver, which segfaults or fails with "
            "undefined symbols mid-build.\n"
            "       Activate the conda env whose clang matches llvm-config, "
            "or point\n       --cc at the matching compiler."
            % (lv, cc, cv, lv, cv))


def build_brx(brx, cc, dry=False):
    """Clean and rebuild libBranchTrace_mtu.so + brtrace_runtime_mtu.o.

    Always a clean rebuild. The artifacts are two compiler invocations and take
    seconds, whereas a stale .so is undetectable: the existence check passes,
    the pass loads, and it instruments according to whatever the source looked
    like at the last build.

    Removing the artifacts BEFORE building matters too: if the build fails
    partway, a leftover .so from the previous build would still satisfy the
    existence check downstream.
    """
    script = brx / "build_mtu.sh"
    plugin = brx / "libBranchTrace_mtu.so"
    runtime = brx / "brtrace_runtime_mtu.o"

    log("  [brtrace build]")
    if not script.exists():
        die("no build_mtu.sh at %s\n"
            "       Pass --brx <your brtrace dir>, or --no-brx-build if you "
            "have built\n       the plugin and runtime some other way."
            % script)

    check_llvm_match(cc)

    if not dry:
        for f in (plugin, runtime):
            if f.exists():
                f.unlink()
                log("    removed stale %s" % f.name)

    env = os.environ.copy()
    # Keep the plugin, the runtime and AMG on one toolchain. The runtime object
    # is linked into the instrumented binary, so an ABI difference between it
    # and AMG shows up at link time. AMG is C and --cc is a C driver, so derive
    # the C++ driver the plugin needs.
    env["CC"] = cc
    env["CXX"] = cc + "++" if cc.endswith("clang") else "clang++"
    rc, out = sh("bash build_mtu.sh", cwd=brx, env=env,
                 logfile=brx / "build_mtu.log", dry=dry)
    if dry:
        return
    if rc != 0:
        for line in (out or "").strip().splitlines()[-20:]:
            log("    %s" % line[:200])
        die("brtrace build failed -- see %s" % (brx / "build_mtu.log"))
    for f in (plugin, runtime):
        if not f.exists():
            die("build_mtu.sh exited 0 but did not produce %s -- see %s"
                % (f.name, brx / "build_mtu.log"))
    log("    built %s, %s" % (plugin.name, runtime.name))
    log()


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
    lines = ["AMG -- brtrace dual-trajectory ground-truth census",
             "(-%s compile+link, single-threaded, problem %d, n=%d^3)"
             % (opt, args.problem, args.n),
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
    lines.append("solver outcome")
    for v in sorted(variant_info):
        info = variant_info[v]
        if "iterations" in info:
            lines.append("    %-5s iterations=%-6s residual=%s"
                         % (v, info["iterations"], info.get("residual")))
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
        {"opt": opt, "problem": args.problem, "n": args.n,
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
    traces = {v: outdir / "traces" / ("amg_%s.out" % v) for v in variants}
    if args.skip_run:
        log("  [run] skipped (--skip-run)")
        for v in variants:
            if not dry and traces[v].exists():
                sz = traces[v].stat().st_size
                variant_info.setdefault(v, {}).update(
                    {"trace": str(traces[v]), "bytes": sz, "events": sz // 12})
    else:
        log("  [run]")
        runargs = ("-problem %d -n %d %d %d -P 1 1 1"
                   % (args.problem, args.n, args.n, args.n))
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
            it = ITER_RE.search(out)
            rs = RESID_RE.search(out)
            iters = int(it.group(1)) if it else None
            sz = traces[v].stat().st_size
            log("      %s  (%s events)   iterations=%s  residual=%s"
                % (human(sz), "{:,}".format(sz // 12), iters,
                   rs.group(1) if rs else None))
            variant_info.setdefault(v, {}).update(
                {"trace": str(traces[v]), "bytes": sz, "events": sz // 12,
                 "iterations": iters,
                 "residual": rs.group(1) if rs else None})
            # Gate: the mpistubs FLOAT/LONG_DOUBLE fall-through returns success
            # having never written recvbuf, so the solver "runs" and produces
            # nothing. Iterations = 0 is the only visible symptom.
            if iters == 0:
                die("%s produced Iterations = 0 -- the solver returned no "
                    "result.\n       This is the mpistubs FLOAT/LONG_DOUBLE "
                    "fall-through: at non-double\n       precision Allreduce "
                    "has no case and no default, so it never writes\n       "
                    "recvbuf and still returns success. The tree is unpatched "
                    "or the patch\n       regressed. See %s" % (v, runlog))
            if iters is None:
                log("      WARNING: no 'Iterations =' line found -- check %s"
                    % runlog)
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
        # The per-site census. adjudicate_cell.py scores tool output against
        # this file, not against report.txt -- it needs the TP/TN/DEAD class
        # per site, which only --sites-txt emits. Without it every downstream
        # cell reports "no census" after all the builds and runs have
        # completed, which is an expensive way to find out.
        sites_txt = pdir / "sites.txt"
        # The reference side supplies the static site universe. AMG scatters
        # .brsites across build subdirectories, so point at the tree root and
        # let the diff glob recursively.
        mods = BUILD_ROOT / opt / VARIANTS[b]
        cmd = ("python3 %s %s %s --mods %s --csv %s --report %s --sites-txt %s"
               % (shlex.quote(str(cfg["diff"])), shlex.quote(str(traces[a])),
                  shlex.quote(str(traces[b])), shlex.quote(str(mods)),
                  shlex.quote(str(csv)), shlex.quote(str(report)),
                  shlex.quote(str(sites_txt))))
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
                "report": str(report), "csv": str(csv),
                "sites_txt": str(sites_txt), "exit": rc,
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
    ap.add_argument("--problem", type=int, default=2,
                    help="1 = AMG-PCG Laplace (no fp32/fp64 divergence at any "
                         "size tested), 2 = GMRES modified diagonal (default)")
    ap.add_argument("-n", type=int, default=5,
                    help="grid points per axis (default 5, matching "
                         "run_amg_fpchecker.py; 3 also diverges)")
    ap.add_argument("--brx", default=str(BRX_ROOT))
    ap.add_argument("--bench", default=str(BENCH_ROOT),
                    help="dir holding amg_fp32/ amg_fp64/ amg_ld/")
    ap.add_argument("--work", default=str(WORK_ROOT))
    ap.add_argument("--cc", default="clang")
    ap.add_argument("--probe-cc", default="clang",
                    help="plain compiler for the HYPRE_Real width probe")
    ap.add_argument("--cflags-extra", default="-DHYPRE_SEQUENTIAL=1 -I../utilities",
                    help="extra compile flags (default matches "
                         "run_amg_fpchecker.py)")
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
    ap.add_argument("--no-brx-build", action="store_true",
                    help="do not rebuild the brtrace plugin and runtime. By "
                         "default they are cleaned and rebuilt every run, "
                         "because a stale .so passes every existence check and "
                         "silently instruments the wrong thing.")
    ap.add_argument("--skip-probe", action="store_true")
    ap.add_argument("--skip-build", action="store_true")
    ap.add_argument("--skip-run", action="store_true")
    ap.add_argument("--allow-site-mismatch", action="store_true")
    ap.add_argument("--allow-same-precision", action="store_true")
    ap.add_argument("--allow-basename-collision", action="store_true",
                    help="proceed even if two .c files share a basename "
                         "(sites would alias -- don't)")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    BRX_ROOT = Path(args.brx).resolve()
    BENCH_ROOT = Path(args.bench).resolve()
    WORK_ROOT = Path(args.work).resolve()
    BUILD_ROOT = WORK_ROOT / "build"
    RESULT_ROOT = WORK_ROOT / "results"
    dry = args.dry_run

    # --fast skips the per-site execution census, so sites.txt comes back with
    # executions=0 everywhere and every non-flipping site is written as DEAD.
    # adjudicate_cell.py cannot tell DEAD from TN in that file and refuses to
    # score it. Say so now rather than after three builds and three runs.
    if args.fast:
        sys.stderr.write(
            "WARNING: --fast skips the execution census. sites.txt will mark\n"
            "         every non-flipping site DEAD, and adjudicate_cell.py\n"
            "         will refuse it. Use --fast only for a quick look at\n"
            "         flip counts, never to produce a scoreable census.\n")

    # Rebuild the pass and runtime by default, before anything else touches
    # them. Skipped under --skip-build, which means "compile nothing" -- and
    # rebuilding the plugin there would be actively wrong, since the .brsites
    # already on disk came from whatever pass built the benchmark.
    brx_build_scheduled = not (args.no_brx_build or args.skip_build)
    if args.no_brx_build:
        log("  [brtrace build] skipped (--no-brx-build)")
        log()
    elif args.skip_build:
        log("  [brtrace build] skipped (--skip-build: compiling nothing, and "
            "the")
        log("                  .brsites on disk came from the previous pass)")
        log()
    else:
        build_brx(BRX_ROOT, args.cc, dry=dry)

    plugin = BRX_ROOT / "libBranchTrace_mtu.so"
    runtime = BRX_ROOT / "brtrace_runtime_mtu.o"
    diff = BRX_ROOT / "tools" / "brtrace_diff_mtu.py"
    check = [plugin, runtime, diff]
    if dry and brx_build_scheduled:
        # A dry run executes nothing, so the build above did not actually
        # produce these. Complaining that they are absent would be complaining
        # about the dry run itself.
        check = [diff]
    missing = [f for f in check if not f.exists()]
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

    log("AMG / brtrace   problem %d  n=%d^3   opt: %s"
        % (args.problem, args.n, " ".join(args.opt)))
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

    if args.problem == 1:
        log("  NOTE: problem 1 shows no fp32/fp64 iteration divergence at any")
        log("        size tested; problem 2 is the census target.")
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