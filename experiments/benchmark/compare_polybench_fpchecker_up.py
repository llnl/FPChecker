#!/usr/bin/env python3
"""
Run compare_polybench_fpchecker.py with a command-line optimization override.

This intentionally does not edit the benchmark Makefiles. It relies on GNU
make command-line variable precedence by running builds as:
  make OP="<optimization flags>"
"""

from __future__ import annotations

import argparse
import math
import os
import statistics
import sys
from pathlib import Path
from typing import Optional, Sequence

import compare_polybench_fpchecker as base


DEFAULT_EXTRA_FLAGS = "-fno-vectorize -fno-slp-vectorize"
AVERAGE_PASSED_COUNT = 33


def normalize_opt_level(value: str) -> str:
    opt = value.strip()
    if opt.startswith("-"):
        opt = opt[1:]
    if opt in {"0", "1", "2", "3"}:
        opt = f"O{opt}"
    if opt not in {"O0", "O1", "O2", "O3"}:
        raise argparse.ArgumentTypeError("expected one of O0, O1, O2, O3")
    return opt


def parse_wrapper_args(argv: Sequence[str]) -> tuple[argparse.Namespace, list[str]]:
    if any(arg in {"-h", "--help"} for arg in argv):
        print(
            "Optimization override options:\n"
            "  --opt-level {O0,O1,O2,O3}\n"
            "      Optimization level for benchmark builds. Defaults to O2.\n"
            "  --opt-flags FLAGS\n"
            "      Full OP value passed to make; overrides --opt-level.\n"
        )

    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "--opt-level",
        type=normalize_opt_level,
        default="O2",
        help="Optimization level for benchmark builds: O0, O1, O2, or O3. Defaults to O2.",
    )
    parser.add_argument(
        "--opt-flags",
        default=None,
        help=(
            "Full OP value passed to make. Overrides --opt-level, for example "
            "'-O3 -fno-vectorize -fno-slp-vectorize'."
        ),
    )
    return parser.parse_known_args(argv)


def average_row(comparisons: Sequence[base.Comparison]) -> list[str]:
    baseline_total = 0.0
    fpchecker_total = 0.0
    delta_total = 0.0
    passed = 0

    for item in comparisons:
        if item.status != "OK" or item.baseline is None or item.fpchecker is None:
            continue
        if not (math.isfinite(item.baseline) and math.isfinite(item.fpchecker)):
            continue
        baseline_total += item.baseline
        fpchecker_total += item.fpchecker
        delta_total += base.absolute_difference(item.baseline, item.fpchecker)
        passed += 1

    denominator = AVERAGE_PASSED_COUNT
    return [
        "Average",
        "-",
        base.fmt_float(baseline_total / denominator),
        base.fmt_float(fpchecker_total / denominator),
        "-",
        base.fmt_float(delta_total / denominator),
        "-",
        f"{passed}/{denominator} passed",
        "AVERAGE",
    ]


def median_row(comparisons: Sequence[base.Comparison]) -> list[str]:
    baselines = []
    fpcheckers = []
    relative_errors = []
    deltas = []

    for item in comparisons:
        if item.status != "OK" or item.baseline is None or item.fpchecker is None:
            continue
        if not (math.isfinite(item.baseline) and math.isfinite(item.fpchecker)):
            continue
        baselines.append(item.baseline)
        fpcheckers.append(item.fpchecker)
        delta = base.absolute_difference(item.baseline, item.fpchecker)
        if delta is not None and math.isfinite(delta):
            deltas.append(delta)
        if item.relative_error is not None and math.isfinite(item.relative_error):
            relative_errors.append(item.relative_error)

    passed = len(baselines)
    denominator = AVERAGE_PASSED_COUNT
    return [
        "Median",
        "-",
        base.fmt_float(statistics.median(baselines) if baselines else None),
        base.fmt_float(statistics.median(fpcheckers) if fpcheckers else None),
        base.fmt_float(statistics.median(relative_errors) if relative_errors else None),
        base.fmt_float(statistics.median(deltas) if deltas else None),
        "-",
        f"{passed}/{denominator} passed",
        "MEDIAN",
    ]


