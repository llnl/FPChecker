#!/usr/bin/env python3
"""
adjudicate.py -- score a tool's flip reports against the brtrace census.

    python3 adjudicate.py \\
        --gt      lulesh/results/O0/fp32_vs_fp64/sites.txt \\
        --tool    fpchecker_experiments/lulesh/results/O0/fp32/summary.txt \\
        --coverage 21.62 --divergence-event 87985

Produces TP / FP / TN / FN at BOTH site and event granularity, for every eta
block found in the tool summary.

THREE THINGS THAT SILENTLY CORRUPT THIS SCORE
---------------------------------------------

1. FILE ATTRIBUTION. FPChecker names the compiled TU, not the file the branch
   came from: std::max is reported as lulesh.cc:263 where it really lives at
   stl_algobase.h:263 (right line, wrong file), because FPChecker reads
   DILocation.line without walking getInlinedAt(). Matching on file+line marks
   the single biggest true positive as a false positive AND leaves the real
   site as a false negative -- two errors from one mismatch. This script
   therefore matches on (function, line) with the function name normalised
   (return type and template arguments stripped), which is stable across both
   tools, and reports the file only for display.

2. THE NEGATIVE UNIVERSE. The two tools do not instrument the same branches.
   A site the tool flags that brtrace never instrumented cannot be scored --
   the oracle has no opinion on it. Counting those as FP punishes the tool for
   looking somewhere the oracle did not; counting them as TP credits it for an
   unverified claim. They are reported separately as OUT-OF-UNIVERSE, and TN is
   computed only over sites brtrace actually executed.

3. COVERAGE. When the census is a prefix (lock-step ends at a divergence),
   every count is a prefix count. A tool flagging a site that only flips AFTER
   the divergence point looks like a false positive but may be correct. With
   --coverage the script says so and labels FP as an upper bound and FN as a
   lower bound.

EVENT-LEVEL SCORING IS NOT SYMMETRIC WITH SITE-LEVEL and is reported with that
caveat: brtrace counts branch-decision disagreements, FPChecker counts warnings
it chose to emit (subject to its own warning cap and eta window). They are
different quantities that happen to share a unit. Site-level is the comparable
one; event-level is reported because the magnitudes are informative, not
because the ratio is meaningful.
"""

import argparse
import re
import sys
from collections import OrderedDict

# ------------------------------------------------------------------ parsing

GT_ROW = re.compile(r"^(TP|TN|DEAD)\s+(\d+)\s+(\d+)\s+(-?\d+)\s+(.*)$")

# "        2247  lulesh.cc:263  [float const& std::max<float>]"
TOOL_ROW = re.compile(r"^\s+(\d+)\s+(\S+?):(\d+)\s*(?:\[(.*)\])?\s*$")
ETA_HDR = re.compile(r"^---\s*(?:eta=)?(\S+?)\s*:")


ANON = "(anonymous namespace)"


def norm_func(f):
    """Normalise a function name so the two tools' spellings agree.

    Strips the return type, the argument list, and template arguments:
        'float const& std::max<float>'  -> 'std::max'
        'std::max<long double>'         -> 'std::max'
        'CalcEnergyForElems'            -> 'CalcEnergyForElems'
    Template args must go: the same site is instantiated at float in the fp32
    build and double in the fp64 build whose side tables the census used.
    """
    if not f:
        return ""
    f = f.strip()
    # "(anonymous namespace)::Foo(int)" -- the leading parenthesis is part of
    # the NAME, not an argument list, and would otherwise be taken as the start
    # of the arg list and truncate the whole thing to "". QuickSilver puts most
    # of MCT.cc in an anonymous namespace, so this affects both sides.
    f = f.replace(ANON, "{anon}")
    # drop argument list at depth 1
    depth, cut = 0, None
    for i, c in enumerate(f):
        if c in "<(":
            depth += 1
            if c == "(" and depth == 1:
                cut = i
                break
        elif c in ">)":
            depth -= 1
    if cut is not None:
        f = f[:cut]
    # drop return type: last space at depth 0
    depth, last = 0, -1
    for i, c in enumerate(f):
        if c in "<(":
            depth += 1
        elif c in ">)":
            depth -= 1
        elif c == " " and depth == 0:
            last = i
    f = f[last + 1:]
    # drop template args
    depth, out = 0, []
    for c in f:
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        elif depth == 0:
            out.append(c)
    return "".join(out).strip()


