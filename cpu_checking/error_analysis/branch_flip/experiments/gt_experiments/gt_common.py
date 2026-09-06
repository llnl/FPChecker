"""gt_common.py -- shared pieces of the run_*_brtrace.py census harnesses.

Environment (all overridable): FPC_ROOT (repo root), BRX_ROOT (brtrace tool
dir, default <this dir>/brtrace), GT_WORK_ROOT (where build/ and results/ go,
default this dir), BENCH_ROOT per harness.

Per pair the harness writes, under <bench>/results/<opt>/<pair>/:
    <a>.out, <b>.out       12-byte brtrace records
    report.txt             diff tool report (coverage line is parsed by scorers)
    flips.csv              one row per flipped execution (occ_index k)
    window.csv             per-site adjudication window (E_S); scoring universe
    sites.txt              per-site TP/TN/DEAD table
    build_info.txt, summary.json
"""

import json
import os
import re
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
FPC_ROOT = Path(os.environ["FPC_ROOT"]) if os.environ.get("FPC_ROOT") \
    else HERE.parents[4]
BRX_ROOT = Path(os.environ.get("BRX_ROOT", HERE / "brtrace"))
BENCH_BASE = FPC_ROOT / "cpu_checking/error_analysis/branch_flip/benchmarks"
WORK_BASE = Path(os.environ.get("GT_WORK_ROOT", HERE))

PLUGIN = BRX_ROOT / "libBranchTrace_mtu.so"
RUNTIME = BRX_ROOT / "brtrace_runtime_mtu.o"
DIFF = BRX_ROOT / "tools" / "brtrace_diff_mtu.py"

PRECISIONS = ["fp32", "fp64", "ld"]
PAIRS = {"fp32_vs_fp64": ("fp32", "fp64"), "fp64_vs_ld": ("fp64", "ld")}

BANNER_RE = re.compile(
    r"^\[BranchTrace\]\s+(\S+)\s+\(mod\s+(\d+)\):\s+instrumented\s+(\d+)\s+"
    r"branch sites(?:,\s+(\d+)\s+select sites)?")
FLIPS_RE = re.compile(r"FLIP EVENTS:\s+(\d+)\s+across\s+(\d+)\s+distinct sites")
SITE_LINE_RE = re.compile(r"^\s+mod\s+(\d+)\s+site\s+(\d+)\s+x(\d+)\s+(.*)$")
DIVERGE_RE = re.compile(r"CONTROL-FLOW DIVERGENCE at event#(\d+)")
COVERAGE_RE = re.compile(r"lock-step compared\s+([\d,]+)\s+events\s+"
                         r"\(([\d.]+)% of longer trace\)")


def sh(cmd, cwd=None, env=None, log=None, timeout=None):
    try:
        p = subprocess.run(cmd, cwd=cwd, env=env, shell=isinstance(cmd, str),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True, errors="replace", timeout=timeout)
        rc, out = p.returncode, p.stdout
    except subprocess.TimeoutExpired as e:
        rc, out = 124, (e.output or "") + "\n[TIMEOUT]"
    if log:
        Path(log).write_text(out)
    return rc, out


def rel(p):
    try:
        return str(Path(p).relative_to(HERE))
    except ValueError:
        return str(p)


def die(msg):
    raise SystemExit("FATAL: " + msg)


def ensure_brtrace(rebuild=False):
    """Locate (or build) the brtrace plugin and runtime."""
    if not BRX_ROOT.is_dir():
        die(f"no brtrace dir at {BRX_ROOT} (set BRX_ROOT)")
    if rebuild or not (PLUGIN.exists() and RUNTIME.exists()):
        script = BRX_ROOT / "build_mtu.sh"
        if not script.exists():
            die(f"no build_mtu.sh at {BRX_ROOT}")
        print("  building brtrace")
        rc, _ = sh("bash build_mtu.sh", cwd=BRX_ROOT,
                   log=BRX_ROOT / "build_mtu.log")
        if rc != 0 or not (PLUGIN.exists() and RUNTIME.exists()):
            die(f"brtrace build failed -- see {BRX_ROOT/'build_mtu.log'}")
    if not DIFF.exists():
        die(f"no diff tool at {DIFF}")


def brx_cflags(opt, fp_only=True):
    f = f"-{opt} -g -fpass-plugin={PLUGIN}"
    if fp_only:
        f += f" -Xclang -load -Xclang {PLUGIN} -mllvm -brtrace-fp-only"
    return f


def add_common_args(ap):
    ap.add_argument("--opt", default="O0", choices=["O0", "O1", "O2", "O3"],
                    help="compile level; -O0 is the only level at which site "
                         "ids correspond across builds")
    ap.add_argument("--all-branches", action="store_true",
                    help="instrument every conditional branch, not only "
                         "FP-controlled ones")
    ap.add_argument("--rebuild-brtrace", action="store_true")
    ap.add_argument("-j", "--jobs", type=int, default=1)


def parse_banners(text):
    """{module_basename: (module_id, branch_sites, select_sites)}"""
    out = {}
    for line in text.splitlines():
        m = BANNER_RE.match(line.strip())
        if m:
            out[os.path.basename(m.group(1))] = (
                int(m.group(2)), int(m.group(3)),
                int(m.group(4)) if m.group(4) else 0)
    return out


def collect_side_tables(src_dir, dst_dir, recursive=False):
    """Copy .brsites/.brselsites/.brmods next to the results."""
    dst_dir.mkdir(parents=True, exist_ok=True)
    n = 0
    pat = "**/*.br*" if recursive else "*.br*"
    for p in src_dir.glob(pat):
        if p.suffix in (".brsites", ".brselsites", ".brmods"):
            shutil.copy2(p, dst_dir / p.name)
            n += 1
    return n


