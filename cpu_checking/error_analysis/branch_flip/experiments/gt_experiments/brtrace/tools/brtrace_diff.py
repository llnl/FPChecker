#!/usr/bin/env python3
"""brtrace_diff.py - compare an fp32 trace against an fp64 trace and report
branch-decision flips.

Usage:
    brtrace_diff.py fp32.out fp64.out [--sites module.brsites] [--csv out.csv]

Both traces are streams of 8-byte records (uint32 site_id, int32 taken),
produced by the same instrumented source built once as fp32 and once as fp64.
Because the pass assigns site_ids deterministically from source structure, the
two streams follow the SAME dynamic branch sequence UNLESS a branch actually
flipped -- at which point control flow can diverge. We therefore walk the two
streams in lock-step and, at each step, compare:

    - if site_ids match and `taken` differs   -> FLIP at that site
    - if site_ids differ                       -> control-flow DIVERGENCE
      (the flip already happened one step earlier and sent the two runs down
       different paths; we report the divergence point and stop lock-step,
       since after a real divergence the streams are no longer aligned)

For the common "same path, occasional flip that re-converges" case this counts
every flip. For a hard divergence it reports the first flip + where paths split.

The site table (optional) maps site_id -> file:line function for readable
output.
"""
import argparse
import struct
import sys
from collections import Counter

REC = struct.Struct("<Ii")  # uint32 site_id, int32 taken
RECSZ = REC.size


def read_records(path):
    with open(path, "rb") as f:
        data = f.read()
    if len(data) % RECSZ != 0:
        sys.stderr.write(
            f"[warn] {path}: size {len(data)} not a multiple of {RECSZ}; "
            f"truncating tail\n"
        )
        data = data[: len(data) - (len(data) % RECSZ)]
    for i in range(0, len(data), RECSZ):
        yield REC.unpack_from(data, i)


def load_sites(path):
    sites = {}
    if not path:
        return sites
    with open(path) as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            parts = line.rstrip("\n").split("\t")
            if len(parts) >= 3:
                sid = int(parts[0])
                sites[sid] = f"{parts[1]}  [{parts[2]}]"
            elif len(parts) == 2:
                sites[int(parts[0])] = parts[1]
    return sites


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("fp32", help="fp32 trace (BRTRACE_OUT from fp32 build)")
    ap.add_argument("fp64", help="fp64 trace (BRTRACE_OUT from fp64 build)")
    ap.add_argument("--sites", help="module.brsites table for source mapping")
    ap.add_argument("--csv", help="write per-flip rows to this CSV")
    ap.add_argument(
        "--max-report",
        type=int,
        default=50,
        help="max flip lines to print (default 50)",
    )
    args = ap.parse_args()

    sites = load_sites(args.sites)
    a = list(read_records(args.fp32))
    b = list(read_records(args.fp64))

    flips = []          # (index, site_id, fp32_taken, fp64_taken)
    per_site = Counter()  # site_id -> flip event count
    divergence = None
    n = min(len(a), len(b))

    i = 0
    while i < n:
        s32, t32 = a[i]
        s64, t64 = b[i]
        if s32 != s64:
            divergence = (i, s32, s64)
            break
        if t32 != t64:
            flips.append((i, s32, t32, t64))
            per_site[s32] += 1
        i += 1

    # ---- report -------------------------------------------------------------
    print(f"fp32 trace: {len(a)} branch events")
    print(f"fp64 trace: {len(b)} branch events")
    print(f"compared in lock-step up to event {i}")
    print()

    if not flips and divergence is None:
        print("No branch-decision flips. The two builds took identical paths.")
    else:
        print(f"FLIP EVENTS: {len(flips)}  across {len(per_site)} distinct sites")
        print()
        print("By site (most frequent first):")
        for sid, cnt in per_site.most_common():
            loc = sites.get(sid, "<no site table>")
            print(f"  site {sid:<6} x{cnt:<8} {loc}")
        print()
        print(f"First {min(args.max_report, len(flips))} flip events:")
        for (idx, sid, t32, t64) in flips[: args.max_report]:
            loc = sites.get(sid, "")
            print(
                f"  event#{idx:<8} site {sid:<6} fp32={t32} fp64={t64}  {loc}"
            )
        if len(flips) > args.max_report:
            print(f"  ... {len(flips) - args.max_report} more")

    if divergence is not None:
        idx, s32, s64 = divergence
        l32 = sites.get(s32, "")
        l64 = sites.get(s64, "")
        print()
        print(
            f"CONTROL-FLOW DIVERGENCE at event#{idx}: "
            f"fp32 reached site {s32} ({l32}), fp64 reached site {s64} ({l64})."
        )
        print(
            "  After a real divergence the two streams are no longer aligned, "
            "so lock-step comparison stops here. The flip that caused it is "
            "the last FLIP event above (if any), or occurred just before this "
            "point."
        )

    if args.csv and (flips or divergence):
        import csv

        with open(args.csv, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["event_index", "site_id", "fp32_taken", "fp64_taken", "location"])
            for (idx, sid, t32, t64) in flips:
                w.writerow([idx, sid, t32, t64, sites.get(sid, "")])
            print(f"\nWrote {len(flips)} flip rows to {args.csv}")

    # exit non-zero if any flips found (handy in scripts)
    sys.exit(1 if (flips or divergence) else 0)


if __name__ == "__main__":
    main()
