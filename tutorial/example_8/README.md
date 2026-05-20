# Example 8: Hypre Build and Test

This example shows how to download Hypre, build it without FPChecker,
build the test, and run one input.

The FPChecker solution workflow is described in `README_solution.md`.

## Files Used

- `hypre_test.c`: Driver program.
- `Makefile`: Builds `hypre_test` against a plain Hypre build.
- `matrix.csv`, `matrix_med_cond.csv`, `matrix_cond_num_4K.csv`: Sample inputs.

## 0) Prerequisites

Verify tools are available:

```bash
which clang
which mpicc
which mpirun
```

## 1) Download Hypre source

From `tutorial/example_8`:

```bash
curl -L -o hypre-3.1.0.tar.gz https://github.com/hypre-space/hypre/archive/refs/tags/v3.1.0.tar.gz
tar -xzf hypre-3.1.0.tar.gz
```

## 2) Build Hypre without FPChecker

Configure in `src/build`, then compile with `make -j`:

```bash
cd hypre-3.1.0/src
mkdir -p build
cd build
cmake \
  -DCMAKE_C_COMPILER=clang \
  -DHYPRE_ENABLE_MPI=ON \
  -DHYPRE_SINGLE=ON \
  -DHYPRE_ENABLE_MIXED_PRECISION=OFF \
  -DHYPRE_LONG_DOUBLE=OFF \
  ..
make -j
```

## 3) Build The Test

Go back to `tutorial/example_8`:

```bash
cd ../../..
make HYPRE_SRC_DIR=./hypre-3.1.0/src
```

## 4) Run a Test with an Input

```bash
mpirun -np 1 ./hypre_test ./matrix.csv pcg
```

Solver options: `amg`, `pcg`, `amg_pcg`.

Optional inputs:

```bash
mpirun -np 1 ./hypre_test ./matrix_med_cond.csv pcg
mpirun -np 1 ./hypre_test ./matrix_cond_num_4K.csv pcg
```

## Challenge

Run `fpchecker-show` and inspect the flags needed to instrument
the code with FPChecker for rounding error tracking.

Identify what needs to change to:

1. Build Hypre with FPChecker instrumentation flags.
2. Build the test with FPChecker instrumentation flags.

Tips:

- To instrument Hypre, pass extra FPChecker flags through
  `"-DHYPRE_WITH_EXTRA_CFLAGS=... "`.

- To instrument the Hypre test, copy `Makefile` into a new file
  (for example, `Makefile.new`). Then add a variable with the extra flags,
  such as:
  `FPCHECKER_FLAGS ?= ...`
  Include this variable in the target compilation flags.

- Clean the build: `make -f Makefile.new clean`

- If the Hypre build directory changed, it needs to be modified in HYPRE_BUILD_DIR

## Solution

Compare your result with `README_solution.md` and `Makefile.solution`.