def parse_loc(text):
    """'stl_algobase.h:263  [_ZSt3maxIdERKT_S2_S2_]' -> (file, line, func)"""
    m = re.match(r"^(.*?):(\d+)\s*(?:\[(.*)\])?\s*$", text.strip())
    if not m:
        return None
    return m.group(1).split("/")[-1], int(m.group(2)), (m.group(3) or "").strip()


def demangle_all(syms):
    """Batch c++filt. FPChecker summaries are already demangled; brtrace side
    tables are not."""
    import subprocess
    todo = [s for s in syms if s.startswith("_Z")]
    if not todo:
        return {}
    try:
        out = subprocess.run(["c++filt"] + todo, stdout=subprocess.PIPE,
                             text=True, timeout=30).stdout.splitlines()
        return dict(zip(todo, out))
    except Exception:
        return {}


def load_gt(path):
    """Read the brtrace --sites-txt dump."""
    rows = []
    for line in open(path):
        if line.startswith("#"):
            continue
        m = GT_ROW.match(line.rstrip("\n"))
        if not m:
            continue
        cls, flips, execs, first, where = (m.group(1), int(m.group(2)),
                                           int(m.group(3)), int(m.group(4)),
                                           m.group(5))
        p = parse_loc(where)
        if not p:
            continue
        rows.append({"class": cls, "flips": flips, "executions": execs,
                     "first": first, "file": p[0], "line": p[1], "sym": p[2]})
    dm = demangle_all([r["sym"] for r in rows])
    for r in rows:
        r["func"] = norm_func(dm.get(r["sym"], r["sym"]))
        r["key"] = (r["func"], r["line"])
        # Secondary key for tools that report a file but no function.
        # EFTSanitizer prints a bare line number and its harness resolves the
        # file from the merged module's debug info, so (file, line) is all
        # there is to match on.
        r["fkey"] = (r["file"], r["line"])

    # Several instrumented branches can share one (function, line) -- AMG has
    # many. Neither tool can tell them apart, since both report at line
    # granularity, so merge them into ONE scoring unit rather than letting the
    # later row overwrite the earlier and silently lose its executions.
    merged = {}
    for r in rows:
        m = merged.get(r["key"])
        if m is None:
            r["merged_sites"] = 1
            merged[r["key"]] = r
            continue
        m["flips"] += r["flips"]
        m["executions"] += r["executions"]
        m["merged_sites"] += 1
        if r["first"] >= 0:
            m["first"] = (r["first"] if m["first"] < 0
                          else min(m["first"], r["first"]))
        m["class"] = ("TP" if m["flips"] else
                      ("TN" if m["executions"] else "DEAD"))
    return list(merged.values())


def load_tool(path):
    """Read an FPChecker-style summary.txt -> OrderedDict eta -> [sites].

    Function names are demangled here as well as on the ground-truth side.
    FPChecker usually demangles before writing its summary, but not always --
    the QuickSilver summaries carry raw _ZN... symbols. Comparing a mangled
    tool name against a demangled census name fails silently and drops the
    site, so both sides are normalised the same way.
    """
    blocks, cur = OrderedDict(), None
    raw = open(path).read()
    dm = demangle_all(set(re.findall(r"\[(_Z[\w.$]+)\]", raw)))
    for line in open(path):
        h = ETA_HDR.match(line.strip())
        if h:
            cur = h.group(1)
            blocks[cur] = []
            continue
        if cur is None:
            continue
        m = TOOL_ROW.match(line.rstrip("\n"))
        if m:
            sym = (m.group(4) or "").strip()
            func = norm_func(dm.get(sym, sym))
            # EFTSanitizer's harness writes the literal token AMBIGUOUS in the
            # filename slot when a line number carries hooks in more than one
            # file, and puts a candidate file in the bracket where a function
            # name would normally go. Parsing that as a file named
            # "AMBIGUOUS" produces a site that can never match anything.
            unresolved_by_tool = m.group(2).upper().startswith("AMBIGUOUS")
            blocks[cur].append({"tool_ambiguous": unresolved_by_tool,
                                "flips": int(m.group(1)),
                                "file": m.group(2).split("/")[-1],
                                "line": int(m.group(3)),
                                "func": func,
                                "key": (func, int(m.group(3))),
                                "fkey": (m.group(2).split("/")[-1],
                                         int(m.group(3)))})
    return blocks


