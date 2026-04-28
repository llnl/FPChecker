# Example 2: Dynamic Range Analysis in Reaction-Diffusion

This example demonstrates why a simulation that appears to run in FP32 can still be numerically unsafe.
You will run the same PDE solver in FP64 and FP32, then compare FPChecker reports.

## Learning goals

- Run FPChecker on a time-dependent PDE code.
- Inspect exponent usage to evaluate precision safety.
- Observe FP32 overflow behavior (NaN and Infinity events).
- Conclude that this case should remain in FP64.

## Program overview

Source file: `reaction_diffusion.cpp`

- 1D linear reaction-diffusion equation
- PDE: du/dt = D * d2u/dx2 + lambda * u
- Explicit finite-difference update
- Large positive `lambda` amplifies the solution over time
- Magnitude growth eventually exceeds FP32 range

## Environment setup

Check your tools:

```bash
clang++ --version
which clang++-fpchecker
which fpc-create-report
python3 -c "import matplotlib; print(matplotlib.__version__)"
```

## Part A: FP64 run (default code)

By default, the code is set to FP64:

```cpp
typedef double Real_t;
// typedef float Real_t;
```

Run:

```bash
make clean
FPC_INSTRUMENT=1 make
FPC_EXPONENT_USAGE=1 ./reaction_diffusion
ls -l .fpc_logs/
fpc-create-report
open fpc-report/index.html
```

Expected artifacts:

- `.fpc_logs/exponent_usage_*.json`
- `.fpc_logs/fpc_*.json`
- `fpc-report/index.html`

Expected interpretation:

- FP64 simulation completes.
- Exponent histogram shows values that exceed FP32 safe range (about 3.402e38).

Clean between experiments:

```bash
fpc-create-report -rc
```

## Part B: FP32 run (participant challenge)

Edit `reaction_diffusion.cpp` and switch precision type:

```cpp
// typedef double Real_t;
typedef float Real_t;
```

Re-run the same script:

```bash
make clean
FPC_INSTRUMENT=1 make
FPC_EXPONENT_USAGE=1 ./reaction_diffusion
ls -l .fpc_logs/
fpc-create-report
open fpc-report/index.html
```

Expected interpretation:

- FP32 run produces NaN/Infinity-related events in the report.
- The report first page and event counts indicate FP32 is not sufficient for this workload.

After the challenge, switch code back to FP64:

```cpp
typedef double Real_t;
// typedef float Real_t;
```

## Questions for participants

- At what magnitude does FP32 become unsafe in this example?
- Which report sections indicate FP32 overflow risk?

## Instructor script (quick copy/paste)

FP64:

```bash
make clean
FPC_INSTRUMENT=1 make
FPC_EXPONENT_USAGE=1 ./reaction_diffusion
ls -l .fpc_logs/
fpc-create-report
open fpc-report/index.html
fpc-create-report -rc
```

FP32:

```bash
# Edit reaction_diffusion.cpp: use typedef float Real_t;
make clean
FPC_INSTRUMENT=1 make
FPC_EXPONENT_USAGE=1 ./reaction_diffusion
ls -l .fpc_logs/
fpc-create-report
open fpc-report/index.html
fpc-create-report -rc
```

## Troubleshooting

1) Build shows plugin-load errors and no `.fpc_logs` are produced.

- Cause: `clang++` points to wrong LLVM/Clang version.

2) `fpc-create-report` fails with matplotlib import error.

- Cause: `python3` used by script does not have matplotlib.
- Fix: ensure `python3` resolves to the tutorial environment Python.

3) Program runs but report is empty or missing expected sections.

- Confirm both variables are used:
  - `FPC_INSTRUMENT=1` at compile time
  - `FPC_EXPONENT_USAGE=1` at runtime