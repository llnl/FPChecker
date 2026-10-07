#!/usr/bin/env python3
"""
Run PolyBench/C with native Clang and LLVM NumericalStabilitySanitizer (NSan)
and report execution-time overhead.

The benchmark Makefiles are NOT edited. GNU make command-line variable
precedence is used to override:

  Baseline:
    make CC="clang" \
         OP="-O2 -fno-vectorize -fno-slp-vectorize" \
         DUMP="-DPOLYBENCH_TIME"

  NSan:
    make CC="clang -fsanitize=numerical" \
         OP="-O2 -fno-vectorize -fno-slp-vectorize" \
         DUMP="-DPOLYBENCH_TIME"

Overhead is computed exactly as:

    (NSanTime - ClangTime) / ClangTime

Example:
    python3 compare_polybench_nsan.py --opt-level O2

Run one benchmark:
    python3 compare_polybench_nsan.py --opt-level O2 --benchmark correlation

Use five repetitions per build:
    python3 compare_polybench_nsan.py --opt-level O2 --runs 5
"""

from __future__ import annotations

import argparse
import math
import os
import re
import shutil
import statistics
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Optional, Sequence


DEFAULT_ROOT = Path("PolyBenchC-4.2.1")
DEFAULT_EXTRA_FLAGS = "-fno-vectorize -fno-slp-vectorize"
DEFAULT_NSAN_OPTIONS = (
    "halt_on_error=0:"
    "log2_max_relative_error=1000:"
    # "log2_absolute_error_threshold=100"
)

# A line containing only one floating-point value. PolyBench/C prints the
# kernel execution time in this form when POLYBENCH_TIME is enabled.
TIME_RE = re.compile(
    r"^\s*([+-]?(?:(?:\d+(?:\.\d*)?)|(?:\.\d+))(?:[eE][+-]?\d+)?)\s*$"
)


@dataclass
class TimingComparison:
    benchmark: str
    mode: str
    opt: str
    clang_time: Optional[float]
    nsan_time: Optional[float]
    overhead: Optional[float]
    clang_status: str
    nsan_status: str


def normalize_opt_level(value: str) -> str:
    opt = value.strip()
    if opt.startswith("-"):
        opt = opt[1:]
    if opt in {"0", "1", "2", "3"}:
        opt = f"O{opt}"
    if opt not in {"O0", "O1", "O2", "O3"}:
        raise argparse.ArgumentTypeError("expected one of O0, O1, O2, O3")
    return opt


def run_cmd(
    cmd: Sequence[str],
    cwd: Path,
    env: dict[str, str],
    timeout: int,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        list(cmd),
        cwd=str(cwd),
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
        check=False,
    )


def discover_benchmarks(root: Path, mode: str) -> list[Path]:
    """
    Match the FPChecker benchmark driver's precision-directory convention:
      fp32 -> directories not ending in *_fp64
      fp64 -> directories under a path component ending in *_fp64
      *_mpfr directories are skipped for both modes
    """
    benchmarks: list[Path] = []

    for makefile in root.rglob("Makefile"):
        bench_dir = makefile.parent
        rel_parts = bench_dir.relative_to(root).parts
        is_fp64_dir = any(part.endswith("_fp64") for part in rel_parts)
        is_mpfr_dir = any(part.lower().endswith("_mpfr") for part in rel_parts)

        if is_mpfr_dir:
            continue
        if mode == "fp32" and is_fp64_dir:
            continue
        if mode == "fp64" and not is_fp64_dir:
            continue

        benchmarks.append(bench_dir)

    return sorted(benchmarks)


def filter_benchmarks(
    benchmarks: Iterable[Path],
    patterns: Sequence[str],
) -> list[Path]:
    if not patterns:
        return list(benchmarks)

    selected: list[Path] = []
    for bench in benchmarks:
        text = str(bench)
        if any(pattern in text for pattern in patterns):
            selected.append(bench)
    return selected


def parse_makefile_outputs(makefile: Path) -> list[str]:
    outputs: list[str] = []
    pattern = re.compile(r"(?:^|\s)-o\s+([^\s]+)")

    try:
        text = makefile.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return outputs

    for match in pattern.finditer(text):
        output = match.group(1)
        if output.startswith("$"):
            continue
        if output not in outputs:
            outputs.append(output)

    return outputs


