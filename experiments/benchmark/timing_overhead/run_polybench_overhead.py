#!/usr/bin/env python3
"""
Measure FPChecker overhead on PolyBench without modifying existing benchmarks.

For each benchmark, the script creates two copied build directories under
timing_overhead/work:
  1. native:    built with clang
  2. fpchecker: built with the FPChecker instrumentation wrapper

It runs each executable once and prints:
  filename, clang time, FPChecker time, overhead

Overhead is computed as:
  (fpchecker_time - clang_time) / clang_time
"""

from __future__ import annotations

import argparse
import math
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Optional, Sequence


SCRIPT_DIR = Path(__file__).resolve().parent
BENCHMARK_DIR = SCRIPT_DIR.parent
REPO_ROOT = SCRIPT_DIR.parents[2]
DEFAULT_ROOT = BENCHMARK_DIR / "PolyBenchC-4.2.1"
DEFAULT_WORK_DIR = SCRIPT_DIR / "work"
DEFAULT_BIN_DIRS = (
    REPO_ROOT / "bin",
    REPO_ROOT / "build" / "install" / "bin",
    REPO_ROOT / "build-current" / "install" / "bin",
    REPO_ROOT / "install" / "bin",
    REPO_ROOT.parent / "install" / "bin",
)
DEFAULT_EXTRA_OPT_FLAGS = "-fno-vectorize -fno-slp-vectorize"
TIME_RE = re.compile(
    r"^\s*(?P<value>[+-]?(?:(?:\d+(?:\.\d*)?)|(?:\.\d+))(?:[eE][+-]?\d+)?)\s*$"
)


@dataclass
class BuildResult:
    ok: bool
    status: str
    output: str


@dataclass
class RunResult:
    status: str
    time: Optional[float]
    output: str


@dataclass
class OverheadResult:
    filename: str
    mode: str
    opt_level: str
    clang_time: Optional[float]
    fpchecker_time: Optional[float]
    overhead: Optional[float]
    clang_status: str
    fpchecker_status: str


def normalize_opt_level(value: str) -> str:
    opt = value.strip()
    if opt.lower() == "both":
        return "both"
    if opt.startswith("-"):
        opt = opt[1:]
    if opt in {"0", "2"}:
        opt = f"O{opt}"
    if opt not in {"O0", "O2"}:
        raise argparse.ArgumentTypeError("expected O0, O2, or both")
    return opt


def selected_modes(value: str) -> list[str]:
    return ["fp32", "fp64"] if value == "both" else [value]


def selected_opts(value: str) -> list[str]:
    return ["O0", "O2"] if value == "both" else [value]


def run_cmd(
    cmd: Sequence[str],
    cwd: Path,
    env: dict[str, str],
    timeout: int,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=str(cwd),
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
        check=False,
    )


def discover_benchmarks(root: Path, mode: str) -> list[Path]:
    benchmarks: list[Path] = []
    for makefile in root.rglob("Makefile"):
        bench_dir = makefile.parent
        rel_parts = bench_dir.relative_to(root).parts
        lower_parts = [part.lower() for part in rel_parts]
        is_fp64 = any(part.endswith("_fp64") for part in lower_parts)
        is_mpfr = any(part.endswith("_mpfr") for part in lower_parts)
        if is_mpfr:
            continue
        if mode == "fp32" and is_fp64:
            continue
        if mode == "fp64" and not is_fp64:
            continue
        benchmarks.append(bench_dir)
    return sorted(benchmarks)


def filter_benchmarks(benchmarks: Iterable[Path], patterns: Sequence[str]) -> list[Path]:
    selected = []
    for bench in benchmarks:
        text = str(bench)
        if not patterns or any(pattern in text for pattern in patterns):
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
        if not output.startswith("$"):
            outputs.append(output)
    return outputs


def find_executable(bench_dir: Path) -> Optional[Path]:
    preferred = [bench_dir.name]
    preferred.extend(parse_makefile_outputs(bench_dir / "Makefile"))

    for name in preferred:
        candidate = bench_dir / name
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate

    executables = []
    for child in bench_dir.iterdir():
        if child.is_file() and os.access(child, os.X_OK) and child.suffix not in {".py", ".sh"}:
            executables.append(child)
    if len(executables) == 1:
        return executables[0]
    return None


def make_environment() -> dict[str, str]:
    env = os.environ.copy()
    env["CCACHE_DISABLE"] = "1"
    path_entries = []
    for candidate in DEFAULT_BIN_DIRS:
        if candidate.is_dir():
            path_entries.append(str(candidate))
    path_entries.append(env.get("PATH", ""))
    env["PATH"] = ":".join(entry for entry in path_entries if entry)
    return env


