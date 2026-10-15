#!/usr/bin/env python3
"""
Run PolyBench overhead baselines and print one combined overhead table.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import re
import shlex
import shutil
import statistics
import subprocess
import sys
import uuid
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


EXPERIMENTS_DIR = Path(__file__).resolve().parent
BENCHMARK_DIR = EXPERIMENTS_DIR / "benchmark"
TIMING_OVERHEAD_DIR = BENCHMARK_DIR / "timing_overhead"
EFTSAN_DIR = EXPERIMENTS_DIR / "EFTSanitizer"

FPCHECKER_ENVS = ("tutorial_env", "fpchecker_env")
NSAN_ENVS = ("tutorial_env", "fpchecker_env")
EFTSAN_ENVS = ("llvm10", "eftsan_env")

TABLE_SPLIT_RE = re.compile(r"\s{2,}")
CONDA_ENV_PREFIX_CACHE: Optional[Dict[str, Path]] = None


@dataclass
class Runner:
    name: str
    label: str
    cwd: Path
    command: List[str]
    env_names: Sequence[str]


@dataclass
class RunResult:
    runner: Runner
    status: int
    output: str
    environment: str


def normalize_opt_level(value: str) -> str:
    opt = value.strip()
    if opt.startswith("-"):
        opt = opt[1:]
    if opt in {"0", "1", "2", "3"}:
        opt = f"O{opt}"
    opt = opt.upper()
    if opt not in {"O0", "O1", "O2", "O3"}:
        raise argparse.ArgumentTypeError("expected one of O0, O1, O2, or O3")
    return opt


def split_requested(values: Iterable[str]) -> List[str]:
    requested: List[str] = []
    for value in values:
        requested.extend(part.strip() for part in value.split(",") if part.strip())
    return requested


def selected_runners(values: Sequence[str]) -> List[str]:
    requested = split_requested(values)
    if not requested or "all" in requested:
        return ["nsan", "fpchecker", "eftsan"]

    runners: List[str] = []
    for name in requested:
        if name not in {"fpchecker", "eftsan", "nsan"}:
            raise argparse.ArgumentTypeError(
                f"unknown runner {name!r}; expected fpchecker, eftsan, nsan, or all"
            )
        if name not in runners:
            runners.append(name)
    return runners


def prepend_path(env: Dict[str, str], name: str, value: Path) -> None:
    old = env.get(name, "")
    env[name] = f"{value}{os.pathsep}{old}" if old else str(value)


def conda_env_prefixes_by_name() -> Dict[str, Path]:
    global CONDA_ENV_PREFIX_CACHE
    if CONDA_ENV_PREFIX_CACHE is not None:
        return CONDA_ENV_PREFIX_CACHE

    prefixes: Dict[str, Path] = {}
    conda = shutil.which("conda")
    if conda is None:
        CONDA_ENV_PREFIX_CACHE = prefixes
        return prefixes

    try:
        result = subprocess.run(
            [conda, "env", "list", "--json"],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            check=False,
            timeout=15,
        )
    except (OSError, subprocess.TimeoutExpired):
        CONDA_ENV_PREFIX_CACHE = prefixes
        return prefixes

    if result.returncode == 0:
        try:
            data = json.loads(result.stdout)
        except json.JSONDecodeError:
            data = {}
        for item in data.get("envs", []):
            prefix = Path(str(item)).expanduser()
            prefixes.setdefault(prefix.name, prefix)

    CONDA_ENV_PREFIX_CACHE = prefixes
    return prefixes


def candidate_env_prefixes(env_name: str) -> List[Path]:
    value = Path(env_name).expanduser()
    if value.is_absolute() or "/" in env_name:
        return [value]

    candidates: List[Path] = []
    conda_prefix = os.environ.get("CONDA_PREFIX")
    if conda_prefix:
        current_prefix = Path(conda_prefix)
        if current_prefix.name == env_name:
            candidates.append(current_prefix)
        candidates.append(current_prefix.parent / "envs" / env_name)

    home = Path.home()
    candidates.extend(
        [
            home / "conda_env" / env_name,
            home / "miniconda3" / "envs" / env_name,
            home / "anaconda3" / "envs" / env_name,
            Path("/opt/anaconda3/envs") / env_name,
        ]
    )

    listed_prefix = conda_env_prefixes_by_name().get(env_name)
    if listed_prefix is not None:
        candidates.append(listed_prefix)
    return candidates


def resolve_env_prefix(env_name: str) -> Optional[Path]:
    for candidate in candidate_env_prefixes(env_name):
        if candidate.is_dir():
            return candidate.resolve()
    return None


def default_conda_sh() -> Optional[Path]:
    candidates: List[Path] = []
    conda_sh = os.environ.get("CONDA_SH")
    if conda_sh:
        candidates.append(Path(conda_sh).expanduser())
    conda_prefix = os.environ.get("CONDA_PREFIX")
    if conda_prefix:
        candidates.append(Path(conda_prefix) / "etc" / "profile.d" / "conda.sh")
    candidates.extend(
        [
            Path.home() / "miniconda3" / "etc" / "profile.d" / "conda.sh",
            Path.home() / "anaconda3" / "etc" / "profile.d" / "conda.sh",
            Path("/opt/anaconda3/etc/profile.d/conda.sh"),
        ]
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    return None


def shell_join(command: Sequence[str]) -> str:
    return " ".join(shlex.quote(part) for part in command)


def current_env_matches(env_names: Sequence[str]) -> Optional[str]:
    current_name = os.environ.get("CONDA_DEFAULT_ENV", "")
    current_prefix = os.environ.get("CONDA_PREFIX", "")
    for env_name in env_names:
        if current_name == env_name or Path(current_prefix).name == env_name:
            return env_name
    return None


def command_in_environment(
    command: Sequence[str],
    env_names: Sequence[str],
) -> Tuple[List[str], Dict[str, str], str]:
    env = os.environ.copy()

    current = current_env_matches(env_names)
    if current is not None:
        return list(command), env, f"current:{current}"

    for env_name in env_names:
        prefix = resolve_env_prefix(env_name)
        if prefix is None:
            continue
        env["CONDA_PREFIX"] = str(prefix)
        env["CONDA_DEFAULT_ENV"] = env_name
        prepend_path(env, "PATH", prefix / "bin")
        if (prefix / "lib").is_dir():
            prepend_path(env, "LD_LIBRARY_PATH", prefix / "lib")
        return list(command), env, f"path:{prefix}"

    searched = "; ".join(
        f"{env_name}: " + ", ".join(str(path) for path in candidate_env_prefixes(env_name))
        for env_name in env_names
    )
    raise RuntimeError(
        "proper environment not found; running is not working. Expected one of "
        + ", ".join(env_names)
        + ". Searched "
        + searched
    )


def build_runners(args: argparse.Namespace) -> List[Runner]:
    benchmark_filters: List[str] = []
    for pattern in args.benchmark:
        benchmark_filters.extend(["--benchmark", pattern])

    run_name = f"overhead-{uuid.uuid4().hex}"
    fpchecker_work_dir = TIMING_OVERHEAD_DIR / "work" / run_name
    eftsan_work_dir = EFTSAN_DIR / "work_eftsan_overhead" / run_name

    all_runners = {
        "fpchecker": Runner(
            name="fpchecker",
            label="FPChecker",
            cwd=TIMING_OVERHEAD_DIR,
            command=[
                "python3",
                "run_polybench_overhead.py",
                "--mode",
                args.mode,
                "--opt-level",
                args.opt_level,
                "--work-dir",
                str(fpchecker_work_dir),
                *benchmark_filters,
            ],
            env_names=FPCHECKER_ENVS,
        ),
        "nsan": Runner(
            name="nsan",
            label="NSan",
            cwd=BENCHMARK_DIR,
            command=[
                "python3",
                "NSan_report.py",
                args.mode,
                "--opt-level",
                args.opt_level,
                *benchmark_filters,
            ],
            env_names=NSAN_ENVS,
        ),
        "eftsan": Runner(
            name="eftsan",
            label="EFTSan",
            cwd=EFTSAN_DIR,
            command=[
                "python3",
                "run_polybench_eftsan_overhead.py",
                args.mode,
                "--opt-level",
                args.opt_level,
                "--work-dir",
                str(eftsan_work_dir),
                *benchmark_filters,
            ],
            env_names=EFTSAN_ENVS,
        ),
    }

    return [all_runners[name] for name in args.runners]


def run_runner(runner: Runner) -> RunResult:
    try:
        command, env, environment = command_in_environment(runner.command, runner.env_names)
    except RuntimeError as exc:
        raise RuntimeError(f"{runner.label}: {exc}") from exc

    print(f"\n=== {runner.label} ({environment}) ===", flush=True)
    print(f"cwd: {runner.cwd}", flush=True)
    print(f"$ {shell_join(command)}", flush=True)

    process = subprocess.run(
        command,
        cwd=str(runner.cwd),
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    print(process.stdout, end="")
    return RunResult(
        runner=runner,
        status=process.returncode,
        output=process.stdout,
        environment=environment,
    )


def parse_overheads(output: str) -> Dict[str, Optional[float]]:
    overheads: Dict[str, Optional[float]] = {}
    header: Dict[str, int] = {}

    for raw_line in output.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        cells = TABLE_SPLIT_RE.split(line)
        if "Filename" in cells and "Overhead" in cells:
            header = {
                "source": cells.index("Filename"),
                "overhead": cells.index("Overhead"),
            }
            continue
        if not header:
            continue
        if set(line.replace(" ", "")) == {"-"}:
            continue
        if len(cells) <= max(header.values()):
            continue
        source = cells[header["source"]]
        if source == "Average":
            continue
        overheads[source] = parse_float(cells[header["overhead"]])

    return overheads


def parse_float(value: str) -> Optional[float]:
    if value in {"", "-"}:
        return None
    try:
        number = float(value)
    except ValueError:
        return None
    return number if math.isfinite(number) else None


def fmt_float(value: Optional[float]) -> str:
    if value is None:
        return "-"
    return f"{value:.6e}"


def average(values: Iterable[Optional[float]]) -> Optional[float]:
    finite = [value for value in values if value is not None and math.isfinite(value)]
    if not finite:
        return None
    return statistics.mean(finite)


def print_combined_table(overheads_by_runner: Dict[str, Dict[str, Optional[float]]]) -> None:
    sources = sorted(
        {
            source
            for overheads in overheads_by_runner.values()
            for source in overheads
        }
    )

    rows: List[List[str]] = []
    for source in sources:
        rows.append(
            [
                source,
                fmt_float(overheads_by_runner.get("fpchecker", {}).get(source)),
                fmt_float(overheads_by_runner.get("eftsan", {}).get(source)),
                fmt_float(overheads_by_runner.get("nsan", {}).get(source)),
            ]
        )

    rows.append(
        [
            "Average",
            fmt_float(average(overheads_by_runner.get("fpchecker", {}).values())),
            fmt_float(average(overheads_by_runner.get("eftsan", {}).values())),
            fmt_float(average(overheads_by_runner.get("nsan", {}).values())),
        ]
    )

    headers = ["Source", "FPChecker", "EFTSan", "NSan"]
    widths = [len(header) for header in headers]
    for row in rows:
        for index, value in enumerate(row):
            widths[index] = max(widths[index], len(value))

    print("\nOverhead")
    print("  ".join(header.ljust(widths[index]) for index, header in enumerate(headers)))
    print("  ".join("-" * width for width in widths))
    for row in rows[:-1]:
        print("  ".join(value.ljust(widths[index]) for index, value in enumerate(row)))
    print("  ".join("-" * width for width in widths))
    print("  ".join(value.ljust(widths[index]) for index, value in enumerate(rows[-1])))


def parse_args(argv: Optional[Sequence[str]]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run FPChecker, EFTSan, and NSan overhead scripts and print one table."
    )
    parser.add_argument(
        "runners",
        nargs="*",
        default=["all"],
        help="Runners to execute: fpchecker, eftsan, nsan, or all.",
    )
    parser.add_argument(
        "--mode",
        choices=("fp32", "fp64"),
        default="fp32",
        help="Precision mode passed to all overhead scripts.",
    )
    parser.add_argument(
        "--opt-level",
        type=normalize_opt_level,
        default="O2",
        help="Optimization level passed to all overhead scripts.",
    )
    parser.add_argument(
        "--benchmark",
        action="append",
        default=[],
        help="Substring filter forwarded to each overhead script.",
    )
    args = parser.parse_args(argv)
    try:
        args.runners = selected_runners(args.runners)
    except argparse.ArgumentTypeError as exc:
        parser.error(str(exc))
    return args


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)

    missing = [
        TIMING_OVERHEAD_DIR / "run_polybench_overhead.py",
        BENCHMARK_DIR / "NSan_report.py",
        EFTSAN_DIR / "run_polybench_eftsan_overhead.py",
    ]
    missing = [path for path in missing if not path.is_file()]
    if missing:
        for path in missing:
            print(f"error: missing overhead script: {path}", file=sys.stderr)
        return 2

    try:
        runners = build_runners(args)
        results = [run_runner(runner) for runner in runners]
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    overheads_by_runner = {
        result.runner.name: parse_overheads(result.output)
        for result in results
    }
    print_combined_table(overheads_by_runner)

    failures = [
        f"{result.runner.name}={result.status}"
        for result in results
        if result.status != 0
    ]
    if failures:
        print("\nCompleted with failures: " + ", ".join(failures), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