def find_executable(bench_dir: Path) -> Optional[Path]:
    preferred = [bench_dir.name]
    preferred.extend(parse_makefile_outputs(bench_dir / "Makefile"))

    for name in preferred:
        candidate = bench_dir / name
        if candidate.is_file():
            try:
                candidate.chmod(candidate.stat().st_mode | 0o100)
            except OSError:
                pass
            if os.access(candidate, os.X_OK):
                return candidate

    # Fallback: if exactly one non-script executable exists, use it.
    executables: list[Path] = []
    for child in bench_dir.iterdir():
        if not child.is_file():
            continue
        if child.suffix in {".py", ".sh"}:
            continue
        if os.access(child, os.X_OK):
            executables.append(child)

    if len(executables) == 1:
        return executables[0]

    return None


def clean_benchmark(
    bench_dir: Path,
    env: dict[str, str],
    timeout: int,
) -> None:
    try:
        run_cmd(["make", "clean"], bench_dir, env, timeout)
    except (OSError, subprocess.TimeoutExpired):
        pass

    # These are FPChecker-specific leftovers. Removing them is harmless here
    # and prevents old files from confusing later experiments.
    for name in (".fpc_logs", ".fpc_log.txt", "fpc-report"):
        path = bench_dir / name
        if path.is_dir():
            shutil.rmtree(path, ignore_errors=True)
        elif path.exists():
            try:
                path.unlink()
            except OSError:
                pass


def last_line(output: str) -> str:
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    return lines[-1][:160] if lines else ""


def parse_polybench_time(output: str) -> Optional[float]:
    """
    Return the last numeric-only line.

    With -DPOLYBENCH_TIME and no array dump, PolyBench prints the kernel
    execution time as a standalone floating-point value. NSan diagnostics may
    also be present in the combined output, so we search rather than assuming
    the entire output is one number.
    """
    values: list[float] = []

    for line in output.splitlines():
        match = TIME_RE.match(line)
        if not match:
            continue
        try:
            values.append(float(match.group(1)))
        except ValueError:
            continue

    return values[-1] if values else None


def aggregate_times(values: Sequence[float]) -> float:
    
    if len(values) == 1:
        return values[0]

    if len(values) >= 5:
        ordered = sorted(values)
        return statistics.mean(ordered[1:-1])

    return statistics.median(values)


def build_benchmark(
    bench_dir: Path,
    cc_value: str,
    opt_flags: str,
    env: dict[str, str],
    timeout: int,
) -> tuple[Optional[Path], str]:
    cmd = [
        "make",
        f"CC={cc_value}",
        f"OP={opt_flags}",
        # Override the Makefile's POLYBENCH_DUMP_ARRAYS setting for timing.
        "DUMP=-DPOLYBENCH_TIME",
    ]

    try:
        result = run_cmd(cmd, bench_dir, env, timeout)
    except subprocess.TimeoutExpired:
        return None, "BUILD_TIMEOUT"
    except OSError as exc:
        return None, f"BUILD_ERROR: {exc}"

    if result.returncode != 0:
        return None, f"BUILD_FAILED: {last_line(result.stdout)}"

    executable = find_executable(bench_dir)
    if executable is None:
        return None, "NO_EXECUTABLE"

    return executable, "OK"


def measure_executable(
    executable: Path,
    bench_dir: Path,
    env: dict[str, str],
    timeout: int,
    runs: int,
) -> tuple[Optional[float], str]:
    times: list[float] = []

    for _ in range(runs):
        try:
            result = run_cmd(
                ["bash", "-lc", f"ulimit -s 8192; ./{executable.name}"],
                bench_dir,
                env,
                timeout,
            )
        except subprocess.TimeoutExpired:
            return None, "RUN_TIMEOUT"
        except OSError as exc:
            return None, f"RUN_ERROR: {exc}"

        if result.returncode != 0:
            return None, f"RUN_FAILED: {last_line(result.stdout)}"

        elapsed = parse_polybench_time(result.stdout)
        if elapsed is None:
            return None, "NO_POLYBENCH_TIME"

        if not math.isfinite(elapsed):
            return None, "INVALID_TIME"

        if elapsed <= 0.0:
            return None, "NONPOSITIVE_TIME"

        times.append(elapsed)

    return aggregate_times(times), "OK"


