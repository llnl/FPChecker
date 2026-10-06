# PolyBench FPChecker Timing Overhead

This folder measures FPChecker runtime overhead on the PolyBench benchmarks
without modifying the existing benchmark directories.

For each selected benchmark, optimization level, and precision mode, the script
creates two separate build directories under `timing_overhead/work/`:

1. `native`: built with basic `clang`;
2. `fpchecker`: built with `FPC_INSTRUMENT_ERR_TRACKING=1 clang-fpchecker`
   for FP32, or `FPC_INSTRUMENT_ERR_TRACKING_FP64=1 clang-fpchecker` for FP64.

It compiles with `-DPOLYBENCH_TIME`, runs the `clang` executable once, runs the
FPChecker executable once, and prints one overhead row for that benchmark.

Default coverage:

- `fp32`: normal PolyBench benchmark folders;
- `fp64`: benchmark folders ending in `_fp64`;
- `O0` and `O2`, both with `-fno-vectorize -fno-slp-vectorize`.

MPFR reference folders are skipped because they are not FPChecker wrapper
targets. The skip is case-insensitive, so folders ending in `_mpfr` or `_MPFR`
are not included.

Run from the repository root:

```bash
python3 experiments/benchmark/timing_overhead/run_polybench_overhead.py
```

Useful narrower runs:

```bash
python3 experiments/benchmark/timing_overhead/run_polybench_overhead.py \
  --mode fp32 --opt-level O0 --benchmark linear-algebra/blas/gemm

python3 experiments/benchmark/timing_overhead/run_polybench_overhead.py \
  --mode fp64 --opt-level O2 --benchmark linear-algebra/blas/gemm_fp64
```

If `clang-fpchecker` is installed outside the usual `PATH` or local install
locations, pass it explicitly:

```bash
python3 experiments/benchmark/timing_overhead/run_polybench_overhead.py \
  --fpchecker-wrapper /path/to/clang-fpchecker
```

Printed fields:

- `Filename`: PolyBench benchmark folder;
- `ClangTime(s)`: time from the basic `clang` executable;
- `FPCheckerTime(s)`: time from the FPChecker-wrapper executable;
- `Overhead`: `(FPCheckerTime - ClangTime) / ClangTime`.