def fpchecker_cc(mode: str, wrapper: str) -> str:
    if mode == "fp64":
        return f"FPC_INSTRUMENT_ERR_TRACKING_FP64=1 {wrapper}"
    return f"FPC_INSTRUMENT_ERR_TRACKING=1 {wrapper}"


def opt_flags(opt_level: str, extra_flags: str) -> str:
    flags = [f"-{opt_level}"]
    if extra_flags.strip():
        flags.append(extra_flags.strip())
    return " ".join(flags)


def copy_ignore(_: str, names: list[str]) -> set[str]:
    ignored = {".git", ".fpc_logs", ".fpc_log.txt", "fpc-report", "__pycache__"}
    ignored.update(name for name in names if name.endswith((".o", ".ll", ".bc")))
    return ignored.intersection(names)


def prepare_work_benchmark(
    source_root: Path,
    source_bench_dir: Path,
    work_dir: Path,
    mode: str,
    opt_level: str,
    variant: str,
) -> Path:
    rel_bench = source_bench_dir.relative_to(source_root)
    work_root = work_dir / mode / opt_level / variant / source_root.name
    work_bench_dir = work_root / rel_bench

    if work_root.exists():
        shutil.rmtree(work_root)

    (work_root / "utilities").parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source_root / "utilities", work_root / "utilities", ignore=copy_ignore)
    work_bench_dir.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source_bench_dir, work_bench_dir, ignore=copy_ignore)
    return work_bench_dir


def build_benchmark(
    bench_dir: Path,
    compiler_value: str,
    opt_value: str,
    dataset_macro: str,
    env: dict[str, str],
    timeout: int,
) -> BuildResult:
    cmd = [
        "make",
        f"CC={compiler_value}",
        f"OP={opt_value}",
        f"DATASET=-D{dataset_macro}",
        "DUMP=-DPOLYBENCH_TIME",
    ]
    try:
        build = run_cmd(cmd, bench_dir, env, timeout)
    except subprocess.TimeoutExpired as exc:
        return BuildResult(False, "BUILD_TIMEOUT", str(exc))
    if build.returncode != 0:
        return BuildResult(False, f"BUILD_FAILED: {last_line(build.stdout)}", build.stdout)
    if find_executable(bench_dir) is None:
        return BuildResult(False, "NO_EXECUTABLE", build.stdout)
    return BuildResult(True, "OK", build.stdout)


def parse_polybench_time(output: str) -> Optional[float]:
    values = []
    for line in output.splitlines():
        match = TIME_RE.match(line)
        if match:
            values.append(float(match.group("value")))
    if not values:
        return None
    return values[-1]


def run_executable(bench_dir: Path, env: dict[str, str], timeout: int) -> RunResult:
    executable = find_executable(bench_dir)
    if executable is None:
        return RunResult("NO_EXECUTABLE", None, "")
    try:
        run = run_cmd([f"./{executable.name}"], bench_dir, env, timeout)
    except subprocess.TimeoutExpired as exc:
        return RunResult("RUN_TIMEOUT", None, str(exc))
    if run.returncode != 0:
        return RunResult(f"RUN_FAILED: {last_line(run.stdout)}", None, run.stdout)
    time_value = parse_polybench_time(run.stdout)
    if time_value is None or not math.isfinite(time_value):
        return RunResult("NO_TIME_OUTPUT", None, run.stdout)
    return RunResult("OK", time_value, run.stdout)


def build_and_run(
    bench_dir: Path,
    compiler_value: str,
    opt_value: str,
    args: argparse.Namespace,
    env: dict[str, str],
) -> RunResult:
    build = build_benchmark(
        bench_dir=bench_dir,
        compiler_value=compiler_value,
        opt_value=opt_value,
        dataset_macro=args.dataset,
        env=env,
        timeout=args.timeout,
    )
    if not build.ok:
        return RunResult(build.status, None, build.output)
    return run_executable(bench_dir, env, args.timeout)