# ----------------------------------------------------------------- scoring

def resolve(tool_site, by_func, by_file):
    """Map a tool-reported site onto ground-truth key(s). Returns a list.

    Function+line is preferred because it survives the file-attribution
    mismatch (FPChecker reports std::max under lulesh.cc). When the tool emits
    no function name -- EFTSanitizer prints a bare line number -- fall back to
    file+line, which is what its harness resolves from debug info.

    One source line can carry several instrumented branches, so file+line can
    map to more than one census site (AMG has many: par_amg.c:787,
    par_strength.c:343). The tool named a line and cannot distinguish between
    the branches on it, so ALL of them are treated as flagged. That is only
    lossy if the candidates disagree about class; when they do, the caller is
    told.
    """
    if tool_site["func"] and tool_site["key"] in by_func:
        return [tool_site["key"]]
    if tool_site["fkey"] in by_file:
        return list(by_file[tool_site["fkey"]])
    return [tool_site["key"] if tool_site["func"] else tool_site["fkey"]]


def score(gt_rows, tool_sites, coverage, divergence):
    gt_by_key = {r["key"]: r for r in gt_rows}
    by_func = {r["key"] for r in gt_rows}
    by_file = {}
    for r in gt_rows:
        by_file.setdefault(r["fkey"], []).append(r["key"])
    # Only genuinely ambiguous if the sites on that line disagree about class:
    # if they are all TN, "the tool flagged this line" is a false positive
    # either way, and there is nothing to resolve.
    cls = {r["key"]: r["class"] for r in gt_rows}
    ambiguous = {k for k, v in by_file.items()
                 if len({cls[x] for x in v}) > 1}
    universe = {r["key"] for r in gt_rows if r["class"] in ("TP", "TN")}
    gt_pos = {r["key"] for r in gt_rows if r["class"] == "TP"}
    gt_neg = {r["key"] for r in gt_rows if r["class"] == "TN"}
    gt_dead = {r["key"] for r in gt_rows if r["class"] == "DEAD"}

    flagged, unresolved = {}, []
    for t in tool_sites:
        if t["fkey"] in ambiguous:
            unresolved.append(t)
            continue
        keys = resolve(t, by_func, by_file)
        # Warnings are attributed once, to the first candidate, so a line
        # carrying two branches does not double the reported volume.
        for i, k in enumerate(keys):
            flagged[k] = flagged.get(k, 0) + (t["flips"] if i == 0 else 0)

    in_uni = {k for k in flagged if k in universe}
    on_dead = {k for k in flagged if k in gt_dead}
    outside = {k for k in flagged if k not in universe and k not in gt_dead}

    tp = in_uni & gt_pos
    fp = in_uni & gt_neg
    fn = gt_pos - in_uni
    tn = gt_neg - in_uni

    ev = {
        "tp": sum(gt_by_key[k]["flips"] for k in tp),
        "fn": sum(gt_by_key[k]["flips"] for k in fn),
        "tool_on_tp": sum(flagged[k] for k in tp),
        "tool_on_fp": sum(flagged[k] for k in fp),
        "tool_on_dead": sum(flagged[k] for k in on_dead),
        "tool_outside": sum(flagged[k] for k in outside),
        "tn": sum(gt_by_key[k]["executions"] for k in tn),
    }
    toolloc = {}
    for t in tool_sites:
        for k in resolve(t, by_func, by_file):
            toolloc.setdefault(k, "%s:%d" % (t["file"], t["line"]))
    return {"tp": tp, "fp": fp, "fn": fn, "tn": tn, "dead_flagged": on_dead,
            "outside": outside, "flagged": flagged, "gt": gt_by_key, "ev": ev,
            "ambiguous": unresolved, "toolloc": toolloc}


