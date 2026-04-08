# Example 6: Report-driven mixed-precision refinement

This example shows how to use FPChecker's rounding-error report to
**selectively promote** FP32 code to FP64.  Only the lines flagged with
high relative error are changed; everything else stays in FP32.

## Workflow overview

1. Build and run the FP32 program with FPChecker instrumentation.
2. Read the rounding-error report — identify lines with high relative error.
3. Construct a mixed-precision version that promotes **only** the flagged regions.
4. Compare both versions against a full-FP64 reference to verify improvement.

## Files

| File | Purpose |
|------|---------|
| `fp32.cpp` | All computation in FP32 (instrumented by FPChecker) |
| `mixed.cpp` | Selective FP64 promotion driven by the FPChecker report |
| `fp64.cpp` | Full FP64 reference (ground truth) |
| `compare_accuracy.py` | Runs all three and prints relative errors |

## Step 1 — Build and run the FP32 version

The instrumented build uses `-O0` so the compiler does not constant-fold
away the floating-point operations we want to measure.

```bash
make clean
make fp32          # compiles with FPC_INSTRUMENT_ERR_TRACKING=1 at -O0
make run-fp32      # runs:  ./fp32 200000 32 1.0 1.0e8 1.0
```

## Step 2 — Read the rounding-error report

```bash
make report-shell
```

Sample output (line numbers refer to `fp32.cpp`):

```
Line   | Code                                    |    Rel. Error
----------------------------------------------------------------
  49   | disc = b * b - 4.0f * a * c             |  ~3e-08
  50   | sqrt_d = sqrtf(disc)                     |  ~2e-16
  51   | numer = -b + sqrt_d                      |  1.0  (total loss!)
  52   | root_small = numer / (2.0f * a)          |  1.0  (propagated)
  24   | sum += x[i]                              |  ~7e-04
  25   | sumsq += x[i] * x[i]                    |  ~7e-04
  29   | mean = sum / n                           |  ~7e-04
  30   | (sumsq / n) - (mean * mean)              |  ~1e+10  (catastrophic!)
  40   | wave = amp * sinf(...)                   |  ~1e-05
  42   | x[i] = trend + wave + jitter             |  ~2e-08
```

### Interpreting the report

**High error — must promote to FP64:**

- **Lines 49–52** (quadratic root): The subtraction `-b + sqrt(disc)` at line 51
  has relative error **1.0** (100% loss of significant digits) because
  `b ≈ sqrt(disc)`.  The error propagates to line 52.  Promote to FP64 and
  use Vieta's formula `c / (a * large_root)` to avoid the cancellation.

- **Lines 24–25, 29–30** (variance): The one-pass formula `E[x²] − E[x]²`
  subtracts two nearly-equal large numbers at line 30, producing relative
  error ~1e+10.  Promote accumulators to FP64 and switch to a two-pass
  algorithm.

**Low error — keep as FP32:**

- **Lines 40, 42** (data generation): Relative errors are ≤ 1e-05.  The
  data array stays `float` since storage precision is the constraint and
  the report confirms these operations are fine.

## Step 3 — Build the mixed-precision version

`mixed.cpp` applies exactly the promotions identified above.  See the
comments at the top of that file for a detailed mapping from report lines
to code changes.

```bash
make mixed fp64
```

## Step 4 — Compare accuracy

```bash
make compare
```

Expected output:

```
=== Relative error (fp32 vs fp64) ===
root_small: 1.000000e+00        # total loss
variance:   ~1.5e+10            # catastrophic

=== Relative error (mixed vs fp64) ===
root_small: 0.000000e+00        # exact match
variance:   ~3.5e-02            # dramatically improved
```

## Notes

- The FP32 build uses `-O0` to prevent the compiler from constant-folding
  or inlining the floating-point operations.  Mixed and FP64 builds use
  `-O2` since they are not instrumented.
- The quadratic coefficients (`a`, `b`, `c`) are passed via command-line
  arguments so they remain as runtime values even under optimization.
- Functions use `__attribute__((noinline))` for clear source-line attribution
  in the FPChecker report.
