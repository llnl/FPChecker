---
name: tool-description-error-tracking
description: "Compile C/C++ code with FPChecker's LLVM plugin for floating-point rounding error tracking, run instrumented programs, and interpret JSON error reports. Use when the user asks about error tracking, rounding errors, FP32 vs FP64 comparison, fpchecker instrumentation, compiling with fpchecker, or reading error reports in .fpc_logs/."
---

## Workflow

### 1. Ensure FPChecker is in PATH

```bash
export PATH=$PROJECT_ROOT/build/install/bin:$PATH
fpchecker-show
```

`fpchecker-show` prints the installation path, compiler flags, and available wrappers.

### 2. Compile with error tracking

Use the wrapper for convenience:

```bash
clang++-fpchecker -o program program.cpp
```

Or pass flags directly (paths shown by `fpchecker-show`):

```bash
clang++ -g \
  -include $INSTALL_PATH/src/Runtime_error.h \
  -fpass-plugin=$INSTALL_PATH/lib/libfpchecker_error.so \
  -o program program.cpp
```

For exception checking instead of rounding error tracking, swap to `Runtime_cpu.h` and `libfpchecker_cpu.so`.

MPI wrappers are also available: `mpicc-fpchecker`, `mpicxx-fpchecker`.

### 3. Run the instrumented program

```bash
./program
```

Error reports are written to `.fpc_logs/` in the working directory as JSON files (one per run).

### 4. Read the error reports

```bash
ls .fpc_logs/
```

Each JSON file contains the source code location, error type, and values involved in the computation for every detected floating-point issue.

## How error tracking works

FPChecker's LLVM plugin instruments arithmetic operations (+, -, *, /), load/store instructions, and branch/PHI nodes. Each instrumented instruction calls a runtime function that computes rounding error by comparing FP32 results against FP64 equivalents.

The core computation is `_FPC_FP32_CALCULATE_ERROR_` in `Runtime_error.h`: operands `(float x, float y, float z, float w)` are the result and operands of the instruction (for add: `x = y + z`; `w` is used for three-operand instructions like FMA: `x = y * z + w`). The `int op` parameter identifies the operation type (add, sub, mul, div, etc.).

Global variables and instrumentation functions use ODR linkage to avoid conflicts with the original program.

## Key source files

- `Runtime_error.h` — runtime error computation (`_FPC_FP32_CALCULATE_ERROR_`)
- `driver_error.cpp` — driver for error tracking mode
- `Instrumentation_error.cpp` — LLVM pass that instruments instructions
- `FPC_Hashtable_Error.h` — hash table for tracking error locations by source line
