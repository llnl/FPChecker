#!/usr/bin/env python3
"""
compare_sites.py -- assert the nsan plugin and brtrace agree on site identity.

    ./compare_sites.py --nsan <dir-or-glob> --brt <dir-or-glob>
    ./compare_sites.py --nsan .../results/O0/fp64/nsansites \
                       --brt  .../gt_experiments/lulesh/build/O0/lulesh_fp64
    ./compare_sites.py --nsan ... --brt ... --kind select

Exits 0 only if every module's site enumeration is IDENTICAL: same module_id,
same count, same file:line:col for every site_id. Anything else is a nonzero
exit and a printed explanation.

WHY THIS HAS TO RUN BEFORE ANY EXPERIMENT
-----------------------------------------
site_id is a positional ordinal from a deterministic walk. If the nsan plugin
admits or rejects one branch that brtrace does not, every id after that point
shifts by one -- and a shifted id is still a well-formed id. It joins. It
produces a confusion matrix. The numbers look ordinary and are entirely wrong,
with no downstream symptom to notice.

The nsan plugin's isFPControlled/firstFCmp/collectFCmps are transcribed from
BranchTrace_mtu.cpp rather than reimplemented, precisely so this check can be
expected to pass. It is asserted anyway, per build, because transcription can
still drift and because the LLVM the two are built against can differ.

This is the nsan counterpart of check_sites.py, which does the same job for
FPChecker. It is separate only because check_sites.py reads .fpcsites/.brsites
by extension.

WHAT IS COMPARED
----------------
  module_id  FNV-1a-32 over the module basename. A disagreement shifts every
             id in the module at once and usually means the two sides hashed
             different strings -- getModuleIdentifier() versus
             getSourceFileName(), or a path where a basename was wanted.

  n_sites    Per module. A disagreement means the enumeration walks differ.
             This should NOT depend on the optimisation level: both walks run
             at PipelineStartEP, before any optimisation.

  location   file:line:col per site_id. Matching counts with differing
             locations means the walks agree on WHICH branches to number but
             disagree on how to LABEL them -- normally a loc_anchor or column
             difference. Both tables must say `loc_anchor fcmp`.

  n_fcmp     Informational. >1 marks sites where several comparisons control
             one branch; those tick more than once per execution, so their
             occurrence indices are inflated and they must be excluded from
             event-level joins even when the site tables agree perfectly.

  table version  Both sides must be version 4. A v3 table anchors labels on
             the terminator and a v2 census predates the phi rule; mixing
             versions compares two different site conventions.
"""

import argparse
import glob
import os
import signal
import sys

try:
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)
except (AttributeError, ValueError):
    pass