def overhead_ratio(
    baseline: Optional[float],
    tool: Optional[float],
) -> Optional[float]:
    if baseline is None or tool is None:
        return None
    if not (math.isfinite(baseline) and math.isfinite(tool)):
        return None
    if baseline <= 0.0:
        return None

    # Same definition used for the FPChecker overhead numbers:
    #   (tool - baseline) / baseline
    return (tool - baseline) / baseline


def compare_benchmark(
    bench_dir: Path,
    root: Path,
    mode: str,
    opt_name: str,
    opt_flags: str,
    clang: str,
    timeout: int,
    runs: int,
    cleanup: bool,
) -> TimingComparison:
    benchmark_name = str(bench_dir.relative_to(root))
    base_env = os.environ.copy()
    base_env["CCACHE_DISABLE"] = "1"

    # ------------------------------------------------------------
    # Native Clang baseline
    # ------------------------------------------------------------
    if cleanup:
        clean_benchmark(bench_dir, base_env, timeout)

    clang_exe, clang_status = build_benchmark(
        bench_dir=bench_dir,
        cc_value=clang,
        opt_flags=opt_flags,
        env=base_env,
        timeout=timeout,
    )

    clang_time: Optional[float] = None
    if clang_exe is not None:
        clang_time, clang_status = measure_executable(
            executable=clang_exe,
            bench_dir=bench_dir,
            env=base_env,
            timeout=timeout,
            runs=runs,
        )

    # ------------------------------------------------------------
    # NSan
    # ------------------------------------------------------------
    if cleanup:
        clean_benchmark(bench_dir, base_env, timeout)

    nsan_env = os.environ.copy()
    nsan_env["CCACHE_DISABLE"] = "1"

    # NSan defaults to halting after its first warning. For an execution-time
    # benchmark, the whole kernel must run to completion. These thresholds match
    # the NSan error-report workflow used for the PolyBench norm comparison.
    nsan_env["NSAN_OPTIONS"] = DEFAULT_NSAN_OPTIONS

    # Put -fsanitize=numerical in CC rather than only OP because the Makefiles
    # do not use $(OP) on the final link command. This ensures compiler-rt's
    # NSan runtime is linked as well as the source being instrumented.
    nsan_cc = f"{clang} -fsanitize=numerical"

    nsan_exe, nsan_status = build_benchmark(
        bench_dir=bench_dir,
        cc_value=nsan_cc,
        opt_flags=opt_flags,
        env=nsan_env,
        timeout=timeout,
    )

    nsan_time: Optional[float] = None
    if nsan_exe is not None:
        nsan_time, nsan_status = measure_executable(
            executable=nsan_exe,
            bench_dir=bench_dir,
            env=nsan_env,
            timeout=timeout,
            runs=runs,
        )

    if cleanup:
        clean_benchmark(bench_dir, base_env, timeout)

    return TimingComparison(
        benchmark=benchmark_name,
        mode=mode,
        opt=opt_name,
        clang_time=clang_time,
        nsan_time=nsan_time,
        overhead=overhead_ratio(clang_time, nsan_time),
        clang_status=clang_status,
        nsan_status=nsan_status,
    )


def fmt_float(value: Optional[float]) -> str:
    if value is None:
        return "-"
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "inf" if value > 0 else "-inf"
    return f"{value:.6e}"


def build_result_rows(
    results: Sequence[TimingComparison],
) -> tuple[list[str], list[list[str]]]:
    headers = [
        "Filename",
        "Mode",
        "Opt",
        "ClangTime(s)",
        "NSanTime(s)",
        "Overhead",
        "ClangStatus",
        "NSanStatus",
    ]

    rows: list[list[str]] = []

    for item in results:
        rows.append(
            [
                item.benchmark,
                item.mode,
                item.opt,
                fmt_float(item.clang_time),
                fmt_float(item.nsan_time),
                fmt_float(item.overhead),
                item.clang_status,
                item.nsan_status,
            ]
        )

    return headers, rows


