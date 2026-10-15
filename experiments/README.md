# FPChecker Artifact Reproduction

This directory contains scripts for reproducing the results reported in the paper.

## Setup

Go to the FPChecker experiments directory:

```bash
cd FPChecker/experiments
```

Most scripts automatically handle the required environments.

## Reproducing Paper Results

### Table 1

To reproduce the FP32 `-O0` comparison between FPChecker and EFTSanitizer:

```bash
python compare_error_baselines.py fp32 --opt-level O0 --baseline fpchecker eftsan
```

### Table 2

To reproduce the FP32 `-O2` comparison between FPChecker and EFTSanitizer:

```bash
python compare_error_baselines.py fp32 --opt-level O2 --baseline fpchecker eftsan
```

### Table 3

To reproduce the proxy-application cancellation results:

```bash
python proxy_apps_cancellation_results.py
```

### Table 6

To reproduce the mixed-precision results:

```bash
python mixed_precision_result.py
```

### Table 7

To reproduce the MPFR-based PolyBench results, activate the FPChecker environment if needed:

```bash
conda activate fpchecker_env
conda install -c conda-forge mpfr gmp
python mpfr_polybench_results.py
```

## Appendix Results

### Table 8

To reproduce the FP32 `-O2` comparison between FPChecker and NSan:

```bash
python compare_error_baselines.py fp32 --opt-level O2 --baseline fpchecker nsan
```

### Table 9

To reproduce the FP64 `-O2` comparison among FPChecker, NSan, and EFTSanitizer:

```bash
python compare_error_baselines.py fp64 --opt-level O2 --baseline fpchecker nsan eftsan
```

### Table 10

To reproduce the FP32 `-O2` execution-overhead ratios:

```bash
python overhead.py fp32 --opt-level O2
```

**Note:** Overhead ratios can change somewhat across runs and environments because they depend on measured execution times.