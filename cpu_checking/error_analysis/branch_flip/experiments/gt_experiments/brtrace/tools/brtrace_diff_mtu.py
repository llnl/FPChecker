#!/usr/bin/env python3
"""brtrace_diff_mtu.py - fp32-vs-fp64 branch-flip diff, streaming.

Records are 12 bytes: (uint32 module_id, uint32 site_id, int32 taken). The
two traces are walked in lock-step; same id with different `taken` is a flip,
a different id is a control-flow divergence and adjudication stops there.

TP = the trajectories disagree (a flip), TN = they agree, at event and site
granularity. Sites never executed inside the window are DEAD and events past
the divergence are unadjudicated; neither is counted as TN.

Usage:
    brtrace_diff_mtu.py fp32.out fp64.out [--kind branch|select]
        [--mods DIR_OR_GLOB] [--csv flips.csv] [--window-csv window.csv]
        [--sites-txt sites.txt] [--report report.txt] [--fast]
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


def load_tables(mods_arg, kind="branch"):
    """Read .brmods and the side tables for one stream.

    Returns (mod_name, sites, meta); the key set of `sites` is the static
    universe for that stream."""
    ext = ".brselsites" if kind == "select" else ".brsites"
    mod_name, sites = {}, {}
    meta = {"brmods_files": 0, "brsites_files": 0, "modules": set(),
            "kind": kind, "versions": set(), "fp_only": set(),
            "n_fcmp": {}, "multi_fcmp_sites": 0, "declared_counts": {}}
    if not mods_arg:
        return mod_name, sites, meta
    if os.path.isdir(mods_arg):
        brmods = glob.glob(os.path.join(mods_arg, "**", "*.brmods"), recursive=True)
        brsites = glob.glob(os.path.join(mods_arg, "**", "*" + ext), recursive=True)
    else:
        brmods = glob.glob(mods_arg + "*.brmods")
        brsites = glob.glob(mods_arg + "*" + ext)
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
        seen_here = 0
        with open(p) as f:
            for line in f:
                if line.startswith("# brtrace-table-version"):
                    meta["versions"].add(line.split()[2])
                    continue
                if line.startswith("# fp_only"):
                    meta["fp_only"].add(line.split()[2])
                    continue
                if line.startswith("# module_id"):
                    try:
                        mid = int(line.split()[2])
                    except (IndexError, ValueError):
                        mid = None
                    continue
                if line.startswith("# n_sites"):
                    if mid is not None:
                        meta["declared_counts"][mid] = int(line.split()[2])
                    continue
                if line.startswith("#") or not line.strip():
                    continue
                parts = line.rstrip("\n").split("\t")
                if mid is not None and len(parts) >= 3:
                    sid = int(parts[0])
                    sites[(mid, sid)] = "%s  [%s]" % (parts[1], parts[2])
                    meta["modules"].add(mid)
                    seen_here += 1
                    if len(parts) >= 4:
                        nf = int(parts[3])
                        meta["n_fcmp"][(mid, sid)] = nf
                        if nf > 1:
                            meta["multi_fcmp_sites"] += 1
        if mid is not None and mid in meta["declared_counts"]:
            if seen_here != meta["declared_counts"][mid]:
                sys.exit("%s: header declares %d sites but file has %d rows"
                         % (p, meta["declared_counts"][mid], seen_here))
    if len(meta["fp_only"]) > 1:
        sys.exit("side tables under --mods were produced with mixed "
                 "-brtrace-fp-only settings (%s)"
                 % ", ".join(sorted(meta["fp_only"])))
    return mod_name, sites, meta


def write_window_csv(path, kind, per_site_exec, per_site, first_occ,
                     first_event, sites, meta, loc):
    """One row per known site with E_S, the number of adjudicated executions;
    a detection at occurrence index k is in-window iff k < E_S. Static sites
    that never executed get E_S = 0."""
    import csv as _csv
    keys = set(per_site_exec) | set(per_site)
    if sites:
        keys |= set(sites)
    nf = meta.get("n_fcmp", {})
    with open(path, "w", newline="") as fh:
        w = _csv.writer(fh)
        w.writerow(["kind", "module_id", "site_id", "E_S", "flips",
                    "first_flip_occ", "first_flip_event", "n_fcmp",
                    "location"])
        for (mid, sid) in sorted(keys):
            w.writerow([kind, mid, sid,
                        per_site_exec.get((mid, sid), 0),
                        per_site.get((mid, sid), 0),
                        first_occ.get((mid, sid), -1),
                        first_event.get((mid, sid), -1),
                        nf.get((mid, sid), -1),
                        loc(mid, sid)])
    print("Wrote per-site adjudication window (%s stream, %d sites) to %s"
          % (kind, len(keys), path))
    if meta.get("multi_fcmp_sites"):
        print("  %d site(s) are controlled by more than one fcmp (see n_fcmp)"
              % meta["multi_fcmp_sites"])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("fp32")
    ap.add_argument("fp64")
    ap.add_argument("--kind", choices=["branch", "select"], default="branch",
                    help="which stream the inputs are; run once per stream")
    ap.add_argument("--mods")
    ap.add_argument("--csv", help="per-execution flips")
    ap.add_argument("--max-report", type=int, default=50)
    ap.add_argument("--progress", type=int, default=0,
                    help="print progress every N million events (0=off)")
    ap.add_argument("--sites-txt",
                    help="per-site table: class, ids, flips, executions, location")
    ap.add_argument("--window-csv",
                    help="per-site adjudication window (E_S per site)")
    ap.add_argument("--report", help="also write the TP/TN report here")
    ap.add_argument("--fast", action="store_true",
                    help="skip the per-site execution census (no TN counts)")
    args = ap.parse_args()

    if args.fast and (args.csv or args.window_csv):
        sys.exit("--fast disables the per-site census that --csv and "
                 "--window-csv depend on")

    mod_name, sites, meta = load_tables(args.mods, args.kind)
    n32 = count_records(args.fp32)
    n64 = count_records(args.fp64)
    n = min(n32, n64)

    def loc(mid, sid):
        return sites.get((mid, sid), mod_name.get(mid, "mod%d" % mid) + ":site%d" % sid)

    per_site = Counter()
    first_event = {}
    first_occ = {}
    first_flips = []
    total_flips = 0
    divergence = None

    csv_f = csv_w = None
    if args.csv:
        import csv
        csv_f = open(args.csv, "w", newline="")
        csv_w = csv.writer(csv_f)
        # occ_index is the 0-based per-site occurrence inside the window.
        csv_w.writerow(["kind", "event_index", "occ_index", "module_id",
                        "site_id", "a_taken", "b_taken", "location"])

    a = iter_records(args.fp32)
    b = iter_records(args.fp64)
    prog = args.progress * 1_000_000 if args.progress else 0

    per_site_exec = Counter()
    census = not args.fast

    idx = 0
    while idx < n:
        m32, s32, t32 = next(a)
        m64, s64, t64 = next(b)
        if (m32, s32) != (m64, s64):
            divergence = (idx, (m32, s32), (m64, s64))
            break
        occ = -1
        if census:
            per_site_exec[(m32, s32)] += 1
            occ = per_site_exec[(m32, s32)] - 1
        if t32 != t64:
            total_flips += 1
            per_site[(m32, s32)] += 1
            first_event.setdefault((m32, s32), idx)
            first_occ.setdefault((m32, s32), occ)
            if len(first_flips) < args.max_report:
                first_flips.append((idx, m32, s32, t32, t64))
            if csv_w:
                csv_w.writerow([args.kind, idx, occ, m32, s32, t32, t64,
                                loc(m32, s32)])
        idx += 1
        if prog and idx % prog == 0:
            sys.stderr.write("  ...%dM events, %d flips so far\n" % (idx // 1_000_000, total_flips))

    if csv_f:
        csv_f.close()

    if args.window_csv:
        write_window_csv(args.window_csv, args.kind, per_site_exec, per_site,
                         first_occ, first_event, sites, meta, loc)

    print("fp32 trace: %d events" % n32)
    print("fp64 trace: %d events" % n64)
    print("lock-step compared to event %d" % idx)
    print()

    if total_flips == 0 and divergence is None:
        if n32 == n64:
            print("No branch-decision flips. Identical paths across all TUs.")
        else:
            print("No branch-decision flips within the common prefix, but the "
                  "traces differ in length (%d vs %d); the %d-event tail is "
                  "unadjudicated." % (n32, n64, abs(n32 - n64)))
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
              "Lock-step stops."
              % (i, loc(m1, s1), loc(m2, s2)))

    if args.csv and (total_flips or divergence):
        print("\nWrote %d flip rows to %s" % (total_flips, args.csv))

    report_population(args, n32, n64, idx, total_flips, per_site,
                      per_site_exec, sites, loc, meta, divergence, first_event)

    sys.exit(1 if (total_flips or divergence) else 0)


def report_population(args, n32, n64, adjudicated, tp_events, per_site,
                      per_site_exec, sites, loc, meta, divergence,
                      first_event=None):
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
    out("  ADJUDICATION WINDOW")
    out("    lock-step compared      %14s events   (%.2f%% of longer trace)"
        % (num(adjudicated), (100.0 * adjudicated / longest) if longest else 0.0))
    out("    unadjudicated           %14s events"
        % num(unadjudicated))
    if unadjudicated:
        if divergence is not None:
            out("      cause: control-flow divergence at event#%s"
                % num(divergence[0]))
        elif n32 != n64:
            out("      cause: trace-length mismatch")
    out()

    out(rule)
    out("1. EVENT-BASED")
    out(rule)
    out()

    ranked = per_site.most_common()
    labels = [loc(m, s) for m, s in [k for k, _ in ranked]]
    w = min(max([len(x) for x in labels] + [24]), 46)

    if ranked:
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

    out(rule)
    out("2. SITE-BASED")
    out(rule)
    out()

    out("  flips @ %d line(s)" % len(tp_sites))
    out()
    fe = first_event or {}
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
        out()
        out("    last site to start flipping: %s at event#%s, %s event(s) "
            "before the divergence"
            % (loc(*last_key), num(last), num(divergence[0] - last)))
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
            out("    %-42s %10s"
                % ("DEAD  instrumented, never executed", num(len(dead))))
            out("    %-42s %10s"
                % ("TOTAL sites instrumented", num(len(static_sites))))
            orphan = exec_sites - static_sites
            if orphan:
                out()
                out("    [warn] %d executed site(s) are absent from the "
                    ".brsites tables; --mods is incomplete" % len(orphan))
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
    if static_sites is not None:
        out("  static universe: %d .brsites file(s), %d module(s)"
            % (meta["brsites_files"], len(meta["modules"])))
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
            rows.append((cls, key[0], key[1], nf, ne, fe.get(key, -1),
                         loc(*key)))
        rows.sort(key=lambda r: ({"TP": 0, "TN": 1, "DEAD": 2}[r[0]], -r[3]))
        with open(args.sites_txt, "w") as fh:
            fh.write("# stream: %s\n" % args.kind)
            fh.write("# class  module_id  site_id  flips  executions  "
                     "first_event  location\n")
            fh.write("# class: TP = flipped at least once, TN = executed and "
                     "never flipped, DEAD = instrumented but never executed\n")
            fh.write("# first_event: -1 if the site never flipped\n")
            if not census:
                fh.write("# NOTE: --fast was used, so executions are 0 and "
                         "TN/DEAD cannot be distinguished\n")
            for cls, mid, sid, nf, ne, first, where in rows:
                fh.write("%-5s %12d %8d %10d %12d %12d  %s\n"
                         % (cls, mid, sid, nf, ne, first, where))
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
