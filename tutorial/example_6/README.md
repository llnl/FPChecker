# Example 6: Data-Driven Mixed Precision

This example teaches a report-driven mixed-precision workflow:

1. Run a baseline FP32 implementation with FPChecker rounding-error instrumentation.
2. Use the report to identify numerically unstable lines.
3. Promote only the sensitive parts to FP64 (mixed precision).
4. Compare FP32 and mixed-precision against FP64 reference accuracy.

## What this example includes

| File | Purpose |
|------|---------|
| `fp32.cpp` | Baseline FP32 implementation instrumented for rounding-error tracking |
| `mixed.cpp` | Selective FP64 promotion and algorithmic improvements |
| `fp64.cpp` | FP64 reference implementation |
| `compare_accuracy.py` | Compares numerical error for FP32 and mixed vs FP64 |
| `algorithms.tex` | Mathematical rationale for the algorithmic changes |

## Participant challenge flow

### Challenge 1: Find unstable FP32 lines

Run the baseline FP32 and inspect rounding errors:

```bash
make clean
make fp32
./fp32 200000 32 1.0 1.0e8 1.0
fpc-create-report -s rounding
```

## Questions for participants

- Which lines show the largest relative error?
- Which error pattern looks like catastrophic cancellation?
- Which lines appear safe and can likely remain FP32?

### Challenge 2: Propose mixed-precision edits

- Which operations should move to FP64?
- Should any formulas be rewritten, not only promoted?

## Mixed-precision script

After discussion, run:

```bash
make mixed fp64
make compare
```

Discuss:

- How `mixed.cpp` addresses high-error hotspots from `fp32.cpp`.
- How `compare_accuracy.py` confirms improvement relative to FP64.
- How this is data-driven mixed precision, not blanket promotion.

## Typical report evidence to discuss

High-error hotspots usually include:

- Quadratic cancellation path around lines 49-52 in `fp32.cpp`.
- One-pass variance formula around lines 24-30 in `fp32.cpp`.

Typical behavior:

- `numer = -b + sqrt(disc)` shows relative error near 1.0.
- `variance = (sumsq / n) - (mean * mean)` can show extremely large relative error.

Low-error lines (for example data-generation terms) are often safe to keep in FP32.

## Why this works

This example combines:

- Precision promotion at sensitive operations.
- Algorithmic improvement (numerically stable formulas).

See `algorithms.tex` for the mathematical details.
