# Example 5: Tracking Error Evolution on a Single CG Line

This tutorial example uses FPChecker to track the sequence of relative errors for one specific line in the FP32 Conjugate Gradient solver.

Focus line:

- Line 90 in `cg.cpp`: residual update
- Formula: `r_{k+1} = r_k - alpha_k * (A p_k)`

As CG progresses, this update can become cancellation-prone, and the relative error may approach or exceed 100%.

## What this example includes

| File | Purpose |
|------|---------|
| `cg.cpp` | FP32 CG baseline (instrumented) |
| `plot_error_values.py` | Plots per-execution relative error values for selected lines |
| `generate_cg_matrices.py` | Utility script to regenerate matrix inputs |
| `matrices/` | Pre-generated 100x100 SPD matrices |
| `Makefile` | Build and clean targets |

## Environment setup

```bash
source ~/.bash_profile
conda activate tutorial_env
export PATH=/opt/anaconda3/envs/tutorial_env/bin:/Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/bin:$PATH
```

Quick checks:

```bash
which clang++
clang++ --version
which python3
python3 -c "import matplotlib; print(matplotlib.__version__)"
which clang++-fpchecker
which fpc-create-report
```

## Challenge flow for participants

### Challenge 1: Run FP32 CG and track only line 90

Use `FPC_SAVE_LINE_ERRORS=90` to save the relative error series for instructions mapped to line 90.

```bash
make clean
FPC_INSTRUMENT_ERR_TRACKING=1 make cg
CG_MATRIX=matrices/5_matrix_3.998e+04.csv FPC_SAVE_LINE_ERRORS=90 ./cg 500 1e-6
```

Expected runtime message:

```text
#FPCHECKER: Saving errors for lines: 90
#FPCHECKER: Writing errors per line to: .fpc_logs/errors_per_line_<host>_<pid>.json
```

### Challenge 2: Plot line-90 error evolution

Plot the JSON created above:

```bash
python3 plot_error_values.py .fpc_logs/errors_per_line_*.json --line 90 --output line90_errors.png
```

Optional interactive display:

```bash
python3 plot_error_values.py .fpc_logs/errors_per_line_*.json --line 90 --show
```

Note: the plot always uses logarithmic y-axis. Zero or negative values are hidden automatically because logarithmic scale requires positive values.

What to discuss:

- Early iterations can have small or zero relative error in this line.
- As the residual gets smaller, subtraction in line 90 becomes increasingly cancellation-prone.
- Relative error near 1.0 means approximately 100% error in that operation.

## Optional: Generate full rounding report (all lines)

```bash
fpc-create-report -s rounding
```

This report gives aggregate line-level relative errors, while `errors_per_line_*.json` gives the per-execution sequence for selected lines.

## Optional: Regenerate matrix inputs

Matrices are already provided in `matrices/` for the tutorial.
If needed, regenerate them with:

```bash
python3 generate_cg_matrices.py
```

## 20-minute teaching suggestion (line-error focus)

1. Minute 0-3: Explain why line-level temporal error tracking matters.
2. Minute 3-8: Participants run FP32 with `FPC_SAVE_LINE_ERRORS=90`.
3. Minute 8-12: Participants plot `errors_per_line_*.json`.
4. Minute 12-16: Discuss curve shape and cancellation behavior near convergence.
5. Minute 16-20: Connect this evidence to mixed-precision decisions.

## Cleanup

```bash
fpc-create-report -cr
make clean
```