def print_results_with_average(comparisons: Sequence[base.Comparison]) -> None:
    headers, rows = base.build_result_rows(comparisons)
    avg_row = average_row(comparisons)
    med_row = median_row(comparisons)

    widths = [len(header) for header in headers]
    for row in [*rows, avg_row, med_row]:
        for idx, value in enumerate(row):
            widths[idx] = max(widths[idx], len(value))

    print("  ".join(header.ljust(widths[idx]) for idx, header in enumerate(headers)))
    print("  ".join("-" * width for width in widths))
    for row in rows:
        print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(row)))
    print("  ".join("-" * width for width in widths))
    print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(avg_row)))
    print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(med_row)))

    print()
    base.print_summary(comparisons)


def print_latex_results_with_average(comparisons: Sequence[base.Comparison]) -> None:
    headers, rows = base.build_result_rows(comparisons)
    avg_row = average_row(comparisons)
    med_row = median_row(comparisons)
    column_spec = "llrrrrlll"

    print(r"\begin{tabular}{" + column_spec + "}")
    print(r"\hline")
    print(" & ".join(r"\textbf{" + base.latex_escape(header) + "}" for header in headers) + r" \\")
    print(r"\hline")
    for row in rows:
        print(" & ".join(base.latex_escape(value) for value in row) + r" \\")
    print(r"\hline")
    print(" & ".join(base.latex_escape(value) for value in avg_row) + r" \\")
    print(" & ".join(base.latex_escape(value) for value in med_row) + r" \\")
    print(r"\hline")
    print(r"\end{tabular}")
    print()
    print("% ", end="")
    base.print_summary(comparisons)


def main(argv: Optional[Sequence[str]] = None) -> int:
    raw_args = list(sys.argv[1:] if argv is None else argv)
    wrapper_args, base_args = parse_wrapper_args(raw_args)
    help_requested = any(arg in {"-h", "--help"} for arg in raw_args)

    opt_flags = wrapper_args.opt_flags
    if opt_flags is None:
        opt_flags = f"-{wrapper_args.opt_level} {DEFAULT_EXTRA_FLAGS}"

    original_run_cmd = base.run_cmd
    original_find_executable = base.find_executable
    original_print_results = base.print_results
    original_print_latex_results = base.print_latex_results

    def run_cmd_with_opt(cmd, cwd: Path, env, timeout):
        if cmd and cmd[0] == "make" and "clean" not in cmd[1:]:
            cmd = [*cmd, f"OP={opt_flags}"]
        return original_run_cmd(cmd, cwd, env, timeout)

    def find_executable_with_chmod(bench_dir: Path):
        executable = original_find_executable(bench_dir)
        if executable is not None:
            return executable

        candidates = [bench_dir / name for name in base.parse_makefile_outputs(bench_dir / "Makefile")]
        candidates.append(bench_dir / bench_dir.name)

        for candidate in candidates:
            if not candidate.is_file():
                continue
            try:
                candidate.chmod(candidate.stat().st_mode | 0o100)
            except OSError:
                pass
            if os.access(candidate, os.X_OK):
                return candidate
        return None

    base.run_cmd = run_cmd_with_opt
    base.find_executable = find_executable_with_chmod
    base.print_results = print_results_with_average
    base.print_latex_results = print_latex_results_with_average
    if not help_requested:
        print(f"Using Makefile OP override: {opt_flags}", file=sys.stderr)
    try:
        return base.main(base_args)
    finally:
        base.run_cmd = original_run_cmd
        base.find_executable = original_find_executable
        base.print_results = original_print_results
        base.print_latex_results = original_print_latex_results


if __name__ == "__main__":
    raise SystemExit(main())
