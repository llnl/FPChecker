"""fpc_common.py -- shared pieces of the run_*_fpchecker.py harnesses.

Environment (all overridable): FPC_ROOT (repo root), FPC_INSTALL
(FPC_ROOT/install), FPC_WORK_ROOT (where build/ and results/ go; default is
the harness directory).

Compile-time knobs the harness exports: FPC_BRANCH_FLIP=1 (branch-flip build:
no SimplifyCFG, site banner on), FPC_OPT_LEVEL, and
-DFPC_SHADOW_FALLBACK_NATIVE=0 when --shadow-fallback zero.

Run-time knobs: FPC_STABILITY_ETA_REL, FPC_BF_MODE, FPC_NONFINITE_POLICY,
FPC_STABILITY_MAX_WARNINGS / FPC_NONFINITE_MAX_WARNINGS (--maxwarn, 0 =
unlimited), FPC_BF_LOG.

Per run the harness keeps: run_<tag>.log (stdout), events_<tag>.log
(#FPC_EVENT / #FPC_SITE / exit summary; split per rule in --bf-mode both),
branch_flip_<tag>.json (the runtime's own JSON), and summary.json.
"""

import json
import os
import re
import shutil
import subprocess
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
FPC_ROOT = Path(os.environ["FPC_ROOT"]) if os.environ.get("FPC_ROOT") \
    else HERE.parents[4]
FPC_INSTALL = Path(os.environ.get("FPC_INSTALL", FPC_ROOT / "install"))
BENCH_BASE = FPC_ROOT / "cpu_checking/error_analysis/branch_flip/benchmarks"
WORK_BASE = Path(os.environ.get("FPC_WORK_ROOT", HERE))
CONDA_LIB = os.environ.get("CONDA_PREFIX", "") and \
    os.path.join(os.environ["CONDA_PREFIX"], "lib")

DEFAULT_ETA = {
    "fp32": ["1e-2", "1e-6", "1e-10"],
    "fp64": ["1e-8", "1e-14", "1e-16"],
}

RULE_VERDICTS = {
    "interval": {"flag": ("UNSTABLE",), "abstain": "DECLINED"},
    "shadow":   {"flag": ("SFLIP", "SFLIPNF"), "abstain": None},
}

FLIP_RE = re.compile(r"Unstable branch", re.I)
SHADOW_FLIP_RE = re.compile(r"Shadow branch flip", re.I)
EVENT_RE = re.compile(
    r"^#FPC_EVENT\s+mod=(\d+)\s+site=(-?\d+)\s+k=(\d+)\s+verdict=(\w+)"
    r"(?:\s+at\s+(\S+):(\d+))?(?:\s+in\s+(\S+))?(?:\s+cause=(\S+))?")
SITE_DUMP_RE = re.compile(
    r"^#FPC_SITE\s+(\d+)\s+(-?\d+)\s+(\d+)\s+(\d+)\s+(\d+)")
SITE_DUMP_S_RE = re.compile(
    r"^#FPC_SITE_S\s+(\d+)\s+(-?\d+)\s+(\d+)\s+(\d+)")
BF_RULE_BANNER_RE = re.compile(r"^#FPCHECKER: Branch-flip rule=(\w+)")
BF_SUMMARY_RE = re.compile(
    r"^#FPCHECKER: branch-flip summary: unstable=(\d+) nonfinite=(\d+)")
NF_CAUSE_RE = re.compile(r"^#FPCHECKER: nonfinite causes:(.*)$")
SHADOW_MISS_RE = re.compile(
    r"^#FPCHECKER: shadow misses: load=(\d+) store=(\d+) ret=(\d+) "
    r"arg=(\d+) phi=(\d+)")
SHADOW_SUMMARY_RE = re.compile(
    r"^#FPCHECKER: shadow-rule summary: sflip=(\d+) sflip_nonfinite=(\d+)")
BANNER_RE = re.compile(
    r"^\[FPChecker\]\s+(\S+)\s+\(mod\s+(\d+)\):\s+(\d+)\s+"
    r"FP-controlled branch sites")
TAB_OVERFLOW_RE = re.compile(r"^#FPC_SITES WARNING")
BF_JSON_RE = re.compile(r"Writing branch-flip JSON to: (\S+)")

_DEMANGLE_CACHE = {}


def rel(p):
    """Path relative to the harness dir when possible, else absolute."""
    try:
        return str(Path(p).relative_to(HERE))
    except ValueError:
        return str(p)


