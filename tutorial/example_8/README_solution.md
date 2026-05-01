# Example 8: FPChecker Solution

This file shows the FPChecker solution workflow 
for building Hypre and the test with FPChecker,
running a test, and printing reports.

## Files used

- `Makefile.solution`: FPChecker-enabled build for `hypre_test_fpchecker`.
- `matrix.csv`, `matrix_med_cond.csv`, `matrix_cond_num_4K.csv`: sample inputs.

## 0) Prerequisites

Verify tools are available:

```bash
which clang
which mpicc
which mpirun
which fpchecker-show
which fpc-create-report
```

## 1) Build Hypre with FPChecker rounding-error tracking

Configure in `src/build-fpchecker` and pass FPChecker flags through `HYPRE_WITH_EXTRA_CFLAGS`:

From `tutorial/example_8`:

```bash
cd hypre-3.1.0/src
mkdir -p build-fpchecker
cd build-fpchecker
cmake \
  -DCMAKE_C_COMPILER=clang \
  -DHYPRE_ENABLE_MPI=ON \
  -DHYPRE_SINGLE=ON \
  -DHYPRE_ENABLE_MIXED_PRECISION=OFF \
  -DHYPRE_LONG_DOUBLE=OFF \
  "-DHYPRE_WITH_EXTRA_CFLAGS=-g -fno-vectorize -fno-slp-vectorize -include /Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/src/Runtime_error.h -fpass-plugin=/Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/lib/libfpchecker_error.dylib" \
  ..
make -j
```

The `HYPRE_WITH_EXTRA_CFLAGS` value follows the style shown by `fpchecker-show` 
for rounding error tracking.

## 2) Build the test with FPChecker flags

See how FPChecker flags are added in `Makefile.solution`.

Go back to `tutorial/example_8`

```bash
cd ../../..
make -f Makefile.solution HYPRE_SRC_DIR=./hypre-3.1.0/src
```

## 3) Run a FPChecker-instrumented test

```bash
mpirun -np 1 ./hypre_test_fpchecker ./matrix.csv pcg
```

Solver options: `amg`, `pcg`, `amg_pcg`.

Alternative inputs:

```bash
mpirun -np 1 ./hypre_test_fpchecker ./matrix_med_cond.csv pcg
mpirun -np 1 ./hypre_test_fpchecker ./matrix_cond_num_4K.csv pcg
```

## 4) Print reports

Text report:

```bash
fpc-create-report -s
```

HTML report:

```bash
fpc-create-report
open fpc-report/index.html
```

Cleanup traces/report artifacts:

```bash
fpc-create-report -cr
```

## Notes

- This workflow does not use wrapper compilers (`clang-fpchecker` or `clang++-fpchecker`).
- If plugin loading fails, use a Clang/LLVM toolchain compatible with the FPChecker plugin binary.
