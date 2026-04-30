# Example 8: Hypre Build and Test (Participant Workflow)

This README is for participants.
It shows how to download Hypre, build it without FPChecker, build the test, and run one input.

The FPChecker solution is in `README_solution.md`.

## Files used

- `hypre_test.c`: driver program.
- `Makefile`: builds `hypre_test` against a plain Hypre build.
- `matrix.csv`, `matrix_med_cond.csv`, `matrix_cond_num_4K.csv`: sample inputs.

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

## 3) Build the test

Back in `tutorial/example_8`:

```bash
make HYPRE_SRC_DIR=$(pwd)/hypre-3.1.0/src
```

## 4) Run a test with an input

```bash
mpirun -np 1 ./hypre_test $(pwd)/matrix.csv 1
```

Optional inputs:

```bash
mpirun -np 1 ./hypre_test $(pwd)/matrix_med_cond.csv 1
mpirun -np 1 ./hypre_test $(pwd)/matrix_cond_num_4K.csv 1
```

## Challenge

Look at `fpchecker-show` and identify what needs to change to:

1. Build Hypre with FPChecker instrumentation.
2. Build the test with FPChecker flags.

Then compare your result with `README_solution.md` and `Makefile.solution`.