FOREIGN_RE = re.compile(r"\b(std|__gnu_cxx|__cxxabiv1)::")


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


def base_env():
    env = os.environ.copy()
    if CONDA_LIB:
        env["LD_LIBRARY_PATH"] = CONDA_LIB + ":" + env.get("LD_LIBRARY_PATH", "")
    return env


def make_env(precision, opt):
    """Compile/link environment. Both instrumentation vars are cleared first
    so only one pass runs; FPC_BRANCH_FLIP selects the branch-flip build."""
    env = base_env()
    env["FPC_OPT_LEVEL"] = opt
    env["FPC_BRANCH_FLIP"] = "1"
    env.pop("FPC_INSTRUMENT_ERR_TRACKING", None)
    env.pop("FPC_INSTRUMENT_ERR_TRACKING_FP64", None)
    if precision == "fp64":
        env["FPC_INSTRUMENT_ERR_TRACKING_FP64"] = "1"
    else:
        env["FPC_INSTRUMENT_ERR_TRACKING"] = "1"
    return env


def instr_var(precision):
    return ("FPC_INSTRUMENT_ERR_TRACKING_FP64" if precision == "fp64"
            else "FPC_INSTRUMENT_ERR_TRACKING")


def fallback_flag(shadow_fallback):
    return "" if shadow_fallback == "native" else "-DFPC_SHADOW_FALLBACK_NATIVE=0"


def run_env(eta, maxwarn, bf_mode, nonfinite_policy, bflog):
    env = base_env()
    env["FPC_STABILITY_ETA_REL"] = eta
    env["FPC_STABILITY_MAX_WARNINGS"] = str(maxwarn)
    env["FPC_NONFINITE_MAX_WARNINGS"] = str(maxwarn)
    env["FPC_BF_MODE"] = bf_mode
    env["FPC_NONFINITE_POLICY"] = nonfinite_policy
    env["FPC_BF_LOG"] = str(bflog)
    return env


def result_dir(result_root, opt, precision, args):
    suffix = ""
    if args.bf_mode != "interval":
        suffix += "_rule-" + args.bf_mode
    if args.nonfinite_policy != "abstain":
        suffix += "_nonfinite-" + args.nonfinite_policy
    if args.shadow_fallback != "native":
        suffix += "_fallback-" + args.shadow_fallback
    d = result_root / opt / (precision + suffix)
    d.mkdir(parents=True, exist_ok=True)
    return d


def etas_for(args, precisions):
    etas = {k: list(DEFAULT_ETA.get(k, DEFAULT_ETA["fp64"])) for k in precisions}
    if args.eta:
        etas = {k: list(args.eta) for k in etas}
    if getattr(args, "eta_fp32", None) and "fp32" in etas:
        etas["fp32"] = list(args.eta_fp32)
    if getattr(args, "eta_fp64", None) and "fp64" in etas:
        etas["fp64"] = list(args.eta_fp64)
    if args.bf_mode == "shadow":
        etas = {k: v[:1] for k, v in etas.items()}
    return etas


def add_common_args(ap):
    ap.add_argument("-e", "--eta", nargs="+", default=None,
                    help="override eta for all precisions")
    ap.add_argument("--eta-fp32", nargs="+", default=None)
    ap.add_argument("--eta-fp64", nargs="+", default=None)
    ap.add_argument("--opt", default="O0", choices=["O0", "O1", "O2", "O3"],
                    help="compile and link level; must match the oracle's")
    ap.add_argument("--bf-mode", choices=["interval", "shadow", "both"],
                    default="interval",
                    help="decision rule: interval (eta window, can abstain), "
                         "shadow (shadow-predicate disagreement, no abstention), or both")
    ap.add_argument("--maxwarn", type=int, default=0,
                    help="cap on printed warning lines; 0 = unlimited. "
                         "Event records are never capped.")
    ap.add_argument("--nonfinite-policy", choices=["abstain", "unstable"],
                    default="abstain",
                    help="interval rule on non-finite shadow: abstain "
                         "(DECLINED) or classify UNSTABLE")
    ap.add_argument("--shadow-fallback", choices=["native", "zero"],
                    default="native",
                    help="value used when shadow state is missing "
                         "(compile-time FPC_SHADOW_FALLBACK_NATIVE)")
    ap.add_argument("-j", "--jobs", type=int, default=1)


# ---------------------------------------------------------------- parsing

