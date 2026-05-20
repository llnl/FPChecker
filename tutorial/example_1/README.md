# Example 1: LU Solver and NaN/Infinity Detection

This example demonstrates how to use FPChecker to catch excpetion such as NaN.
The sample program is an LU-based linear solver.
You run the same program with two inputs:

- `matrix.csv` (well-behaved case)
- `bad_matrix.csv` (ill-conditioned case that leads to NaN values)

The tutorial goal is to identify where floating-point exceptions originate and how they propagate.

## What this example does

Program file: `lu_solve.cpp`

- Solves linear system `A x = b` with `b = 1`
- Uses LU factorization with partial pivoting (`P A = L U`)
- Solves with forward/backward substitution

In the bad-matrix run, the solver can hit divisions by values near zero, and the result becomes NaN.

## Prerequisites

1. FPChecker wrappers are in `PATH`.
2. Python dependencies for report generation are installed.
3. `clang++` version is compatible with your FPChecker build (expected: LLVM/Clang 19 for this repository setup).

Quick checks:

```bash
fpchecker-show
clang++ --version
```

If you use conda for the tutorial environment:

```bash
conda activate tutorial_env
```

## Build shared tutorial library

From repository root:

```bash
cd tutorial/common
make clean
FPC_INSTRUMENT=1 make
```

Notes:

- `FPC_INSTRUMENT=1` is required if you want exception instrumentation.
- Without `FPC_INSTRUMENT=1`, `clang++-fpchecker` compiles without instrumentation.

## Build and run Example 1

```bash
cd ../example_1
make clean
FPC_INSTRUMENT=1 make
```

### Run the good matrix

```bash
./lu_solve matrix.csv
ls -l .fpc_logs/
fpc-create-report -t "./lu_solve matrix.csv"
open fpc-report/index.html
```

Clean logs/report when needed:

```bash
fpc-create-report -rc
```

### Run the bad matrix

```bash
./lu_solve bad_matrix.csv
ls -l .fpc_logs/
fpc-create-report -t "./lu_solve bad_matrix.csv"
open fpc-report/index.html
```

## Discussion questions for participants

- Which code lines produce NaN values?
- Where does division by zero (or near-zero pivot) occur?
- How does the exception propagate to the final residual norm?

## Troubleshooting

### 1) Build prints plugin-load errors and continues

Symptom (example):

- `unable to load plugin ... libfpchecker_cpu.so`
- `symbol not found ... DisableABIBreakingChecks`

Meaning:

- LLVM/Clang version mismatch between the compiler and the FPChecker plugin.
- The wrapper falls back to normal compilation, so the executable may run but no `.fpc_logs` are generated.

Action:

- Use the supported LLVM/Clang version for FPChecker (tutorial guidance expects Clang 19).
- Rebuild/reinstall FPChecker against that same LLVM toolchain.

### 2) `fpc-create-report` fails with matplotlib import error

Symptom:

- `ModuleNotFoundError: No module named 'matplotlib'`

Meaning:

- Python report dependency is missing in the active environment.

Action:

- Install matplotlib in the environment used to run `fpc-create-report`.

### 3) Program runs but `.fpc_logs` does not exist

Possible causes:

- Instrumentation env variable not set during compilation.
- Plugin load failure (version mismatch), causing fallback compile.

Action:

- Rebuild with `FPC_INSTRUMENT=1` after fixing toolchain compatibility.