#!/usr/bin/env python3
"""brtrace_diff_mtu.py - multi-TU fp32-vs-fp64 branch-flip diff (STREAMING).

Records are 12 bytes: (uint32 module_id, uint32 site_id, int32 taken).
A flip is (module_id, site_id) reached in both runs with different `taken`.

This version STREAMS both trace files in fixed-size chunks and compares
record-by-record, so memory stays constant regardless of trace size. Handles
multi-GB traces (LULESH -s 10, hypre) without loading them into RAM.

POPULATION STATS (TP / TN)
--------------------------
Besides "which branches flipped", this also reports "out of how many" -- the
denominator that gives TN a definition. Because brtrace IS the oracle, TP/TN
here are properties of the branch population, not of a tool under test:

    TP = the two trajectories DISAGREE  (a flip)
    TN = the two trajectories AGREE     (a matched decision)

reported at both granularities:

    site-level  : a static instrumented site is TP if it flipped at least once
                  inside the lock-step window, TN if it executed there and
                  never flipped.
    event-level : every lock-step-compared branch evaluation is TP or TN.

Two categories are deliberately NOT counted as TN:

  * instrumented-but-never-executed sites -> DEAD. Folding dead code into TN
    inflates the negative universe with branches no run ever reached.
  * events past the divergence point / past min(len32, len64) -> UNADJUDICATED.
    Once the streams stop visiting the same site sequence there is no oracle
    verdict, so those events are not correct rejections. The coverage line
    says how much of the run this costs.

CAVEAT for captions: if the pair was built with -brtrace-fp-only, every count
here is over FP-CONTROLLED branches, not all branches. The static universe is
also only as complete as the .brsites files found under --mods; without --mods
there is no static universe and DEAD cannot be computed.

The report is printed to stdout and, with --report, also written to a .txt
file. It has two sections: EVENT-BASED (flips per location with TP/TN/executed
and a TOTAL row) and SITE-BASED (which lines flipped, then TP/TN/DEAD site
counts with totals).

Usage:
    brtrace_diff_mtu.py fp32.out fp64.out [--mods DIR_OR_GLOB] [--csv out.csv]
                        [--report report.txt] [--fast]
"""
import argparse
import glob
import os
import struct
import sys
from collections import Counter

REC = struct.Struct("<IIi")
RECSZ = REC.size
CHUNK_RECS = 1 << 16


def iter_records(path):
    buf = b""
    with open(path, "rb") as f:
        while True:
            data = f.read(CHUNK_RECS * RECSZ)
            if not data:
                break
            if buf:
                data = buf + data
                buf = b""
            n = len(data) // RECSZ
            rem = len(data) - n * RECSZ
            if rem:
                buf = data[n * RECSZ:]
                data = data[: n * RECSZ]
            for i in range(0, len(data), RECSZ):
                yield REC.unpack_from(data, i)
    if buf:
        sys.stderr.write("[warn] %s: %d trailing bytes ignored\n" % (path, len(buf)))


def count_records(path):
    return os.path.getsize(path) // RECSZ