def parse_banners(text):
    """module basename -> (module_id, site_count)"""
    out = {}
    for line in text.splitlines():
        m = BANNER_RE.match(line.strip())
        if m:
            out[os.path.basename(m.group(1))] = (int(m.group(2)),
                                                 int(m.group(3)))
    return out


def parse_events(text):
    events, totals, totals_s, oos, overflow = [], {}, {}, 0, False
    summary = {}
    for line in text.splitlines():
        m = EVENT_RE.match(line)
        if m:
            mod, site, k = int(m.group(1)), int(m.group(2)), int(m.group(3))
            if site < 0:
                oos += 1
            loc = None
            if m.group(5):
                loc = m.group(5) + (":" + m.group(6) if m.group(6) else "")
            events.append((mod, site, k, m.group(4), loc, m.group(7),
                           m.group(8)))
            continue
        m = SITE_DUMP_S_RE.match(line)
        if m:
            totals_s[(int(m.group(1)), int(m.group(2)))] = (
                int(m.group(3)), int(m.group(4)))
            continue
        m = SITE_DUMP_RE.match(line)
        if m:
            totals[(int(m.group(1)), int(m.group(2)))] = (
                int(m.group(3)), int(m.group(4)), int(m.group(5)))
            continue
        if TAB_OVERFLOW_RE.match(line):
            overflow = True
            continue
        m = BF_SUMMARY_RE.match(line)
        if m:
            summary["unstable"] = int(m.group(1))
            summary["nonfinite"] = int(m.group(2))
            continue
        m = NF_CAUSE_RE.match(line)
        if m:
            summary["nonfinite_causes"] = {
                kv.split("=")[0]: int(kv.split("=")[1])
                for kv in m.group(1).split() if "=" in kv}
            continue
        m = SHADOW_MISS_RE.match(line)
        if m:
            summary["shadow_miss"] = dict(zip(
                ("load", "store", "ret", "arg", "phi"), map(int, m.groups())))
            continue
        m = SHADOW_SUMMARY_RE.match(line)
        if m:
            summary["sflip"] = int(m.group(1))
            summary["sflip_nonfinite"] = int(m.group(2))
    return {"events": events, "totals": totals, "totals_s": totals_s,
            "out_of_scope": oos, "overflow": overflow, "summary": summary}


def demangle(sym):
    if not sym:
        return ""
    if sym not in _DEMANGLE_CACHE:
        try:
            out = subprocess.run(["c++filt", sym], stdout=subprocess.PIPE,
                                 text=True, timeout=10).stdout.strip()
        except Exception:
            out = sym
        out = re.sub(r"\s*\(.*\)\s*$", "", out)
        _DEMANGLE_CACHE[sym] = out or sym
    return _DEMANGLE_CACHE[sym]


def tally(events, verdicts, cxx=False):
    """Per-rule flag/abstain counts keyed by location."""
    sites, foreign, abstain_sites = Counter(), Counter(), Counter()
    n_flag = n_abstain = n_scoreable = n_nf = 0
    for mod, site, k, verdict, loc, func, cause in events:
        if verdict not in verdicts["flag"] and verdict != verdicts["abstain"]:
            continue
        fn = demangle(func) if (cxx and func) else (func or "")
        key = loc if loc else f"mod{mod}:site{site}"
        if fn:
            key += f"  [{fn}]"
        if verdict == verdicts["abstain"]:
            n_abstain += 1
            abstain_sites[key] += 1
            continue
        n_flag += 1
        if verdict == "SFLIPNF":
            n_nf += 1
        sites[key] += 1
        if site >= 0:
            n_scoreable += 1
        if cxx and FOREIGN_RE.search(fn):
            foreign[key] += 1
    return {"flips": n_flag, "flips_nonfinite": n_nf,
            "scoreable_flips": n_scoreable, "locations": len(sites),
            "sites": dict(sites.most_common()),
            "foreign_sites": dict(foreign),
            "foreign_flips": sum(foreign.values()),
            "abstained": n_abstain,
            "abstained_sites": dict(abstain_sites.most_common())}


# ---------------------------------------------------------------- build gates

def _defines(nm_out, sym):
    n = 0
    for l in nm_out.splitlines():
        parts = l.split()
        if len(parts) >= 2 and parts[-1] == sym and parts[-2] not in "Uw":
            n += 1
    return n


