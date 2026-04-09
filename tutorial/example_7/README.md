# Example: Report-driven mixed-precision Conjugate Gradient solver

This example shows how to use FPChecker's rounding-error report to
**selectively promote** a Conjugate Gradient (CG) solver from FP32 to a
mixed-precision version.  Only the lines flagged with high relative error
are promoted to FP64 arithmetic; all matrix and vector storage stays in
FP32.

## Workflow overview

1. Build and run the FP32 CG solver with FPChecker instrumentation.
2. Read the rounding-error report — identify operations with high relative error.
3. Construct a mixed-precision version that promotes **only** the flagged operations.
4. Compare all three versions (FP32, mixed, FP64) on the same input matrix.

## Files

| File | Purpose |
|------|---------|
| `cg.cpp` | All-FP32 CG solver (instrumented by FPChecker) |
| `cg_mixed.cpp` | Selective FP64 promotion driven by the FPChecker report |
| `cg_FP64.cpp` | Full FP64 reference (ground truth) |
| `driver.py` | Generates tridiagonal SPD test matrices with controlled condition numbers |
| `matrices/` | Pre-generated 100×100 matrices with κ from ~1 to ~4×10⁸ |
| `Makefile` | Builds all three variants |

## Step 1 — Generate test matrices

The script creates tridiagonal Toeplitz SPD matrices with varying
condition numbers.  Higher κ makes CG harder and amplifies rounding error.

```bash
python driver.py
```

This populates `matrices/` with CSV files named by condition number.

## Step 2 — Build the FP32 version with instrumentation

```bash
make clean
make cg            # compiles cg.cpp with FPC_INSTRUMENT_ERR_TRACKING=1
```

## Step 3 — Run FP32 and generate the rounding-error report

```bash
export CG_MATRIX=matrices/5_matrix_3.998e+04.csv
./cg 500 1e-6
fpc-create-report -s rounding
```

Sample output (line numbers refer to `cg.cpp`):

```
Line   | Code                                       | Rel. Error
-----------------------------------------------------------------
 230   | alpha = rs_old / dot_product(p, Ap)         | 2.36e+05  (highest!)
  52   | result += v1[i] * v2[i]                     | 1.0
 111   | result[i] = r[i] + beta * p[i]              | 1.0
  90   | result[i] = v1[i] - alpha * v2[i]           | 1.0
  39   | guess = 0.5f * (guess + x / guess)          | 1.0
  75   | result[i] += A[i*n+j] * v[j]                | 1.04
  97   | result[i] = v1[i] + alpha * v2[i]           | 0.83
 249   | beta = rs_new / rs_old                      | 0.33
```

### Interpreting the report

**High error — promoted to FP64 intermediates:**

- **Line 52** (`dot_product`): The FP32 accumulation `result += v1[i]*v2[i]`
  loses nearly all significant digits (rel err 1.0).  This cascades into
  `alpha` (line 230) and `beta` (line 249).  **Fix:** accumulate in a
  `double` variable, return `double`.

- **Line 230** (`alpha = rs_old / dot_product(p, Ap)`): Relative error
  ~2.4×10⁵ — the single worst line.  A tiny error in the denominator
  is amplified by the division.  **Fix:** compute `alpha` as `double`
  (follows from the FP64 dot product).

- **Lines 90, 97** (residual and solution vector updates): FP32
  multiply-add `v1[i] ± alpha * v2[i]` has rel err ~1.0.  **Fix:**
  cast operands to `double` before arithmetic, store back as `float`
  (for residual) or `double` (for solution `x`).

- **Line 111** (search direction update): Same pattern as above.
  **Fix:** FP64 intermediate, store as `float`.

- **Line 75** (`mat_vec_mult`): Row accumulation `result[i] += A*v`
  has rel err 1.04.  **Fix:** accumulate each row in a `double`
  variable, then cast the final sum back to `float`.

- **Line 39** (`my_sqrt`): Newton iteration in FP32 loses precision.
  **Fix:** promote to `double`.

**Kept as FP32 (low or zero error):**

- Matrix `A` storage, vectors `b`, `r`, `p`, `Ap` storage, `loadMatrix`,
  and all I/O remain in FP32.

## Step 4 — Build the mixed-precision and FP64 versions

```bash
make cg_mixed cg_FP64   # no instrumentation, plain builds
```

`cg_mixed.cpp` applies exactly the promotions listed above.  See the
comments at the top of that file for a line-by-line mapping from the
report to each code change.

## Step 5 — Compare accuracy

```bash
export CG_MATRIX=matrices/5_matrix_3.998e+04.csv

echo "=== FP32 ==="
./cg 500 1e-6

echo "=== Mixed ==="
./cg_mixed 500 1e-6

echo "=== FP64 ==="
./cg_FP64 500 1e-6
```

Expected output (κ ≈ 4×10⁴ matrix):

```
                 FP32          Mixed         FP64
Iterations:      144           77            50
||Ax - b||:      3.25e-02      5.17e-03      1.91e-06
```

The mixed-precision version:
- Converges in **77 iterations** (47% fewer than FP32).
- Achieves a final residual of **5.17×10⁻³** — a **6.3× improvement**
  over FP32's 3.25×10⁻².
- Keeps all matrix and vector storage in FP32; only scalar and
  accumulator arithmetic is promoted to FP64.

## What changed between cg.cpp and cg_mixed.cpp

| Change | Report Line | Rel. Error |
|--------|-------------|-----------|
| `dot_product` returns `double`, accumulates in `double` | 52 | 1.0 |
| `alpha`, `beta`, `rs_old`, `rs_new` are `double` | 230, 249 | 2.4e5, 0.33 |
| `mat_vec_mult` row accumulation in `double` | 75 | 1.04 |
| `vec_add_mult` casts to `double` before arithmetic | 90, 97 | 1.0, 0.83 |
| `vec_add_scaled` casts to `double` before arithmetic | 111 | 1.0 |
| Solution vector `x` stored as `vector<double>` | — | drift prevention |
| `my_sqrt` computed in `double` | 39 | 1.0 |

No algorithmic changes — the CG iteration is identical.

## Notes

- Only `cg.cpp` is compiled with `FPC_INSTRUMENT_ERR_TRACKING=1`.
  The mixed and FP64 builds are plain (no instrumentation overhead).
- The `-fno-vectorize -fno-slp-vectorize` flags ensure the compiler
  does not auto-vectorize, keeping source-line attribution accurate.
- The `conjugate_gradient` function uses `__attribute__((noinline))` for
  clear source-line attribution in the FPChecker report.  Helper functions
  use `__attribute__((always_inline))` so their operations are attributed
  to the call site inside `conjugate_gradient`.
- The `CG_MATRIX` environment variable selects the input matrix.
  Try different condition numbers to see how rounding error scales.
