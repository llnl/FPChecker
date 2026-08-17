# Running brtrace on the NPB benchmarks (SP / BT / LU)

Detect fp32-vs-fp64 branch-decision flips across the NAS pseudo-application
solvers. Build each single-file benchmark once as fp32 and once as fp64 with the
BranchTrace pass, run both, and diff the two branch traces. A *flip* is a branch
where the two precisions made different decisions.

This is a dynamic dual-trajectory oracle (your "Method B"): it observes real
branch outcomes from two independently compiled binaries, so it has no
comparison-local blind spot — an accumulation-driven flip surfaces like any
other. (It does share the `fcmp+select` gap; see Caveats.)

## Prerequisites

LLVM 19.1.7 on PATH via the fpchecker env, and the plugin + runtime built once:

```bash
source /usr/workspace/das9/miniconda3/etc/profile.d/conda.sh
conda activate fpchecker_env

cd /usr/workspace/das9/fpchecker_new/cpu_checking/error_analysis/brtrace
bash build.sh          # -> libBranchTrace.so, brtrace_runtime.o
```

## One command for everything

From the `nas` directory (the one containing `sp/ bt/ lu/`):

```bash
cd /usr/workspace/das9/fpchecker_new/cpu_checking/error_analysis/nas
BRX=/usr/workspace/das9/fpchecker_new/cpu_checking/error_analysis/brtrace

# all benchmarks it can find:
$BRX/run_nas.sh

# or specific ones:
$BRX/run_nas.sh sp bt lu
```

That script, for each `<B>` with a `<B>/<B>_fp32/<B>.c` + `<B>/<B>_fp64/<B>.c`
pair:

1. builds fp32 and fp64 with `-O0 -g -fpass-plugin=.../libBranchTrace.so`
2. **checks both builds report the same site count** (aborts that benchmark if
   not — mismatched IDs make the diff meaningless)
3. runs each single-threaded (`OMP_NUM_THREADS=1`) with a distinct `BRTRACE_OUT`
4. diffs the traces and writes a report + CSV

### Outputs (per benchmark)

| file | contents |
|---|---|
| `<B>/fp32.out`, `<B>/fp64.out` | raw branch traces (8-byte records) |
| `<B>/<B>_flips.csv` | one row per flip: event index, site, fp32/fp64 taken, `file:line function` |
| `<B>/<B>_report.txt` | saved diff report |
| `BRTRACE_SUMMARY.txt` | one line per benchmark: sites / events / flips / flip-sites |

## Doing it by hand (one benchmark)

If you'd rather run the steps yourself (this is exactly what the script does):

```bash
BRX=/usr/workspace/das9/fpchecker_new/cpu_checking/error_analysis/brtrace
B=sp

clang -O0 -g -fpass-plugin=$BRX/libBranchTrace.so \
    $B/${B}_fp32/${B}.c $BRX/brtrace_runtime.o -lm -o $B/${B}_fp32/${B}.brx
clang -O0 -g -fpass-plugin=$BRX/libBranchTrace.so \
    $B/${B}_fp64/${B}.c $BRX/brtrace_runtime.o -lm -o $B/${B}_fp64/${B}.brx
# both must print the SAME "[BranchTrace] instrumented N branch sites"

( cd $B/${B}_fp32 && OMP_NUM_THREADS=1 BRTRACE_OUT=../fp32.out ./${B}.brx )
( cd $B/${B}_fp64 && OMP_NUM_THREADS=1 BRTRACE_OUT=../fp64.out ./${B}.brx )

python3 $BRX/tools/brtrace_diff.py $B/fp32.out $B/fp64.out \
    --sites $B/${B}_fp64/${B}.c.brsites --csv $B/${B}_flips.csv
```

The `.brsites` site table is written next to the source as `<B>.c.brsites`
(e.g. `sp/sp_fp64/sp.c.brsites`), because the pass names it after the source
path it was handed. Point `--sites` at the fp64 one.

## Reading the result

Typical NPB signature (observed for SP, BT, LU at class S):

```
FLIP EVENTS: 11  across 3 distinct sites
  site 118  x5  .../sp.c:2377  [verify]     fp32=1 fp64=0   (residual norm exceeds tol in fp32)
  site 122  x5  .../sp.c:2396  [verify]     fp32=1 fp64=0
  site 124  x1  .../sp.c:2409  [verify]     fp32=0 fp64=1   (final verdict / verified flag)
```

Interpretation: the solver is **branch-stable** — millions of solver branches,
zero flips — and *all* fp32/fp64 divergence lands at the final `verify`
tolerance tests. The numerical error is real (~1e-3 relative for these) but
accumulates through arithmetic; the algorithm never branches on the accumulating
quantity until the verify gate. These benchmarks are clean negative controls:
real fp32 divergence, no in-core branch instability.

If a benchmark instead shows flips at solver line numbers (not `verify`), that's
a genuine in-core branch instability worth chasing.

## Caveats

- **`-O0` is required.** Keeps front-end lowering deterministic and identical
  across the two builds so site IDs align. `-O2` would fold/vectorize branches
  and can differ between builds.
- **`fcmp+select` / ternary / `min`/`max` are invisible.** They lower to
  `select`, not a conditional `br`, so brtrace doesn't see them (same
  `isBranchControllingFCmp` gap). "Zero solver flips" therefore means "zero
  solver *branch* flips"; select-form comparisons are untested. To check how
  many a benchmark has:

  ```bash
  clang -O0 -g -S -emit-llvm -o - sp/sp_fp64/sp.c 2>/dev/null | grep -c 'select '
  ```

  If that's ~0 the negative result is airtight. If it's large in solver code,
  the pass needs a `SelectInst` variant to be conclusive.
- **Single-threaded only.** Multi-threaded runs interleave the trace
  non-deterministically and break lock-step. Keep `OMP_NUM_THREADS=1`.
- **Input parity.** Both builds must process the same problem. NPB generates its
  grid deterministically from the class, so this holds here; if you port a
  benchmark whose RNG is precision-typed, keep the RNG in fp64 in both builds.
- **BT NITER lock.** BT's verify reference is locked to the full NITER; the
  short fast-path produces class 'U' and never runs verify. Make sure the BT
  build defaults to the verifying NITER (the run should print residual
  comparisons and a verdict), or the verify branches never execute.

## Class scaling

Results above are class S. Larger classes (W/A) run more iterations and larger
grids; rerun the same way to check whether a bigger problem exposes a
data-dependent solver branch that S doesn't reach. Traces grow ~linearly with
work — class A traces can be large (hundreds of MB); the diff streams them fine.