def measure_one(
    source_bench_dir: Path,
    source_root: Path,
    mode: str,
    opt_level: str,
    args: argparse.Namespace,
    env: dict[str, str],
) -> OverheadResult:
    filename = str(source_bench_dir.relative_to(source_root))
    flags = opt_flags(opt_level, args.extra_opt_flags)

    native_dir = prepare_work_benchmark(
        source_root, source_bench_dir, args.work_dir, mode, opt_level, "native"
    )
    fpc_dir = prepare_work_benchmark(
        source_root, source_bench_dir, args.work_dir, mode, opt_level, "fpchecker"
    )

    clang_run = build_and_run(native_dir, args.native_cc, flags, args, env)
    fpc_run = build_and_run(fpc_dir, fpchecker_cc(mode, args.fpchecker_wrapper), flags, args, env)

    overhead = None
    if clang_run.time is not None and fpc_run.time is not None and clang_run.time > 0.0:
        overhead = (fpc_run.time - clang_run.time) / clang_run.time

    return OverheadResult(
        filename=filename,
        mode=mode,
        opt_level=opt_level,
        clang_time=clang_run.time,
        fpchecker_time=fpc_run.time,
        overhead=overhead,
        clang_status=clang_run.status,
        fpchecker_status=fpc_run.status,
    )


def fmt_float(value: Optional[float]) -> str:
    if value is None:
        return "-"
    return f"{value:.6e}"


def print_results(results: Sequence[OverheadResult]) -> None:
    headers = [
        "Filename",
        "Mode",
        "Opt",
        "ClangTime(s)",
        "FPCheckerTime(s)",
        "Overhead",
        "ClangStatus",
        "FPCheckerStatus",
    ]
    rows = []
    for item in results:
        rows.append(
            [
                item.filename,
                item.mode,
                item.opt_level,
                fmt_float(item.clang_time),
                fmt_float(item.fpchecker_time),
                fmt_float(item.overhead),
                item.clang_status,
                item.fpchecker_status,
            ]
        )

    widths = [len(header) for header in headers]
    for row in rows:
        for idx, value in enumerate(row):
            widths[idx] = max(widths[idx], len(value))

    print("  ".join(header.ljust(widths[idx]) for idx, header in enumerate(headers)))
    print("  ".join("-" * width for width in widths))
    for row in rows:
        print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(row)))


def last_line(output: str) -> str:
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    if not lines:
        return ""
    return lines[-1][:160]


def parse_args(argv: Optional[Sequence[str]]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Print FPChecker runtime overhead on PolyBench."
    )
    parser.add_argument(
        "--root",
        default=str(DEFAULT_ROOT),
        help="PolyBench root. Defaults to experiments/benchmark/PolyBenchC-4.2.1.",
    )
    parser.add_argument(
        "--work-dir",
        type=Path,
        default=DEFAULT_WORK_DIR,
        help="Separate build workspace. Defaults to timing_overhead/work.",
    )
    parser.add_argument(
        "--mode",
        choices=("fp32", "fp64", "both"),
        default="both",
        help="Precision mode to run. Defaults to both.",
    )
    parser.add_argument(
        "--opt-level",
        type=normalize_opt_level,
        default="both",
        help="Optimization level: O0, O2, or both. Defaults to both.",
    )
    parser.add_argument(
        "--extra-opt-flags",
        default=DEFAULT_EXTRA_OPT_FLAGS,
        help="Extra flags appended to -O0/-O2. Defaults to vectorization-disable flags.",
    )
    parser.add_argument(
        "--dataset",
        default="MINI_DATASET",
        help="PolyBench dataset macro without -D. Defaults to MINI_DATASET.",
    )
    parser.add_argument(
        "--benchmark",
        action="append",
        default=[],
        help="Substring filter for benchmark paths. Can be passed more than once.",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help="Timeout in seconds for each build or run. Defaults to 300.",
    )
    parser.add_argument(
        "--native-cc",
        default="clang",
        help="Native baseline compiler. Defaults to clang.",
    )
    parser.add_argument(
        "--fpchecker-wrapper",
        default="clang-fpchecker",
        help="FPChecker instrumentation wrapper. Defaults to clang-fpchecker.",
    )
    return parser.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    source_root = Path(args.root).resolve()
    if not source_root.is_dir():
        print(f"error: benchmark root not found: {source_root}", file=sys.stderr)
        return 2

    args.work_dir = args.work_dir.resolve()
    env = make_environment()
    results: list[OverheadResult] = []

    for mode in selected_modes(args.mode):
        benchmarks = filter_benchmarks(discover_benchmarks(source_root, mode), args.benchmark)
        if not benchmarks:
            print(f"warning: no {mode} benchmarks matched", file=sys.stderr)
            continue
        for opt_level in selected_opts(args.opt_level):
            for index, bench_dir in enumerate(benchmarks, start=1):
                rel = bench_dir.relative_to(source_root)
                print(f"[{mode} {opt_level} {index}/{len(benchmarks)}] {rel}", flush=True)
                results.append(measure_one(bench_dir, source_root, mode, opt_level, args, env))

    if not results:
        print("error: no benchmark runs were attempted", file=sys.stderr)
        return 1

    print()
    print_results(results)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