def ratio(a, b):
    return "%.4f" % (a / b) if b else "—"


def report(eta, r, coverage, divergence, show):
    g, ev = r["gt"], r["ev"]
    ntp, nfp, nfn, ntn = len(r["tp"]), len(r["fp"]), len(r["fn"]), len(r["tn"])
    w = sys.stdout.write

    w("\n" + "=" * 74 + "\n")
    w("eta = %s\n" % eta)
    w("=" * 74 + "\n\n")

    w("  SITE-BASED   (scored over the %d site(s) brtrace executed)\n"
      % (ntp + nfp + nfn + ntn))
    w("    TP  flagged, really flips        %6d\n" % ntp)
    w("    FP  flagged, never flips         %6d\n" % nfp)
    w("    FN  flips, not flagged           %6d\n" % nfn)
    w("    TN  never flips, not flagged     %6d\n" % ntn)
    w("    ----------------------------------------\n")
    w("    precision  TP/(TP+FP)            %8s\n" % ratio(ntp, ntp + nfp))
    w("    recall     TP/(TP+FN)            %8s\n" % ratio(ntp, ntp + nfn))
    f1 = ratio(2 * ntp, 2 * ntp + nfp + nfn)
    w("    F1         2TP/(2TP+FP+FN)       %8s\n" % f1)
    w("\n")

    # A tool that flags every executed site scores recall 1.0 by construction.
    # When a real tool flags most of the universe, its recall is close to free
    # and only the margin over this baseline is evidence of discrimination.
    uni = ntp + nfp + nfn + ntn
    npos = ntp + nfn
    if uni and npos:
        b_fp = uni - npos
        w("    flag-everything baseline (flags all %d executed sites)\n" % uni)
        w("      precision %8s   recall %8s   F1 %8s\n"
          % (ratio(npos, uni), "1.0000", ratio(2 * npos, 2 * npos + b_fp)))
        w("      the tool flagged %d of %d sites (%.1f%%)\n"
          % (ntp + nfp, uni, 100.0 * (ntp + nfp) / uni))
        w("\n")

    if r["dead_flagged"] or r["outside"] or r.get("ambiguous"):
        w("  UNSCORABLE (excluded from the counts above)\n")
        if r.get("ambiguous"):
            w("    %d flagged site(s) on a source line carrying several\n"
              "      instrumented branches that DISAGREE about class -- one "
              "flips,\n      another does not -- so the verdict cannot be "
              "assigned without a\n      function name.\n"
              % len(r["ambiguous"]))
        if r["dead_flagged"]:
            w("    %d site(s) flagged that brtrace instrumented but never\n"
              "      executed -- the oracle has no verdict.\n"
              % len(r["dead_flagged"]))
        if r["outside"]:
            w("    %d site(s) flagged that brtrace never instrumented.\n"
              % len(r["outside"]))
            w("      The two tools do not cover the same branches; counting\n"
              "      these as FP would punish the tool for looking where the\n"
              "      oracle did not.\n")
        w("\n")

    w("  EVENT-BASED\n")
    w("    oracle flip events at TP sites   %10s\n" % "{:,}".format(ev["tp"]))
    w("    oracle flip events at FN sites   %10s   [missed]\n"
      % "{:,}".format(ev["fn"]))
    w("    tool warnings at TP sites        %10s\n"
      % "{:,}".format(ev["tool_on_tp"]))
    w("    tool warnings at FP sites        %10s\n"
      % "{:,}".format(ev["tool_on_fp"]))
    if ev["tool_outside"] or ev["tool_on_dead"]:
        w("    tool warnings, unscorable        %10s\n"
          % "{:,}".format(ev["tool_outside"] + ev["tool_on_dead"]))
    w("    oracle-verified TN events        %10s\n" % "{:,}".format(ev["tn"]))
    w("    event recall  TP/(TP+FN)         %10s\n"
      % ratio(ev["tp"], ev["tp"] + ev["fn"]))
    w("\n")
    w("    CAVEAT: these two columns are not the same quantity. brtrace\n")
    w("    counts branch-decision disagreements; the tool counts warnings it\n")
    w("    chose to emit, subject to its own warning cap and eta window. Use\n")
    w("    the site-based block for tables.\n")
    w("\n")

    if coverage is not None and coverage < 99.995:
        w("  COVERAGE %.2f%%%s\n"
          % (coverage,
             " (lock-step ended at event #%s)" % "{:,}".format(divergence)
             if divergence else ""))
        w("    The census is a PREFIX. A site that only flips after the\n")
        w("    divergence is scored FP here but may be correct, so:\n")
        w("        FP = %d is an UPPER bound\n" % nfp)
        w("        FN = %d is a LOWER bound\n" % nfn)
        w("        precision is a lower bound; recall is an upper bound\n")
        w("\n")

    if show:
        def dump(title, keys, withgt):
            if not keys:
                return
            w("  %s\n" % title)
            for k in sorted(keys, key=lambda k: -r["flagged"].get(k, 0)):
                g_ = g.get(k)
                # For sites outside the census there is no ground-truth row,
                # so fall back to the location the TOOL reported -- "?" tells
                # the reader nothing about which site is unmatched.
                where = (("%s:%d" % (g_["file"], g_["line"])) if g_
                         else r["toolloc"].get(k, "?:%d" % k[1]))
                bits = "    %-34s [%s]" % (where, k[0])
                if r["flagged"].get(k):
                    bits += "   tool %s" % "{:,}".format(r["flagged"][k])
                if withgt and g_:
                    bits += "   oracle %s of %s" % (
                        "{:,}".format(g_["flips"]),
                        "{:,}".format(g_["executions"]))
                w(bits + "\n")
            w("\n")
        dump("TP sites", r["tp"], True)
        dump("FN sites  (oracle flips, tool silent)", r["fn"], True)
        dump("FP sites  (tool flags, oracle says never flips)", r["fp"], True)
        dump("Unscorable: flagged but outside the oracle's universe",
             r["outside"], False)


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gt", required=True,
                    help="brtrace --sites-txt dump")
    ap.add_argument("--tool", required=True,
                    help="FPChecker-style summary.txt")
    ap.add_argument("--coverage", type=float, default=None,
                    help="census coverage %% (from the brtrace report)")
    ap.add_argument("--divergence-event", type=int, default=None)
    ap.add_argument("--eta", default=None, help="score only this eta block")
    ap.add_argument("--no-detail", action="store_true",
                    help="counts only, no per-site listing")
    args = ap.parse_args()

    gt = load_gt(args.gt)
    if not gt:
        sys.exit("no site rows parsed from %s -- is it a --sites-txt dump?"
                 % args.gt)
    blocks = load_tool(args.tool)
    if not blocks:
        sys.exit("no eta blocks parsed from %s" % args.tool)

    npos = sum(1 for r in gt if r["class"] == "TP")
    nneg = sum(1 for r in gt if r["class"] == "TN")
    ndead = sum(1 for r in gt if r["class"] == "DEAD")
    print("ground truth: %s" % args.gt)
    print("  %d flipping site(s), %d executed-and-stable, %d dead (unscored)"
          % (npos, nneg, ndead))
    print("tool: %s" % args.tool)
    print("  eta blocks: %s" % ", ".join(blocks))

    for eta, sites in blocks.items():
        if args.eta and eta != args.eta:
            continue
        r = score(gt, sites, args.coverage, args.divergence_event)
        report(eta, r, args.coverage, args.divergence_event,
               not args.no_detail)
    return 0


if __name__ == "__main__":
    sys.exit(main())