def check_binary(binary, build_log_text, outdir, expected_sites=None,
                 indent="  "):
    """Instrumentation gates. Returns (nsym, banners, site_total) or None."""
    _, nm_out = sh(["nm", str(binary)])
    nsym = sum(1 for l in nm_out.splitlines() if "_FPC" in l)
    banners = parse_banners(build_log_text)
    site_total = sum(n for _, n in banners.values())
    if not banners:
        print(f"{indent}*** no [FPChecker] site banner in the build log")
        return None
    ntab = _defines(nm_out, "_FPC_SITE_TAB_")
    nmode = _defines(nm_out, "_FPC_BF_MODE_")
    print(f"{indent}_FPC symbols={nsym}  branch sites={site_total} across "
          f"{len(banners)} TU(s)  site_tables={ntab}  bf_mode_syms={nmode}")
    if expected_sites is not None and site_total != expected_sites:
        print(f"{indent}*** expected {expected_sites} branch sites, got "
              f"{site_total}; run check_sites.py before scoring")
    if ntab > 1:
        print(f"{indent}*** {ntab} site-counter tables in one binary")
        return None
    if nmode != 1:
        print(f"{indent}*** _FPC_BF_MODE_ instances = {nmode} (must be 1)")
        return None
    if nsym == 0:
        print(f"{indent}*** UNINSTRUMENTED BINARY")
        return None
    return nsym, banners, site_total, ntab, nmode


def collect_manifests(dst, outdir, recursive=False):
    manifests = sorted(dst.glob("**/*.fpcsites" if recursive else "*.fpcsites"))
    mdir = outdir / "fpcsites"
    if mdir.exists():
        shutil.rmtree(mdir)
    if manifests:
        mdir.mkdir(parents=True, exist_ok=True)
        for m in manifests:
            shutil.copy2(m, mdir / m.name)
    return len(manifests)


# ---------------------------------------------------------------- run

def split_events(bflog, bf_mode):
    """In --bf-mode both, split the combined log into one file per rule."""
    if bf_mode != "both" or not bflog.exists():
        return {}
    text = bflog.read_text().splitlines()
    keep = {
        "interval": {"verdicts": set(RULE_VERDICTS["interval"]["flag"])
                                 | {RULE_VERDICTS["interval"]["abstain"]},
                     "site_tag": "#FPC_SITE ", "sites_tag": "#FPC_SITES ",
                     "summary": ("#FPCHECKER: branch-flip summary:",
                                 "#FPCHECKER: NOTE:",
                                 "#FPCHECKER: nonfinite causes:",
                                 "#FPCHECKER: shadow misses:"),
                     "prose": ("#FPCHECKER: Nonfinite branch",)},
        "shadow":   {"verdicts": set(RULE_VERDICTS["shadow"]["flag"]),
                     "site_tag": "#FPC_SITE_S ", "sites_tag": "#FPC_SITES_S ",
                     "summary": ("#FPCHECKER: shadow-rule summary:",),
                     "prose": ("#FPCHECKER: Shadow branch flip",)},
    }
    out = {}
    for rule, k in keep.items():
        path = bflog.with_name(bflog.stem + f".{rule}.log")
        lines = [f"# rule={rule}"]
        for l in text:
            if l.startswith("#FPC_EVENT "):
                m = EVENT_RE.match(l)
                if m and m.group(4) in k["verdicts"]:
                    lines.append(l)
            elif l.startswith(k["site_tag"]) or l.startswith(k["sites_tag"]):
                lines.append(l.replace("#FPC_SITE_S ", "#FPC_SITE ", 1)
                              .replace("#FPC_SITES_S ", "#FPC_SITES ", 1))
            elif l.startswith(k["summary"]) or l.startswith(k["prose"]):
                lines.append(l)
            elif l.startswith("#FPC_SITES total=") or \
                    l.startswith("#FPC_SITES WARNING"):
                lines.append(l)
        path.write_text("\n".join(lines) + "\n")
        out[rule] = path
    bflog.unlink()
    return out