def aggregate_row(
    results: Sequence[TimingComparison],
    kind: str,
) -> list[str]:
    valid = [
        item
        for item in results
        if item.clang_status == "OK"
        and item.nsan_status == "OK"
        and item.clang_time is not None
        and item.nsan_time is not None
        and item.overhead is not None
        and math.isfinite(item.clang_time)
        and math.isfinite(item.nsan_time)
        and math.isfinite(item.overhead)
    ]

    if not valid:
        return [kind, "-", "-", "-", "-", "-", "-", "0 passed"]

    clang_values = [item.clang_time for item in valid if item.clang_time is not None]
    nsan_values = [item.nsan_time for item in valid if item.nsan_time is not None]
    overhead_values = [item.overhead for item in valid if item.overhead is not None]

    if kind == "Average":
        clang_value = statistics.mean(clang_values)
        nsan_value = statistics.mean(nsan_values)
        overhead_value = statistics.mean(overhead_values)
    elif kind == "Median":
        clang_value = statistics.median(clang_values)
        nsan_value = statistics.median(nsan_values)
        overhead_value = statistics.median(overhead_values)
    else:
        raise ValueError(kind)

    return [
        kind,
        "-",
        "-",
        fmt_float(clang_value),
        fmt_float(nsan_value),
        fmt_float(overhead_value),
        "-",
        f"{len(valid)}/{len(results)} passed",
    ]


def print_results(results: Sequence[TimingComparison]) -> None:
    headers, rows = build_result_rows(results)
    avg_row = aggregate_row(results, "Average")
    med_row = aggregate_row(results, "Median")

    all_rows = [*rows, avg_row, med_row]

    widths = [len(header) for header in headers]
    for row in all_rows:
        for idx, value in enumerate(row):
            widths[idx] = max(widths[idx], len(value))

    print("  ".join(header.ljust(widths[idx]) for idx, header in enumerate(headers)))
    print("  ".join("-" * width for width in widths))

    for row in rows:
        print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(row)))

    print("  ".join("-" * width for width in widths))
    print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(avg_row)))
    # print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(med_row)))

    passed = sum(
        1
        for item in results
        if item.clang_status == "OK" and item.nsan_status == "OK"
    )
    print()
    print(f"Summary: {passed}/{len(results)} benchmarks completed for both Clang and NSan.")


def latex_escape(value: str) -> str:
    replacements = {
        "\\": r"\textbackslash{}",
        "&": r"\&",
        "%": r"\%",
        "$": r"\$",
        "#": r"\#",
        "_": r"\_",
        "{": r"\{",
        "}": r"\}",
        "~": r"\textasciitilde{}",
        "^": r"\textasciicircum{}",
    }
    return "".join(replacements.get(char, char) for char in value)