def load_tables(mods_arg):
    """Parse .brmods/.brsites side tables.

    Returns (mod_name, sites, meta). `sites` is keyed by (module_id, site_id)
    and its KEY SET is the static universe of instrumented branch sites -- the
    denominator for the DEAD/TN split. `meta` records how that universe was
    assembled so it can be sanity-checked in the report.
    """
    mod_name, sites = {}, {}
    meta = {"brmods_files": 0, "brsites_files": 0, "modules": set()}
    if not mods_arg:
        return mod_name, sites, meta
    if os.path.isdir(mods_arg):
        brmods = glob.glob(os.path.join(mods_arg, "**", "*.brmods"), recursive=True)
        brsites = glob.glob(os.path.join(mods_arg, "**", "*.brsites"), recursive=True)
    else:
        brmods = glob.glob(mods_arg + "*.brmods")
        brsites = glob.glob(mods_arg + "*.brsites")
    for p in brmods:
        meta["brmods_files"] += 1
        with open(p) as f:
            for line in f:
                if line.startswith("#") or not line.strip():
                    continue
                mid, name = line.rstrip("\n").split("\t", 1)
                mod_name[int(mid)] = name
    for p in brsites:
        meta["brsites_files"] += 1
        mid = None
        with open(p) as f:
            for line in f:
                if line.startswith("# module_id"):
                    try:
                        mid = int(line.split()[2])
                    except (IndexError, ValueError):
                        mid = None
                    continue
                if line.startswith("#") or not line.strip():
                    continue
                parts = line.rstrip("\n").split("\t")
                if mid is not None and len(parts) >= 3:
                    sites[(mid, int(parts[0]))] = "%s  [%s]" % (parts[1], parts[2])
                    meta["modules"].add(mid)
    return mod_name, sites, meta


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("fp32")
    ap.add_argument("fp64")
    ap.add_argument("--mods")
    ap.add_argument("--csv")
    ap.add_argument("--max-report", type=int, default=50)
    ap.add_argument("--progress", type=int, default=0,
                    help="print progress every N million events (0=off)")
    ap.add_argument("--sites-txt",
                    help="dump one line per site (flips, executions, "
                         "TP/TN/DEAD class, location) to this .txt file. "
                         "Needed to score another tool against this census: "
                         "the report gives aggregate TN but not WHICH sites "
                         "are TN, and without that the negative universe "
                         "cannot be reconstructed.")
    ap.add_argument("--report",
                    help="also write the TP/TN report to this .txt file "
                         "(it is printed to stdout either way)")
    ap.add_argument("--fast", action="store_true",
                    help="skip the per-site execution census. Event-level TP "
                         "and site-level TP stay exact; all TN counts become "
                         "unavailable. ~40%% faster on traces of 100M+ events.")
    args = ap.parse_args()

    mod_name, sites, meta = load_tables(args.mods)
    n32 = count_records(args.fp32)
    n64 = count_records(args.fp64)
    n = min(n32, n64)

    def loc(mid, sid):
        return sites.get((mid, sid), mod_name.get(mid, "mod%d" % mid) + ":site%d" % sid)

    per_site = Counter()
    first_event = {}          # (mid,sid) -> event index of its FIRST flip
    first_flips = []
    total_flips = 0
    divergence = None

    csv_f = csv_w = None
    if args.csv:
        import csv
        csv_f = open(args.csv, "w", newline="")
        csv_w = csv.writer(csv_f)
        csv_w.writerow(["event_index", "module_id", "site_id",
                        "fp32_taken", "fp64_taken", "location"])

    a = iter_records(args.fp32)
    b = iter_records(args.fp64)
    prog = args.progress * 1_000_000 if args.progress else 0

    # Per-site execution census: how many times each site was adjudicated,
    # flip or not. This is what separates TN sites (executed, never flipped)
    # from DEAD sites (instrumented, never executed). It costs one dict
    # update per event, hence --fast.
    per_site_exec = Counter()
    census = not args.fast

    idx = 0
    while idx < n:
        m32, s32, t32 = next(a)
        m64, s64, t64 = next(b)
        if (m32, s32) != (m64, s64):
            divergence = (idx, (m32, s32), (m64, s64))
            break
        if census:
            per_site_exec[(m32, s32)] += 1
        if t32 != t64:
            total_flips += 1
            per_site[(m32, s32)] += 1
            first_event.setdefault((m32, s32), idx)
            if len(first_flips) < args.max_report:
                first_flips.append((idx, m32, s32, t32, t64))
            if csv_w:
                csv_w.writerow([idx, m32, s32, t32, t64, loc(m32, s32)])
        idx += 1
        if prog and idx % prog == 0:
            sys.stderr.write("  ...%dM events, %d flips so far\n" % (idx // 1_000_000, total_flips))

    if csv_f:
        csv_f.close()

    print("fp32 trace: %d events" % n32)
    print("fp64 trace: %d events" % n64)
    print("lock-step compared to event %d" % idx)
    print()

    if total_flips == 0 and divergence is None:
        if n32 == n64:
            print("No branch-decision flips. Identical paths across all TUs.")
        else:
            # Lock-step held for every event both runs have, but one run kept
            # going. The shorter trace is a strict prefix of the longer one --
            # the paths are NOT identical, and the tail is unadjudicated.
            print("No branch-decision flips within the common prefix, but the "
                  "traces differ in length (%d vs %d). The shorter run is a "
                  "prefix; its %d-event tail has no counterpart and is "
                  "UNADJUDICATED." % (n32, n64, abs(n32 - n64)))
    else:
        print("FLIP EVENTS: %d  across %d distinct sites" % (total_flips, len(per_site)))
        print()
        print("By site (most frequent first):")
        for (mid, sid), cnt in per_site.most_common():
            print("  mod %-11d site %-5d x%-9d %s" % (mid, sid, cnt, loc(mid, sid)))
        print()
        print("First %d flip events:" % len(first_flips))
        for (i, mid, sid, t32, t64) in first_flips:
            print("  event#%-10d mod %-11d site %-5d fp32=%d fp64=%d  %s"
                  % (i, mid, sid, t32, t64, loc(mid, sid)))
        if total_flips > len(first_flips):
            print("  ... %d more (all in CSV)" % (total_flips - len(first_flips)))

    if divergence:
        i, (m1, s1), (m2, s2) = divergence
        print()
        print("CONTROL-FLOW DIVERGENCE at event#%d: fp32 -> %s, fp64 -> %s. "
              "Lock-step stops (streams no longer aligned)."
              % (i, loc(m1, s1), loc(m2, s2)))

    if args.csv and (total_flips or divergence):
        print("\nWrote %d flip rows to %s" % (total_flips, args.csv))

    # Appended below everything the original tool printed, so existing log
    # scrapers keep working unchanged.
    report_population(args, n32, n64, idx, total_flips, per_site,
                      per_site_exec, sites, loc, meta, divergence, first_event)

    sys.exit(1 if (total_flips or divergence) else 0)


def report_population(args, n32, n64, adjudicated, tp_events, per_site,
                      per_site_exec, sites, loc, meta, divergence,
                      first_event=None):
    """Build the TP/TN report, print it, and optionally write it to --report.

    brtrace IS the oracle, so TP/TN are properties of the branch population,
    not of a tool under test:  TP = trajectories disagree, TN = they agree.
    """
    census = not args.fast
    tn_events = adjudicated - tp_events
    longest = max(n32, n64)
    unadjudicated = longest - adjudicated

    tp_sites = set(per_site)
    exec_sites = set(per_site_exec) if census else set()
    static_sites = set(sites) if sites else None

    L = []
    def out(line=""):
        L.append(line)

    def num(x):
        return "{:,}".format(x)

    bar = "=" * 74
    rule = "-" * 74

    out()
    out(bar)
    out("brtrace TP/TN REPORT   %s  vs  %s"
        % (os.path.basename(args.fp32), os.path.basename(args.fp64)))
    out(bar)
    out()
    out("  brtrace is the oracle, so:")
    out("      TP = the two trajectories DISAGREE  (a flip)")
    out("      TN = the two trajectories AGREE     (a matched decision)")
    out()
    out("  ADJUDICATION WINDOW")
    out("    lock-step compared      %14s events   (%.2f%% of longer trace)"
        % (num(adjudicated), (100.0 * adjudicated / longest) if longest else 0.0))
    out("    unadjudicated           %14s events   [no oracle verdict; NOT TN]"
        % num(unadjudicated))
    if unadjudicated:
        if divergence is not None:
            out("      cause: control-flow divergence at event#%s"
                % num(divergence[0]))
        elif n32 != n64:
            out("      cause: trace-length mismatch "
                "(shorter run is a prefix of the longer)")
    out()

    # ------------------------------------------------------------ 1. EVENTS
    out(rule)
    out("1. EVENT-BASED   (one branch evaluation = one item)")
    out(rule)
    out()

    ranked = per_site.most_common()
    labels = [loc(m, s) for m, s in [k for k, _ in ranked]]
    w = min(max([len(x) for x in labels] + [24]), 46)

    if ranked:
        out("  flips @ location")
        out()
        out("    %-*s %10s %12s %12s" % (w, "location", "TP", "TN", "executed"))
        out("    " + "-" * (w + 36))
        for (mid, sid), cnt in ranked:
            lab = loc(mid, sid)
            if len(lab) > w:
                lab = "..." + lab[-(w - 3):]
            if census:
                ex = per_site_exec[(mid, sid)]
                out("    %-*s %10s %12s %12s"
                    % (w, lab, num(cnt), num(ex - cnt), num(ex)))
            else:
                out("    %-*s %10s %12s %12s" % (w, lab, num(cnt), "n/a", "n/a"))
        out("    " + "-" * (w + 36))
        if census:
            quiet_sites = exec_sites - tp_sites
            quiet_ev = sum(per_site_exec[k] for k in quiet_sites)
            out("    %-*s %10s %12s %12s"
                % (w, "(%d site(s) that never flipped)" % len(quiet_sites),
                   "0", num(quiet_ev), num(quiet_ev)))
            out("    " + "-" * (w + 36))
        out("    %-*s %10s %12s %12s"
            % (w, "TOTAL", num(tp_events), num(tn_events), num(adjudicated)))
    else:
        out("  no flip events in the adjudicated window")
        out()
        out("    TOTAL   TP %s   TN %s   adjudicated %s"
            % (num(tp_events), num(tn_events), num(adjudicated)))
    out()
    if adjudicated:
        out("    event flip rate: %.8f %%" % (100.0 * tp_events / adjudicated))
    out()

    # ------------------------------------------------------------- 2. SITES
    out(rule)
    out("2. SITE-BASED   (one static branch site = one item)")
    out(rule)
    out()

    out("  flips @ %d line(s)" % len(tp_sites))
    out()
    fe = first_event or {}
    # Ordered by FIRST flip, not by frequency. When separating a root cause
    # from its cascade, what matters is which site flipped first -- and the
    # site immediately preceding a divergence is the one that caused it, which
    # frequency ordering buries.
    by_first = sorted(ranked, key=lambda kv: fe.get(kv[0], 1 << 62))
    for (mid, sid), cnt in by_first:
        lab = loc(mid, sid)
        bits = "%s flips" % num(cnt)
        if census:
            bits += " of %s executions" % num(per_site_exec[(mid, sid)])
        if (mid, sid) in fe:
            bits += ", first @ event#%s" % num(fe[(mid, sid)])
        out("    %s   (%s)" % (lab, bits))
    if not ranked:
        out("    (none)")
    if fe and divergence is not None:
        last_key = max(fe, key=lambda k: fe[k])
        last = fe[last_key]
        gap = divergence[0] - last
        out()
        out("    NOTE: the last site to start flipping was")
        out("            %s" % loc(*last_key))
        out("          at event#%s, %s event(s) before the divergence at "
            "event#%s." % (num(last), num(gap), num(divergence[0])))
        # The gap is the evidence, so state it and let its size speak. A flip
        # immediately preceding the split is near-conclusive; a larger gap
        # still identifies the candidate, but the trajectories stayed in
        # lock-step for a while afterwards, so the link is inferred rather
        # than observed.
        if gap <= 2:
            out("          Adjacent, so that site is almost certainly the "
                "cause of the split.")
        elif gap <= 1000:
            out("          The trajectories stayed in lock-step for those %s "
                "events, so the" % num(gap))
            out("          link is inferred, not observed -- the flip changed "
                "a value that only")
            out("          altered control flow later. Check the flips CSV "
                "around that index.")
        else:
            out("          That is a wide gap; treat the two as possibly "
                "unrelated and look")
            out("          for the real cause near event#%s."
                % num(divergence[0]))
        out("          Earlier-flipping sites did not break lock-step.")
    out()

    out("    %-42s %10s" % ("TP    sites that flipped at least once", num(len(tp_sites))))
    if census:
        tn_sites = exec_sites - tp_sites
        out("    %-42s %10s" % ("TN    sites executed, never flipped", num(len(tn_sites))))
        out("    " + "-" * 53)
        out("    %-42s %10s" % ("TOTAL sites executed", num(len(exec_sites))))
        out()
        if static_sites is not None:
            dead = static_sites - exec_sites
            out("    %-42s %10s   [NOT TN]"
                % ("DEAD  instrumented, never executed", num(len(dead))))
            out("    %-42s %10s"
                % ("TOTAL sites instrumented", num(len(static_sites))))
            orphan = exec_sites - static_sites
            if orphan:
                out()
                out("    [warn] %d executed site(s) are absent from the .brsites"
                    % len(orphan))
                out("           tables, so --mods is incomplete and both the")
                out("           instrumented total and DEAD are understated.")
                out("           Point --mods at a build dir holding every TU.")
        else:
            out("    %-42s %10s   [pass --mods]"
                % ("DEAD  instrumented, never executed", "n/a"))
            out("    %-42s %10s   [pass --mods]"
                % ("TOTAL sites instrumented", "n/a"))
        if exec_sites:
            out()
            out("    site flip rate: %.4f %%"
                % (100.0 * len(tp_sites) / len(exec_sites)))
    else:
        out("    %-42s %10s   [--fast]" % ("TN    sites executed, never flipped", "n/a"))
        out("    %-42s %10s   [--fast]" % ("TOTAL sites executed", "n/a"))
        out()
        out("    %-42s %10s   [--fast]" % ("DEAD  instrumented, never executed", "n/a"))
        if static_sites is not None:
            out("    %-42s %10s"
                % ("TOTAL sites instrumented", num(len(static_sites))))
    out()

    # -------------------------------------------------------------- caveats
    out(rule)
    out("CAVEATS  (carry these into any caption built from these numbers)")
    out(rule)
    out("  * DEAD sites and unadjudicated events are deliberately NOT counted")
    out("    as TN. Dead code was never reached and post-divergence events")
    out("    have no oracle verdict; neither is a correct rejection.")
    out("  * SCOPE: the diff cannot tell from the traces alone which branches")
    out("    were instrumented. If the pair was built with -brtrace-fp-only,")
    out("    every count above is over FP-CONTROLLED branches; otherwise it is")
    out("    over ALL conditional branches. Check the build log and say which")
    out("    in the caption -- the two give different denominators and, on")
    out("    LULESH, a different number of ground-truth sites.")
    if static_sites is not None:
        out("  * Static universe assembled from %d .brsites file(s), %d module(s)."
            % (meta["brsites_files"], len(meta["modules"])))
    out("  * Event-level TN outnumbers TP by orders of magnitude, so metrics")
    out("    that divide by it (accuracy, specificity) saturate. Use the")
    out("    site-based section for tables; use event TN as a denominator for")
    out("    false-alarm rates in prose.")
    out()

    if args.sites_txt:
        rows = []
        keys = set(per_site_exec) | set(per_site)
        if static_sites is not None:
            keys |= static_sites
        for key in sorted(keys):
            nf = per_site.get(key, 0)
            ne = per_site_exec.get(key, 0)
            cls = "TP" if nf else ("TN" if ne else "DEAD")
            rows.append((cls, nf, ne, fe.get(key, -1), loc(*key)))
        rows.sort(key=lambda r: ({"TP": 0, "TN": 1, "DEAD": 2}[r[0]], -r[1]))
        with open(args.sites_txt, "w") as fh:
            fh.write("# class  flips  executions  first_event  location\n")
            fh.write("# class: TP = flipped at least once, TN = executed and "
                     "never flipped,\n")
            fh.write("#        DEAD = instrumented but never executed "
                     "(NOT a correct rejection)\n")
            fh.write("# first_event: -1 if the site never flipped\n")
            if not census:
                fh.write("# NOTE: --fast was used, so executions are 0 and "
                         "TN/DEAD cannot be distinguished\n")
            for cls, nf, ne, first, where in rows:
                fh.write("%-5s %10d %12d %12d  %s\n"
                         % (cls, nf, ne, first, where))
        L.append("  wrote per-site table to %s" % args.sites_txt)
        L.append("")

    text = "\n".join(L)
    print(text)
    if args.report:
        with open(args.report, "w") as fh:
            fh.write(text + "\n")
        print("Wrote report to %s" % args.report)


if __name__ == "__main__":
    main()