def run_instrumented(binary, argv, eta, args, outdir, tag, cxx=False,
                     timeout=None):
    """Run one configuration and return the record. Benchmark-specific fields
    are added by the caller."""
    log = outdir / f"run_{tag}.log"
    bflog = outdir / f"events_{tag}.log"
    env = run_env(eta, args.maxwarn, args.bf_mode, args.nonfinite_policy, bflog)
    logs = binary.parent / ".fpc_logs"
    shutil.rmtree(logs, ignore_errors=True)

    rc, out = sh(["stdbuf", "-i0", "-o0", "-e0", str(binary)] + list(argv),
                 cwd=str(binary.parent), env=env, log=log, timeout=timeout)

    banner_rule = None
    for line in out.splitlines():
        m = BF_RULE_BANNER_RE.match(line)
        if m:
            banner_rule = m.group(1)
            break

    bf_json = None
    m = BF_JSON_RE.search(out)
    src = None
    if m:
        cand = Path(m.group(1))
        src = cand if cand.is_absolute() else binary.parent / cand
    if src is None or not src.exists():
        found = sorted(logs.glob("branch_flip_*.json")) if logs.exists() else []
        src = found[-1] if found else None
    if src is not None and src.exists():
        dst = outdir / f"branch_flip_{tag}.json"
        shutil.move(str(src), dst)
        try:
            bf_json = json.load(open(dst))
        except Exception:
            bf_json = None

    bftext = bflog.read_text() if bflog.exists() else ""
    parsed = parse_events(bftext)
    events = parsed["events"]
    site_totals, site_totals_s = parsed["totals"], parsed["totals_s"]
    rt = parsed["summary"]

    rec = {
        "eta": eta if args.bf_mode != "shadow" else None,
        "opt": args.opt,
        "bf_mode": args.bf_mode,
        "nonfinite_policy": args.nonfinite_policy,
        "shadow_fallback": args.shadow_fallback,
        "maxwarn": args.maxwarn,
        "out_of_scope_events": parsed["out_of_scope"],
        "site_table_overflow": parsed["overflow"],
        "executed_sites": len(site_totals),
        "total_executions": sum(v[0] for v in site_totals.values()),
        "exit_code": rc,
        "log": rel(log),
        "event_log_by_rule": {r: rel(p)
                              for r, p in split_events(bflog, args.bf_mode).items()},
        "branch_flip_json": (rel(outdir / f"branch_flip_{tag}.json")
                             if bf_json else None),
        "runtime_summary": rt,
        "runtime_rule": banner_rule,
        "consistency": [],
    }
    if not rec["event_log_by_rule"]:
        rec["event_log"] = rel(bflog)

    if banner_rule is None:
        rec["consistency"].append("no 'Branch-flip rule=' banner in the run log")
    elif banner_rule != args.bf_mode:
        rec["consistency"].append(
            f"asked for rule={args.bf_mode}, runtime selected {banner_rule}")
    if parsed["overflow"]:
        rec["consistency"].append("site table overflow; k is not reliable")

    for rule in ("interval", "shadow"):
        if rule == "interval" and args.bf_mode not in ("interval", "both"):
            continue
        if rule == "shadow" and args.bf_mode not in ("shadow", "both"):
            continue
        rec[rule] = tally(events, RULE_VERDICTS[rule], cxx)

    if site_totals_s:
        bad = [k for k in site_totals if k in site_totals_s
               and site_totals[k][0] != site_totals_s[k][0]]
        missing = set(site_totals) ^ set(site_totals_s)
        if bad or missing:
            rec["consistency"].append(
                f"#FPC_SITE and #FPC_SITE_S disagree on executions "
                f"({len(bad)} mismatched, {len(missing)} in only one)")

    if "shadow" in rec and rt.get("sflip") is not None \
            and rt["sflip"] != rec["shadow"]["flips"]:
        rec["consistency"].append(
            f"runtime sflip={rt['sflip']} but log has "
            f"{rec['shadow']['flips']}")
    if "interval" in rec and rt.get("unstable") is not None \
            and rt["unstable"] != rec["interval"]["flips"]:
        rec["consistency"].append(
            f"runtime unstable={rt['unstable']} but log has "
            f"{rec['interval']['flips']}")
    if bf_json:
        c = bf_json.get("counts", {})
        if "interval" in rec and c.get("unstable") is not None \
                and c["unstable"] != rec["interval"]["flips"]:
            rec["consistency"].append(
                f"branch_flip json unstable={c['unstable']} but log has "
                f"{rec['interval']['flips']}")
        if "shadow" in rec and c.get("shadow_flip") is not None \
                and c["shadow_flip"] != rec["shadow"]["flips"]:
            rec["consistency"].append(
                f"branch_flip json shadow_flip={c['shadow_flip']} but log "
                f"has {rec['shadow']['flips']}")
        rec["nonfinite_causes"] = bf_json.get("nonfinite_causes")
        rec["shadow_misses"] = bf_json.get("shadow_misses")

    rec["printed_interval"] = sum(1 for l in out.splitlines() if FLIP_RE.search(l))
    rec["printed_shadow"] = sum(1 for l in bftext.splitlines()
                                if SHADOW_FLIP_RE.search(l))
    return rec, out


