# Example 7: Data-Driven Mixed-Precision Conjugate Gradient

This example shows how to use FPChecker rounding-error reports to guide selective 
FP64 promotion in a Conjugate Gradient (CG) solver.

Example objectives:

1. Run the baseline FP32 solver with instrumentation.
2. Identify high-error lines from the report.
3. Compare FP32, mixed, and FP64 behavior.
4. Explain why targeted mixed precision improves convergence and residual.

## What this example includes

| File                      | Purpose                                                         |
|---------------------------|-----------------------------------------------------------------|
| `cg.cpp`                  | FP32 solver instrumented with FPChecker rounding-error tracking |
| `cg_mixed.cpp`            | Mixed-precision solver (targeted FP64 intermediates)            |
| `cg_FP64.cpp`             | FP64 reference solver                                           |
| `generate_cg_matrices.py` | Matrix generation utility                                       |
| `matrices/`               | Pre-generated SPD matrices used in the tutorial                 |
| `Makefile`                | Build rules for all variants                                    |

## Participant challenge flow

For this tutorial session, matrices are already available in `matrices/`.
Participants do not need to generate them.

### Challenge 1: run FP32 and inspect rounding errors

```bash
make clean
make cg
./cg matrices/5_matrix_3.998e+04.csv 500 1e-6
fpc-create-report -s rounding
```

Representative FP32 report snippet (relative error only):

```text
Line  | Code                                                    | Rel. Error
------+---------------------------------------------------------+------------
39    | guess = 0.5f * (guess + x / guess);                     | 9.999589e-01
52    | result += v1[i] * v2[i];                                | 1.000000e+00
75    | result[i] += A[i * n + j] * v[j];                       | 1.443186e+00
90    | result[i] = v1[i] - alpha * v2[i];                      | 9.998595e-01
97    | result[i] = v1[i] + alpha * v2[i];                      | 4.621049e+00
111   | result[i] = r[i] + beta * p[i];                         | 1.000000e+00
229   | float alpha = rs_old / dot_product(p, Ap);              | 2.768030e+05
239   | float relative_residual = my_sqrt(rs_new) / relative_b; | 1.000000e+00
248   | float beta = rs_new / rs_old;                           | 2.857829e-01
265   | (no source text in report)                              | 1.000000e+00
```

### Questions

- Which lines have the largest relative error?
- Which operations are likely causing cancellation or unstable accumulation?
- Which parts can remain FP32?
- Which variables should move to FP64?
- Which kernels need FP64 accumulation?

## Mixed and FP64 cases

```bash
make cg_mixed cg_FP64

printf "\n=== FP32 ===\n"
./cg matrices/5_matrix_3.998e+04.csv 500 1e-6

printf "\n=== Mixed ===\n"
./cg_mixed matrices/5_matrix_3.998e+04.csv 500 1e-6

printf "\n=== FP64 ===\n"
./cg_FP64 matrices/5_matrix_3.998e+04.csv 500 1e-6
```

## Changes made based on the report

Representative high-error lines in `cg.cpp`:

- Line 229: `alpha = rs_old / dot_product(p, Ap)`
- Line 239: `relative_residual = my_sqrt(rs_new) / relative_b`
- Line 52: dot-product accumulation
- Lines 90 and 97: vector fused updates
- Line 111: search-direction update
- Line 75: matrix-vector row accumulation
- Line 39: FP32 Newton-style sqrt routine

Key interpretation:

- FP32 accumulation and sensitive divisions amplify error.
- Targeted FP64 intermediates reduce propagation without converting all storage to FP64.

## Expected comparison outcome

For the tutorial matrix (kappa about 4e4), a representative run is:

```text
                 FP32          Mixed         FP64
Iterations:      148           77            50
||Ax - b||:      3.15e-02      5.17e-03      1.91e-06
```

Small differences in the FP32 iteration count can occur across toolchains and environments.

Takeaway:

- Mixed precision significantly improves convergence and residual versus FP32.
- Most storage remains FP32; only sensitive arithmetic is promoted.


## Optional: matrix generation (not part of participant exercise)

Matrices are pre-generated for the tutorial.
If you need to regenerate them, use the matrix generator script.

Run:

```bash
python generate_cg_matrices.py
```

## Notes

- Only `cg.cpp` is built with `FPC_INSTRUMENT_ERR_TRACKING=1`.
- `cg_mixed.cpp` and `cg_FP64.cpp` are plain builds (no instrumentation overhead).
- The `-fno-vectorize -fno-slp-vectorize` flags keep source-line attribution accurate by disabling auto-vectorization.
- The matrix CSV file is passed as the first command-line argument.

## Cleanup between attempts

To clear traces from previous reports:

```bash
fpc-create-report -cr
```