def print_latex_results(results: Sequence[TimingComparison]) -> None:
    headers, rows = build_result_rows(results)
    avg_row = aggregate_row(results, "Average")
    med_row = aggregate_row(results, "Median")

    print(r"\begin{tabular}{lllrrrll}")
    print(r"\hline")
    print(" & ".join(r"\textbf{" + latex_escape(h) + "}" for h in headers) + r" \\")
    print(r"\hline")

    for row in rows:
        print(" & ".join(latex_escape(value) for value in row) + r" \\")

    print(r"\hline")
    print(" & ".join(latex_escape(value) for value in avg_row) + r" \\")
    print(" & ".join(latex_escape(value) for value in med_row) + r" \\")
    print(r"\hline")
    print(r"\end{tabular}")


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Measure PolyBench/C runtime overhead of LLVM NSan versus native Clang."
    )

    parser.add_argument(
        "mode",
        nargs="?",
        choices=("fp32", "fp64"),
        default="fp32",
        help="Precision directory set to run. Default: fp32.",
    )

    parser.add_argument(
        "--root",
        default=str(DEFAULT_ROOT),
        help="PolyBench root. Default: PolyBenchC-4.2.1",
    )

    parser.add_argument(
        "--opt-level",
        type=normalize_opt_level,
        default="O2",
        help="Optimization level: O0, O1, O2, or O3. Default: O2.",
    )

    parser.add_argument(
        "--opt-flags",
        default=None,
        help=(
            "Full OP value passed to make. Overrides --opt-level, e.g. "
            "'-O3 -fno-vectorize -fno-slp-vectorize'."
        ),
    )

    parser.add_argument(
        "--clang",
        default=os.environ.get("CLANG", "clang"),
        help="Clang executable/path. Default: clang, or $CLANG if set.",
    )

    parser.add_argument(
        "--benchmark",
        action="append",
        default=[],
        help="Substring filter for benchmark paths. May be specified more than once.",
    )

    parser.add_argument(
        "--runs",
        type=int,
        default=1,
        help=(
            "Number of executions per built binary. Default: 1 to match the "
            "current FPChecker timing workflow. Use 5 if rerunning both tools "
            "with repeated PolyBench timing."
        ),
    )

    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help="Timeout in seconds for each make/run/clean command. Default: 300.",
    )

    parser.add_argument(
        "--no-clean",
        action="store_true",
        help="Do not run make clean between Clang and NSan builds.",
    )

    parser.add_argument(
        "--stop-on-failure",
        action="store_true",
        help="Stop after the first benchmark where Clang or NSan fails.",
    )

    parser.add_argument(
        "--latex",
        action="store_true",
        help="Print final results as a LaTeX tabular.",
    )

    args = parser.parse_args(argv)

    if args.runs < 1:
        parser.error("--runs must be >= 1")

    root = Path(args.root).resolve()
    if not root.is_dir():
        print(f"error: benchmark root not found: {root}", file=sys.stderr)
        return 2

    if args.opt_flags is None:
        opt_flags = f"-{args.opt_level} {DEFAULT_EXTRA_FLAGS}"
    else:
        opt_flags = args.opt_flags

    benchmarks = filter_benchmarks(
        discover_benchmarks(root, args.mode),
        args.benchmark,
    )

    if not benchmarks:
        print(f"error: no {args.mode} benchmarks matched", file=sys.stderr)
        return 2

    progress_stream = sys.stderr if args.latex else sys.stdout

    print(f"Clang baseline: {args.clang}", file=progress_stream)
    print(
        f"NSan compiler:  {args.clang} -fsanitize=numerical",
        file=progress_stream,
    )
    # print(f"OP override:    {opt_flags}", file=progress_stream)
    # print("Timing macro:   -DPOLYBENCH_TIME", file=progress_stream)
    # print(f"Runs/build:     {args.runs}", file=progress_stream)
    # print(
    #         f"NSAN_OPTIONS:   {DEFAULT_NSAN_OPTIONS}",
    #         file=progress_stream,
    #     )
    # print(file=progress_stream)

    results: list[TimingComparison] = []

    for index, bench_dir in enumerate(benchmarks, start=1):
        benchmark_name = str(bench_dir.relative_to(root))
        print(
            f"[{index}/{len(benchmarks)}] {benchmark_name}",
            file=progress_stream,
            flush=True,
        )

        try:
            result = compare_benchmark(
                bench_dir=bench_dir,
                root=root,
                mode=args.mode,
                opt_name=args.opt_level,
                opt_flags=opt_flags,
                clang=args.clang,
                timeout=args.timeout,
                runs=args.runs,
                cleanup=not args.no_clean,
            )
        except Exception as exc:
            result = TimingComparison(
                benchmark=benchmark_name,
                mode=args.mode,
                opt=args.opt_level,
                clang_time=None,
                nsan_time=None,
                overhead=None,
                clang_status="ERROR",
                nsan_status=f"{type(exc).__name__}: {exc}",
            )

        results.append(result)

        if args.stop_on_failure and (
            result.clang_status != "OK" or result.nsan_status != "OK"
        ):
            break

    if args.latex:
        print_latex_results(results)
    else:
        print()
        print_results(results)

    return 0 if all(
        item.clang_status == "OK" and item.nsan_status == "OK"
        for item in results
    ) else 1


if __name__ == "__main__":
    raise SystemExit(main())