def check_shadow_invariance(records):
    counts = {r["eta"]: r["shadow"]["flips"]
              for r in records if "shadow" in r and r["eta"]}
    if len(set(counts.values())) > 1:
        return f"shadow-rule flip count varies across the eta sweep ({counts})"
    return None


# ---------------------------------------------------------------- reporting

def one_line(r, cxx=False):
    bits = []
    for rule in ("interval", "shadow"):
        if rule not in r:
            continue
        d = r[rule]
        b = f"{rule} {d['flips']} @ {d['locations']} loc"
        if cxx:
            b += f" [in-TU {d['flips'] - d['foreign_flips']}, STL {d['foreign_flips']}]"
        if rule == "shadow" and d["flips_nonfinite"]:
            b += f" (nf {d['flips_nonfinite']})"
        if d["abstained"]:
            b += f" abst {d['abstained']}"
        bits.append(b)
    if r["out_of_scope_events"]:
        bits.append(f"out-of-scope {r['out_of_scope_events']}")
    rs = r["runtime_summary"].get("shadow_miss")
    if rs:
        bits.append("miss " + "/".join(str(v) for v in rs.values()))
    return "   ".join(bits)


def write_results(title, records, outdir, extra_per_record=None):
    lines = [title, ""]
    for r in records:
        eta = f"eta={r['eta']}" if r["eta"] else "eta=n/a"
        lines.append(f"  {eta:<14s}  {one_line(r)}")
    lines.append("")
    inv = check_shadow_invariance(records)
    if inv:
        lines += ["*** " + inv, ""]
    for r in records:
        eta = f"eta={r['eta']}" if r["eta"] else "shadow rule (eta inert)"
        lines.append(f"=== {eta} ===")
        for msg in r["consistency"]:
            lines.append(f"  *** {msg}")
        if extra_per_record:
            lines += extra_per_record(r)
        rs = r["runtime_summary"].get("shadow_miss")
        if rs:
            lines.append("  shadow misses: " +
                         "  ".join(f"{k}={v:,}" for k, v in rs.items()))
        nc = r["runtime_summary"].get("nonfinite_causes")
        if nc:
            lines.append("  nonfinite causes: " +
                         "  ".join(f"{k}={v:,}" for k, v in nc.items() if v))
        for rule in ("interval", "shadow"):
            if rule not in r:
                continue
            d = r[rule]
            nf = (f" ({d['flips_nonfinite']} non-finite shadow)"
                  if rule == "shadow" and d["flips_nonfinite"] else "")
            lines.append(f"  --- {rule}: {d['flips']}{nf} flips @ "
                         f"{d['locations']} loc ---")
            for site, n in d["sites"].items():
                lines.append(f"      {n:8d}  {site}")
            if d["abstained"]:
                lines.append(f"      ABSTAINED: {d['abstained']} event(s) @ "
                             f"{len(d['abstained_sites'])} loc")
                for site, n in list(d["abstained_sites"].items())[:10]:
                    lines.append(f"      {n:8d}  {site}   [abstain]")
            if r["out_of_scope_events"]:
                lines.append(f"      scoreable flips: {d['scoreable_flips']} "
                             f"of {d['flips']}")
        if "interval" in r and "shadow" in r:
            iv, sd = set(r["interval"]["sites"]), set(r["shadow"]["sites"])
            lines.append("  --- rule disagreement (by site) ---")
            lines.append(f"      both: {len(iv & sd)}   interval only: "
                         f"{len(iv - sd)}   shadow only: {len(sd - iv)}")
        if r["executed_sites"]:
            lines.append(f"      sites executed: {r['executed_sites']}   "
                         f"total executions: {r['total_executions']:,}")
        if r.get("event_log"):
            lines.append(f"      events: {r['event_log']}")
        for rule, p in r.get("event_log_by_rule", {}).items():
            lines.append(f"      events ({rule}): {p}")
        if r.get("branch_flip_json"):
            lines.append(f"      json: {r['branch_flip_json']}")
        lines.append("")
    (outdir / "summary.txt").write_text("\n".join(lines))
    (outdir / "summary.json").write_text(json.dumps(records, indent=2))


def print_header(name, args, extra=""):
    print(f"{name} / FPChecker   -{args.opt}  rule={args.bf_mode}  "
          f"nonfinite={args.nonfinite_policy}  fallback={args.shadow_fallback}"
          f"  {extra}".rstrip())
    print(f"  install: {FPC_INSTALL}")
    print(f"  work:    {WORK_BASE}\n")
