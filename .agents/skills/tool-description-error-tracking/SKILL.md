---
name: tool-description-error-tracking
description: Guide for understanding the tool's functionality and usage in error tracking
---

## How the tool works

The tool uses LLVM to instrument program instructions such as:
(1) the arithmetic operations (+, -, *, /)
(2) the load and store instructions
(3) Branch instructions and PHI nodes

Each instruction is instrumented with a call to a function that computes the error of the instruction. 

The error is computed by comparing the result of the instruction in FP32 precision with the result of the same instruction in FP64 precision. 

The tool also tracks the source code location of each instruction, which allows us to identify the specific lines of code that are causing errors.

## Source of the error tracking:

The main source of the error tracking are in the following files:

- driver_error.cpp
- FPC_Hashtable_Error.h
- Instrumentation_error.cpp
- Runtime_error.h

Note that global variablea and functions added as instrumentation are marked with ODR linkage to avoid conflicts with the original program.

## Compiling code with the tool:

To compile code with the tool, you need to use the clang++ compiler.
Users can either use the clang++-fpchecker wrapper or directly use clang++ with the appropriate flags to incude the header files of the runtime and the tool's LLVM plugin.

To know where the wrappers and the header files are located, you can run this command:

```bash
$ fpchecker-show
```

This will show a message like this:

```
========================================
         FPChecker Configuration        
========================================

Installation path: /usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install

Add the following to CFLAGS and/or CXXFLAGS:

(1) For exceptions checking:
-g -include /usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install/src/Runtime_cpu.h -fpass-plugin=/usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install/lib/libfpchecker_cpu.so

(2) For rounding error tracking:
-g -include /usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install/src/Runtime_error.h -fpass-plugin=/usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install/lib/libfpchecker_error.so

Wrappers are located here:
/usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install/bin/clang-fpchecker
/usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install/bin/clang++-fpchecker
/usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install/bin/mpicc-fpchecker
/usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install/bin/mpicxx-fpchecker
```

If the command is not in the PATH, you can add it by running this command:

```bash
export PATH=/usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install/bin:$PATH
```

## Reading the error reports:

The error reports are generated in JSON files stored in .fpc_logs directory. 
Each file corresponds to a specific run of the instrumented program and contains detailed information about the errors detected, including the source code location, the type of error, and the values involved in the computation.