def check_site_agreement(banners, label):
    """Every build of a pair must enumerate the same sites per module."""
    ok = True
    mods = sorted(set().union(*[set(b) for b in banners.values()]))
    for mod in mods:
        counts = {p: banners[p].get(mod, (None, None, None))[1]
                  for p in banners}
        if len(set(counts.values())) > 1:
            print(f"  *** {label}: site count differs for {mod}: {counts}")
            ok = False
    return ok


def run_diff(a_out, b_out, mods_dir, outdir, kind="branch", timeout=None):
    """Run the diff tool for one stream and parse its report."""
    if kind == "select":
        outdir = outdir / "select"
        outdir.mkdir(parents=True, exist_ok=True)
    report = outdir / "report.txt"
    flips = outdir / "flips.csv"
    window = outdir / "window.csv"
    sites = outdir / "sites.txt"
    cmd = ["python3", str(DIFF), str(a_out), str(b_out), "--kind", kind,
           "--mods", str(mods_dir), "--csv", str(flips),
           "--window-csv", str(window), "--sites-txt", str(sites),
           "--report", str(report), "--max-report", "20"]
    rc, out = sh(cmd, timeout=timeout)
    (outdir / "diff.log").write_text(out)

    res = {"kind": kind, "flip_events": 0, "flip_sites": 0, "sites": {},
           "divergence_event": None, "coverage": None, "adjudicated": None,
           "diff_rc": rc}
    m = FLIPS_RE.search(out)
    if m:
        res["flip_events"], res["flip_sites"] = int(m.group(1)), int(m.group(2))
    for line in out.splitlines():
        s = SITE_LINE_RE.match(line)
        if s:
            res["sites"][s.group(4).strip()] = int(s.group(3))
    m = DIVERGE_RE.search(out)
    if m:
        res["divergence_event"] = int(m.group(1))
    m = COVERAGE_RE.search(out)
    if m:
        res["adjudicated"] = int(m.group(1).replace(",", ""))
        res["coverage"] = float(m.group(2))
    for name, p in (("report", report), ("flips_csv", flips),
                    ("window_csv", window), ("sites_txt", sites)):
        res[name] = rel(p) if p.exists() else None
    return res


def run_traced(binary, argv, out_path, cwd=None, env=None, log=None,
               timeout=None):
    """Run one instrumented binary, tracing both streams."""
    env = dict(env or os.environ)
    env["BRTRACE_OUT"] = str(out_path)
    env["BRTRACE_SEL_OUT"] = str(out_path).replace(".out", "_sel.out")
    env["OMP_NUM_THREADS"] = "1"
    rc, out = sh([str(binary)] + list(argv), cwd=cwd, env=env, log=log,
                 timeout=timeout)
    for p in (Path(env["BRTRACE_OUT"]), Path(env["BRTRACE_SEL_OUT"])):
        if not p.exists():
            p.touch()
    return rc, out


PRECISION_MARKERS = [
    ("single", r"\b(mulss|addss|subss|divss|cvtss2sd)\b"),
    ("double", r"\b(mulsd|addsd|subsd|divsd|cvtsd2ss)\b"),
    ("x87/ld", r"\b(fldt|fstpt|fmulp|faddp|fdivp)\b"),
]


def arith_profile(binary):
    """(single, double, x87) instruction counts, or None without objdump."""
    if not shutil.which("objdump"):
        return None
    _, out = sh(f"objdump -d {binary}")
    return tuple(len(re.findall(p, out)) for _, p in PRECISION_MARKERS)


def check_precisions(profiles, label):
    """Two variants with identical arithmetic profiles were built at the same
    precision; the pair would compare a program with itself."""
    seen = {}
    ok = True
    for v, prof in profiles.items():
        if prof is None:
            continue
        if prof in seen:
            print(f"  *** {label}: {v} and {seen[prof]} have identical "
                  f"arithmetic profiles {prof}")
            ok = False
        seen[prof] = v
    return ok


def print_pair(label, res, sel=None):
    cov = f"{res['coverage']:.2f}%" if res["coverage"] is not None else "n/a"
    div = (f"  divergence @ {res['divergence_event']:,}"
           if res["divergence_event"] is not None else "")
    line = (f"  {label:<14s} {res['flip_events']:8d} flips @ "
            f"{res['flip_sites']:3d} sites   coverage {cov}{div}")
    if sel:
        line += f"   [select: {sel['flip_events']} @ {sel['flip_sites']}]"
    print(line)


def write_summary(outdir, rec):
    (outdir / "summary.json").write_text(json.dumps(rec, indent=2))
    L = [rec.get("title", ""), ""]
    for pair, r in rec.get("pairs", {}).items():
        b = r["branch"]
        cov = f"{b['coverage']:.2f}%" if b["coverage"] is not None else "n/a"
        L.append(f"{pair}: {b['flip_events']} flips @ {b['flip_sites']} sites, "
                 f"coverage {cov}, adjudicated {b['adjudicated']}")
        if b["divergence_event"] is not None:
            L.append(f"    divergence at event {b['divergence_event']}")
        for loc, n in list(b["sites"].items())[:20]:
            L.append(f"    {n:9d}  {loc}")
        s = r.get("select")
        if s:
            L.append(f"    select stream: {s['flip_events']} flips @ "
                     f"{s['flip_sites']} sites")
        L.append("")
    (outdir / "summary.txt").write_text("\n".join(L))