def load(pattern, ext):
    """Parse side tables -> {module_id: {path, version, anchor, sites}}

    sites maps site_id -> (location, function, n_fcmp).
    """
    if os.path.isdir(pattern):
        paths = glob.glob(os.path.join(pattern, "**", "*" + ext),
                          recursive=True)
    else:
        paths = glob.glob(pattern + "*" + ext) or glob.glob(pattern)
    out = {}
    for p in sorted(paths):
        mid = None
        version = None
        anchor = None
        modname = os.path.basename(p)
        sites = {}
        with open(p) as fh:
            for line in fh:
                line = line.rstrip("\n")
                if line.startswith("#"):
                    parts = line.lstrip("#").split()
                    if len(parts) >= 2:
                        if parts[0] in ("module_id",):
                            mid = int(parts[1])
                            if len(parts) >= 4 and parts[2] == "module":
                                modname = os.path.basename(parts[3])
                        elif parts[0].endswith("table-version"):
                            version = parts[1]
                        elif parts[0] == "loc_anchor":
                            anchor = parts[1]
                    continue
                if not line.strip():
                    continue
                f = line.split("\t")
                if len(f) < 4:
                    continue
                try:
                    sid = int(f[0])
                except ValueError:
                    continue
                sites[sid] = (f[1], f[2], f[3])
        if mid is None:
            print(f"  ! {p}: no module_id header; skipped")
            continue
        out[mid] = {"path": p, "module": modname, "version": version,
                    "anchor": anchor, "sites": sites}
    return out


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--nsan", required=True,
                    help="directory or glob holding .nsansites/.nsanselsites")
    ap.add_argument("--brt", required=True,
                    help="directory or glob holding .brsites/.brselsites")
    ap.add_argument("--kind", choices=["branch", "select"], default="branch")
    ap.add_argument("--max-show", type=int, default=15,
                    help="how many differing sites to print per module")
    ap.add_argument("--expect", type=int, default=None,
                    help="total site count both sides must report "
                         "(LULESH branch: 75)")
    args = ap.parse_args()

    if args.kind == "branch":
        n_ext, b_ext = ".nsansites", ".brsites"
    else:
        n_ext, b_ext = ".nsanselsites", ".brselsites"

    N = load(args.nsan, n_ext)
    B = load(args.brt, b_ext)

    print(f"kind: {args.kind}")
    print(f"nsan: {len(N)} module(s) from {args.nsan}")
    print(f"brt : {len(B)} module(s) from {args.brt}")
    print()

    if not N and not B:
        print("Neither side emitted tables of this kind. For selects at -O0 "
              "that is the expected result: std::max only becomes a select "
              "once inlined and folded, which is an -O2 phenomenon. Nothing "
              "to compare, nothing wrong.")
        return 0
    if not N or not B:
        missing = "nsan" if not N else "brtrace"
        other = B if not N else N
        if all(not d["sites"] for d in other.values()):
            print(f"{missing} emitted no tables of this kind, and the other "
                  f"side's tables are all empty. Both agree there are no "
                  f"{args.kind} sites.")
            return 0
        print(f"FAIL: {missing} has no tables but the other side numbered "
              f"sites. Check the paths and that the build emitted them.")
        return 2

    bad = 0

    # Paths carry the conda prefix of whichever env did the build, so an
    # identical site reads as a mismatch purely because one side compiled in
    # nsan_env and the other in fpchecker_env. Compare from the last path
    # component that both sides share -- the file:line:col that identifies the
    # site -- not the absolute path to a toolchain header.
    def loc_key(loc):
        # "some/long/path/stl_algobase.h:263:15" -> "stl_algobase.h:263:15"
        head, sep, tail = loc.rpartition("/")
        return tail if sep else loc

    # --- table conventions -------------------------------------------------
    for label, T in (("nsan", N), ("brt", B)):
        for mid, d in T.items():
            if d["version"] not in (None, "4"):
                print(f"FAIL: {label} {d['module']} is table version "
                      f"{d['version']}, expected 4. A v3 table anchors labels "
                      f"on the terminator and a v2 census predates the phi "
                      f"rule; the conventions are not comparable.")
                bad += 1
            if d["anchor"] not in (None, "fcmp"):
                print(f"FAIL: {label} {d['module']} has loc_anchor "
                      f"{d['anchor']}, expected fcmp. Every site's column "
                      f"will disagree.")
                bad += 1

    # --- module coverage ---------------------------------------------------
    # brtrace writes a side table only when a module actually has sites
    # (`if (!SiteTable.empty())`); the nsan plugin writes one unconditionally.
    # So a module present on one side with ZERO sites is a difference in when a
    # file gets created, not in what was enumerated. Only a module carrying
    # sites on one side and missing entirely from the other is a real problem.
    only_n = set(N) - set(B)
    only_b = set(B) - set(N)
    empty_only = 0
    for mid in sorted(only_n):
        if not N[mid]["sites"]:
            empty_only += 1
            continue
        print(f"FAIL: module_id {mid} ({N[mid]['module']}) has "
              f"{len(N[mid]['sites'])} site(s) in nsan, absent from brtrace.")
        bad += 1
    for mid in sorted(only_b):
        if not B[mid]["sites"]:
            empty_only += 1
            continue
        print(f"FAIL: module_id {mid} ({B[mid]['module']}) has "
              f"{len(B[mid]['sites'])} site(s) in brtrace, absent from nsan.")
        bad += 1
    if empty_only:
        print(f"note: {empty_only} module(s) present on one side only, with "
              f"no sites. brtrace omits the table entirely when a module has "
              f"none; the plugin always writes one. Not a disagreement.")
    if bad:
        print("      A module_id mismatch shifts every id in that module at "
              "once. Usually the two sides hashed different strings.")
    print()

    # --- per-module comparison --------------------------------------------
    tot_n = tot_b = 0
    multi_fcmp = []
    for mid in sorted(set(N) & set(B)):
        n, b = N[mid], B[mid]
        ns, bs = n["sites"], b["sites"]
        tot_n += len(ns)
        tot_b += len(bs)
        head = f"{b['module']}  (mod {mid})"

        if len(ns) != len(bs):
            print(f"FAIL {head}: nsan {len(ns)} sites, brtrace {len(bs)}.")
            print("      The enumeration walks differ. Since site_id is a "
                  "positional ordinal, every id after the first disagreement "
                  "is wrong.")
            bad += 1
        else:
            print(f"ok   {head}: {len(ns)} sites")

        diffs = []
        for sid in sorted(set(ns) | set(bs)):
            a = ns.get(sid)
            c = bs.get(sid)
            if a is None:
                diffs.append((sid, "-- missing in nsan --", c[0]))
            elif c is None:
                diffs.append((sid, a[0], "-- missing in brtrace --"))
            elif loc_key(a[0]) != loc_key(c[0]):
                diffs.append((sid, a[0], c[0]))
            else:
                if a[2] != c[2]:
                    print(f"     site {sid}: n_fcmp differs "
                          f"(nsan {a[2]}, brtrace {c[2]})")
                    bad += 1
                if a[2] not in ("0", "1"):
                    multi_fcmp.append((mid, sid, a[2]))

        if diffs:
            bad += 1
            print(f"     {len(diffs)} site(s) label differently:")
            for sid, x, y in diffs[:args.max_show]:
                print(f"       {sid:6d}  nsan={x}")
                print(f"               brt ={y}")
            if len(diffs) > args.max_show:
                print(f"       ... {len(diffs) - args.max_show} more")

    print()
    print(f"totals: nsan {tot_n}, brtrace {tot_b}")
    if args.expect is not None:
        for label, t in (("nsan", tot_n), ("brtrace", tot_b)):
            if t != args.expect:
                print(f"FAIL: {label} total is {t}, expected {args.expect}.")
                bad += 1

    if multi_fcmp:
        print()
        print(f"note: {len(multi_fcmp)} site(s) have n_fcmp > 1. These tick "
              f"once per controlling comparison, so their occurrence indices "
              f"are inflated relative to the oracle's. Exclude them from "
              f"event-level joins even though the tables agree:")
        for mid, sid, nf in multi_fcmp[:args.max_show]:
            print(f"       mod {mid} site {sid}  n_fcmp={nf}")

    print()
    if bad:
        print(f"RESULT: {bad} problem(s). The site spaces are NOT "
              f"interchangeable; do not score any run from this build.")
        return 1
    print("RESULT: site enumerations are identical. (module_id, site_id) "
          "joins directly against the oracle.")
    return 0


if __name__ == "__main__":
    sys.exit(main())