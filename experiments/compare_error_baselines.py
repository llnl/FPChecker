#!/usr/bin/env python3
"""
Run selected PolyBench error baselines and print a joined error/diff table.
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
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_RESULTS_DIR = SCRIPT_DIR / "RQ_results"
DEFAULT_BENCHMARK_DIR = SCRIPT_DIR / "benchmark"
DEFAULT_EFTSAN_DIR = SCRIPT_DIR / "EFTSanitizer"

BASELINE_CHOICES = ("fpchecker", "eftsan", "nsan")
DEFAULT_BASELINES = ("fpchecker", "eftsan")
PRECISION_CHOICES = ("fp32", "fp64")
OPT_CHOICES = ("O0", "O1", "O2", "O3")
BASELINE_LABELS = {
    "fpchecker": "FPChecker",
    "eftsan": "EFTSan",
    "nsan": "NSan",
}
DEFAULT_ACTIVATION_MODE = "auto"
DEFAULT_FPCHECKER_ENVS = ("tutorial_env", "fpchecker_env")
DEFAULT_NSAN_ENVS = ("fpchecker_env", "nsan_env", "tutorial_env")
DEFAULT_EFTSAN_ENVS = ("llvm10", "eftsan_env")
CONDA_ENV_PREFIX_CACHE: Optional[Dict[str, Path]] = None


@dataclass
class CommandResult:
    name: str
    returncode: int
    output: str
    log_path: Path
    activation: str


@dataclass
class BaselineRow:
    baseline_name: str
    precision: str
    opt: str
    benchmark: str
    source: str
    baseline: Optional[float]
    error: Optional[float]
    diff: Optional[float]
    status: str


def normalize_opt_level(value: str) -> str:
    opt = value.strip()
    if opt.startswith("-"):
        opt = opt[1:]
    if opt in {"0", "1", "2", "3"}:
        opt = f"O{opt}"
    opt = opt.upper()
    if opt not in OPT_CHOICES:
        raise argparse.ArgumentTypeError("expected one of O0, O1, O2, O3")
    return opt


def fmt_float(value: Optional[float]) -> str:
    if value is None:
        return "-"
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "inf" if value > 0 else "-inf"
    return f"{value:.6e}"


def parse_float(value: object) -> Optional[float]:
    if value is None:
        return None
    text = str(value).strip()
    if text in {"", "-"}:
        return None
    try:
        return float(text)
    except ValueError:
        return None


def finite_values(values: Iterable[Optional[float]]) -> List[float]:
    return [value for value in values if value is not None and math.isfinite(value)]


def aggregate(values: Sequence[float], use_median: bool) -> Optional[float]:
    if not values:
        return None
    return statistics.median(values) if use_median else statistics.mean(values)


def split_requested(values: Sequence[object]) -> List[str]:
    split_values: List[str] = []
    for value in values:
        if isinstance(value, (list, tuple)):
            split_values.extend(split_requested(value))
            continue
        split_values.extend(part.strip() for part in str(value).split(",") if part.strip())
    return split_values


def parse_baselines(values: Sequence[object]) -> List[str]:
    requested = split_requested(values)
    if not requested:
        return list(DEFAULT_BASELINES)

    baselines: List[str] = []
    for item in requested:
        item = item.lower()
        candidates = BASELINE_CHOICES if item == "all" else (item,)
        for candidate in candidates:
            if candidate not in BASELINE_CHOICES:
                raise argparse.ArgumentTypeError(
                    f"unknown baseline {candidate!r}; expected fpchecker, eftsan, nsan, or all"
                )
            if candidate not in baselines:
                baselines.append(candidate)
    return baselines


def parse_precisions(flag_values: Sequence[object], positional_values: Sequence[str]) -> List[str]:
    requested = split_requested([*flag_values, *positional_values])
    if not requested:
        return ["fp32"]

    precisions: List[str] = []
    for item in requested:
        item = item.lower()
        candidates = PRECISION_CHOICES if item == "all" else (item,)
        for candidate in candidates:
            if candidate not in PRECISION_CHOICES:
                raise argparse.ArgumentTypeError(
                    f"unknown precision {candidate!r}; expected fp32, fp64, or all"
                )
            if candidate not in precisions:
                precisions.append(candidate)
    return precisions


def parse_opt_levels(values: Sequence[object]) -> List[str]:
    requested = split_requested(values)
    if not requested:
        return ["O2"]

    opt_levels: List[str] = []
    for item in requested:
        candidates = OPT_CHOICES if item.lower() == "all" else (normalize_opt_level(item),)
        for candidate in candidates:
            if candidate not in opt_levels:
                opt_levels.append(candidate)
    return opt_levels


def prepend_path(env: Dict[str, str], name: str, value: Path) -> None:
    old = env.get(name, "")
    env[name] = f"{value}:{old}" if old else str(value)


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

    candidates = []
    conda_prefix = os.environ.get("CONDA_PREFIX")
    if conda_prefix:
        candidates.append(Path(conda_prefix).parent / "envs" / env_name)
    home = Path.home()
    candidates.extend(
        [
            home / "conda_env" / env_name,
            home / "miniconda3" / "envs" / env_name,
            home / "anaconda3" / "envs" / env_name,
            Path("/opt/anaconda3/envs") / env_name,
        ]
    )
    conda_prefix = conda_env_prefixes_by_name().get(env_name)
    if conda_prefix is not None:
        candidates.append(conda_prefix)
    return candidates


def resolve_env_prefix(env_name: str) -> Optional[Path]:
    for candidate in candidate_env_prefixes(env_name):
        if candidate.is_dir():
            return candidate.resolve()
    return None


def unique_names(names: Sequence[str]) -> List[str]:
    result: List[str] = []
    for name in names:
        if name not in result:
            result.append(name)
    return result


def choose_named_env(names: Sequence[str]) -> str:
    names = unique_names(names)
    for env_name in names:
        prefix = resolve_env_prefix(env_name)
        if prefix is not None:
            return env_name
    return names[0]


def choose_default_fpchecker_env() -> str:
    return choose_named_env(DEFAULT_FPCHECKER_ENVS)


def choose_default_nsan_env() -> str:
    fpchecker_env = choose_default_fpchecker_env()
    return choose_named_env((fpchecker_env, *DEFAULT_NSAN_ENVS))


def choose_default_eftsan_env() -> str:
    return choose_named_env(DEFAULT_EFTSAN_ENVS)


def default_conda_sh() -> Optional[Path]:
    candidates = []
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
    return " ".join(shlex.quote(item) for item in command)


def command_with_activation(
    command: Sequence[str],
    env_name: str,
    activation_mode: str,
    extra_shell: Sequence[str],
    env: Dict[str, str],
) -> Tuple[Sequence[str], Dict[str, str], str]:
    if activation_mode == "none":
        return command, env, "current environment"

    prefix = resolve_env_prefix(env_name)
    conda_sh = default_conda_sh()
    conda_available = shutil.which("conda", path=env.get("PATH")) is not None

    if activation_mode in {"auto", "conda"} and (conda_sh is not None or conda_available):
        setup = []
        if conda_sh is not None:
            setup.append(f". {shlex.quote(str(conda_sh))}")
        target = str(prefix) if prefix is not None else env_name
        setup.append(f"conda activate {shlex.quote(target)}")
        setup.extend(extra_shell)
        setup.append(shell_join(command))
        return ["bash", "-lc", " && ".join(setup)], env, f"conda:{target}"

    if activation_mode == "conda":
        raise RuntimeError(
            f"could not find conda activation support for environment {env_name!r}"
        )

    if activation_mode in {"auto", "path"} and prefix is not None:
        direct_env = env.copy()
        direct_env["CONDA_PREFIX"] = str(prefix)
        prepend_path(direct_env, "PATH", prefix / "bin")
        if (prefix / "lib").is_dir():
            prepend_path(direct_env, "LD_LIBRARY_PATH", prefix / "lib")
        return command, direct_env, f"path:{prefix}"

    if activation_mode == "path":
        raise RuntimeError(f"could not find environment directory for {env_name!r}")

    return command, env, "current environment"


def run_child(
    name: str,
    command: Sequence[str],
    cwd: Path,
    log_path: Path,
    env_name: str,
    activation_mode: str,
    extra_env: Optional[Dict[str, str]] = None,
    extra_shell: Sequence[str] = (),
) -> CommandResult:
    env = os.environ.copy()
    if extra_env:
        env.update(extra_env)

    effective_command, effective_env, activation = command_with_activation(
        command,
        env_name,
        activation_mode,
        extra_shell,
        env,
    )
    result = subprocess.run(
        effective_command,
        cwd=str(cwd),
        env=effective_env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    log_path.parent.mkdir(parents=True, exist_ok=True)
    log_path.write_text(result.stdout, encoding="utf-8", errors="replace")
    return CommandResult(
        name=name,
        returncode=result.returncode,
        output=result.stdout,
        log_path=log_path,
        activation=activation,
    )


def parse_fpchecker_rows(output: str, precision: str, opt: str) -> List[BaselineRow]:
    rows: List[BaselineRow] = []
    for line in output.splitlines():
        parts = re.split(r"\s{2,}", line.strip(), maxsplit=8)
        if len(parts) != 9:
            continue
        if parts[0] in {"Benchmark", "Average", "Median"} or set(parts[0]) == {"-"}:
            continue
        rows.append(
            BaselineRow(
                baseline_name="fpchecker",
                precision=precision,
                opt=opt,
                benchmark=parts[0],
                source=source_filename(parts[6]),
                baseline=parse_float(parts[2]),
                error=parse_float(parts[3]),
                diff=parse_float(parts[5]),
                status=parts[8],
            )
        )
    return rows


def source_from_eftsan_entry(entry: dict) -> str:
    file_name = str(entry.get("file", "-"))
    if file_name in {"", "-"}:
        return "-"
    return Path(file_name).name


def parse_eftsan_rows(json_path: Path, precision: str, opt: str) -> List[BaselineRow]:
    with json_path.open("r", encoding="utf-8") as fp:
        data = json.load(fp)
    if not isinstance(data, list):
        raise ValueError(f"{json_path} does not contain a JSON list")

    rows: List[BaselineRow] = []
    for entry in data:
        if not isinstance(entry, dict):
            continue
        rows.append(
            BaselineRow(
                baseline_name="eftsan",
                precision=precision,
                opt=opt,
                benchmark=str(entry.get("benchmark", "-")),
                source=source_from_eftsan_entry(entry),
                baseline=parse_float(entry.get("baseline_error", entry.get("baseline"))),
                error=parse_float(entry.get("error")),
                diff=parse_float(entry.get("absolute_difference")),
                status=str(entry.get("status", "-")),
            )
        )
    return rows


def parse_nsan_rows(output: str, precision: str, opt: str) -> List[BaselineRow]:
    rows: List[BaselineRow] = []
    for line in output.splitlines():
        parts = re.split(r"\s{2,}", line.strip(), maxsplit=10)
        if len(parts) != 11:
            continue
        if parts[0] in {"Benchmark", "Mean", "Median"} or set(parts[0]) == {"-"}:
            continue
        rows.append(
            BaselineRow(
                baseline_name="nsan",
                precision=precision,
                opt=opt,
                benchmark=parts[0],
                source=source_filename(parts[9]),
                baseline=parse_float(parts[4]),
                error=parse_float(parts[7]),
                diff=parse_float(parts[8]),
                status=parts[10],
            )
        )
    return rows


def source_filename(source: str) -> str:
    if source in {"", "-"}:
        return "-"
    return Path(source.split(":", 1)[0]).name


def is_ok_status(row: BaselineRow) -> bool:
    if row.baseline_name == "nsan":
        return row.status.startswith("OK")
    return row.status == "OK"


def keyed_rows(rows: Sequence[BaselineRow]) -> Dict[Tuple[str, str, str, str, int], Dict[str, BaselineRow]]:
    by_key: Dict[Tuple[str, str, str, str, int], Dict[str, BaselineRow]] = {}
    occurrences: Dict[Tuple[str, str, str, str, str], int] = {}
    for row in rows:
        source = source_filename(row.source)
        occurrence_base = (row.precision, row.opt, row.benchmark, source, row.baseline_name)
        occurrence = occurrences.get(occurrence_base, 0)
        occurrences[occurrence_base] = occurrence + 1
        key = (row.precision, row.opt, row.benchmark, source, occurrence)
        by_key.setdefault(key, {})[row.baseline_name] = row
    return by_key


def baseline_value(rows_by_baseline: Dict[str, BaselineRow], baselines: Sequence[str]) -> Optional[float]:
    for baseline in baselines:
        row = rows_by_baseline.get(baseline)
        if row is not None and row.baseline is not None:
            return row.baseline
    return None


def summary_diffs(
    keyed: Dict[Tuple[str, str, str, str, int], Dict[str, BaselineRow]],
    precision: str,
    opt: str,
    baseline: str,
    missing_diff: str,
) -> List[float]:
    diffs: List[float] = []
    for key, rows_by_baseline in keyed.items():
        if key[:2] != (precision, opt):
            continue
        baseline_row = rows_by_baseline.get(baseline)
        if (
            baseline_row is not None
            and is_ok_status(baseline_row)
            and baseline_row.diff is not None
            and math.isfinite(baseline_row.diff)
        ):
            diffs.append(baseline_row.diff)
        elif missing_diff == "zero":
            diffs.append(0.0)
    return diffs


def build_combined_table(
    rows: Sequence[BaselineRow],
    baselines: Sequence[str],
    missing_diff: str = "skip",
) -> str:
    rows = [row for row in rows if row.source != "-"]
    by_key = keyed_rows(rows)

    lines: List[str] = []
    groups = sorted({key[:2] for key in by_key})
    for group_index, (precision, opt) in enumerate(groups):
        if group_index:
            lines.append("")
        lines.append(f"==== {precision} {opt} ====")

        headers = ["Source", "Baseline error"]
        for baseline in baselines:
            label = BASELINE_LABELS[baseline]
            headers.extend([f"{label} error", f"{label} diff"])

        body = []
        for key in sorted(key for key in by_key if key[:2] == (precision, opt)):
            rows_by_baseline = by_key[key]
            _precision, _opt, _benchmark, source, _occurrence = key
            row = [source, fmt_float(baseline_value(rows_by_baseline, baselines))]
            for baseline in baselines:
                baseline_row = rows_by_baseline.get(baseline)
                row.extend(
                    [
                        fmt_float(baseline_row.error if baseline_row else None),
                        fmt_float(baseline_row.diff if baseline_row else None),
                    ]
                )
            body.append(row)

        summary_rows = []
        for label, use_median in [("Average", False), ("Median", True)]:
            row = [label, "-"]
            for baseline in baselines:
                diffs = summary_diffs(by_key, precision, opt, baseline, missing_diff)
                row.extend(["-", fmt_float(aggregate(diffs, use_median))])
            summary_rows.append(row)

        widths = [len(header) for header in headers]
        for row in [*body, *summary_rows]:
            for idx, value in enumerate(row):
                widths[idx] = max(widths[idx], len(value))

        def format_row(row: Sequence[str]) -> str:
            return "  ".join(value.ljust(widths[idx]) for idx, value in enumerate(row))

        separator = "  ".join("=" * width for width in widths)
        lines.append(format_row(headers))
        lines.append(separator)
        lines.extend(format_row(row) for row in body)
        lines.append(separator)
        lines.extend(format_row(row) for row in summary_rows)

    if not lines:
        headers = ["Source", "Baseline error"]
        for baseline in baselines:
            label = BASELINE_LABELS[baseline]
            headers.extend([f"{label} error", f"{label} diff"])
        lines.append("  ".join(headers))
    return "\n".join(lines)


def write_combined_json(rows: Sequence[BaselineRow], path: Path) -> None:
    payload = [
        {
            "baseline": row.baseline_name,
            "precision": row.precision,
            "opt": row.opt,
            "benchmark": row.benchmark,
            "source": row.source,
            "baseline_error": row.baseline,
            "error": row.error,
            "diff": row.diff,
            "status": row.status,
        }
        for row in rows
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as fp:
        json.dump(payload, fp, indent=2)
        fp.write("\n")


def parse_args(argv: Optional[Sequence[str]]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run selected PolyBench error baselines and print a combined error/diff table.",
        epilog=(
            "Examples:\n"
            "  %(prog)s --baselines fpchecker eftsan nsan --opt-level O2 --precision fp32\n"
            "  %(prog)s --baselines all --opt-level O2 O0 --precision fp32 fp64\n"
            "  %(prog)s fp64 --baselines fpchecker nsan --opt-level O0"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "precision_positional",
        nargs="*",
        help="Optional precision(s): fp32, fp64, or all. Kept for compatibility.",
    )
    parser.add_argument(
        "--baseline",
        "--baselines",
        dest="baselines",
        action="append",
        nargs="+",
        default=[],
        help=(
            "Baseline(s) to run: fpchecker, eftsan, nsan, or all. "
            "Can be comma-separated or repeated. Defaults to fpchecker,eftsan."
        ),
    )
    parser.add_argument(
        "--precision",
        "--mode",
        dest="precision",
        action="append",
        nargs="+",
        default=[],
        help="Precision(s): fp32, fp64, or all. Can be comma-separated or repeated.",
    )
    parser.add_argument(
        "--opt-level",
        action="append",
        nargs="+",
        default=[],
        help="Optimization level(s): O0, O1, O2, O3, or all. Can be comma-separated or repeated.",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help="Timeout in seconds passed to each underlying benchmark driver.",
    )
    parser.add_argument(
        "--benchmark",
        action="append",
        default=[],
        help="Substring filter forwarded to all selected drivers. Can be passed more than once.",
    )
    parser.add_argument(
        "--python",
        default="python3",
        help="Python executable used inside the selected environments. Defaults to python3.",
    )
    parser.add_argument(
        "--nsan-clang",
        default=None,
        help="Override clang executable forwarded to the NSan driver.",
    )
    parser.add_argument(
        "--nsan-save-logs",
        action="store_true",
        help="Let the NSan error driver write per-benchmark logs. Off by default to avoid quota issues.",
    )
    parser.add_argument(
        "--results-dir",
        default=str(DEFAULT_RESULTS_DIR),
        help="Directory for child logs and combined output files.",
    )
    parser.add_argument(
        "--missing-diff",
        choices=("zero", "skip"),
        default="skip",
        help=(
            "How summary rows handle displayed sources without a numeric baseline diff. "
            "Defaults to skip, so each baseline summary uses its passed numeric count."
        ),
    )
    parser.add_argument(
        "--interactive",
        action="store_true",
        help="Prompt for missing baselines, opt levels, and precisions.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Also echo each child driver's captured output.",
    )
    return parser.parse_args(argv)


def benchmark_filters(patterns: Sequence[str]) -> List[str]:
    result: List[str] = []
    for pattern in patterns:
        result.extend(["--benchmark", pattern])
    return result


def run_fpchecker(
    args: argparse.Namespace,
    precision: str,
    opt: str,
    results_dir: Path,
) -> Tuple[CommandResult, List[BaselineRow]]:
    cmd = [
        args.python,
        "compare_polybench_fpchecker_up.py",
        precision,
        "--opt-level",
        opt,
        "--timeout",
        str(args.timeout),
        *benchmark_filters(args.benchmark),
    ]
    result = run_child(
        "FPChecker",
        cmd,
        DEFAULT_BENCHMARK_DIR,
        results_dir / f"fpchecker_{precision}_{opt}.log",
        choose_default_fpchecker_env(),
        DEFAULT_ACTIVATION_MODE,
    )
    return result, parse_fpchecker_rows(result.output, precision, opt)


def eftsan_env_settings(env_name: str) -> Tuple[Dict[str, str], List[str]]:
    extra_env: Dict[str, str] = {}
    extra_shell: List[str] = []

    eftsan_prefix = resolve_env_prefix(env_name)
    if eftsan_prefix is not None:
        extra_env["LLVM10"] = str(eftsan_prefix / "bin")
        extra_env["LLVM10_LIB"] = str(eftsan_prefix / "lib")
    else:
        extra_shell.append('export LLVM10="$CONDA_PREFIX/bin" LLVM10_LIB="$CONDA_PREFIX/lib"')
    return extra_env, extra_shell


def run_eftsan(
    args: argparse.Namespace,
    precision: str,
    opt: str,
    results_dir: Path,
) -> Tuple[CommandResult, List[BaselineRow]]:
    json_path = results_dir / f"eftsan_{precision}_{opt}.json"
    report_path = results_dir / f"eftsan_{precision}_{opt}.txt"
    cmd = [
        args.python,
        "generate_eftsan_error_report_up.py",
        precision,
        "--opt-level",
        opt,
        "--timeout",
        str(args.timeout),
        "--json",
        str(json_path),
        "--report",
        str(report_path),
        *benchmark_filters(args.benchmark),
    ]
    eftsan_env = choose_default_eftsan_env()
    extra_env, extra_shell = eftsan_env_settings(eftsan_env)
    result = run_child(
        "EFTSan",
        cmd,
        DEFAULT_EFTSAN_DIR,
        results_dir / f"eftsan_{precision}_{opt}.log",
        eftsan_env,
        DEFAULT_ACTIVATION_MODE,
        extra_env=extra_env,
        extra_shell=extra_shell,
    )
    rows = parse_eftsan_rows(json_path, precision, opt) if json_path.is_file() else []
    return result, rows


def run_nsan(
    args: argparse.Namespace,
    precision: str,
    opt: str,
    results_dir: Path,
) -> Tuple[CommandResult, List[BaselineRow]]:
    cmd = [
        args.python,
        "compare_polybench_nsan_error.py",
        "--mode",
        precision,
        "--opt-level",
        opt,
        "--timeout",
        str(args.timeout),
        *benchmark_filters(args.benchmark),
    ]
    if args.nsan_save_logs:
        cmd.extend(["--log-dir", str(results_dir / f"nsan_logs_{precision}_{opt}")])
    else:
        cmd.append("--no-logs")
    if args.nsan_clang:
        cmd.extend(["--clang", args.nsan_clang])

    result = run_child(
        "NSan",
        cmd,
        DEFAULT_BENCHMARK_DIR,
        results_dir / f"nsan_{precision}_{opt}.log",
        choose_default_nsan_env(),
        DEFAULT_ACTIVATION_MODE,
    )
    return result, parse_nsan_rows(result.output, precision, opt)


def run_baseline(
    baseline: str,
    args: argparse.Namespace,
    precision: str,
    opt: str,
    results_dir: Path,
) -> Tuple[CommandResult, List[BaselineRow]]:
    if baseline == "fpchecker":
        return run_fpchecker(args, precision, opt, results_dir)
    if baseline == "eftsan":
        return run_eftsan(args, precision, opt, results_dir)
    if baseline == "nsan":
        return run_nsan(args, precision, opt, results_dir)
    raise AssertionError(f"unexpected baseline: {baseline}")


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    if args.interactive:
        if not args.baselines:
            value = input("Baselines [fpchecker eftsan]: ").strip()
            if value:
                args.baselines = [value]
        if not args.opt_level:
            value = input("Opt levels [O2]: ").strip()
            if value:
                args.opt_level = [value]
        if not args.precision and not args.precision_positional:
            value = input("Precisions [fp32]: ").strip()
            if value:
                args.precision = [value]

    try:
        baselines = parse_baselines(args.baselines)
        precisions = parse_precisions(args.precision, args.precision_positional)
        opt_levels = parse_opt_levels(args.opt_level)
    except argparse.ArgumentTypeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    results_dir = Path(args.results_dir).resolve()
    all_rows: List[BaselineRow] = []
    command_results: List[CommandResult] = []

    for precision in precisions:
        for opt in opt_levels:
            for baseline in baselines:
                label = BASELINE_LABELS[baseline]
                print(f"Running {label} {precision} {opt}")
                result, rows = run_baseline(baseline, args, precision, opt, results_dir)
                command_results.append(result)
                all_rows.extend(rows)
                print(f"{label} log: {result.log_path}")
                if result.returncode != 0:
                    print(f"warning: {label} exited with {result.returncode}", file=sys.stderr)
                if args.verbose:
                    print(result.output, end="")

    table = build_combined_table(all_rows, baselines, args.missing_diff)
    tag = "_".join([",".join(precisions), ",".join(opt_levels), ",".join(baselines)])
    safe_tag = re.sub(r"[^A-Za-z0-9_.-]+", "_", tag)
    combined_report = results_dir / f"error_baselines_{safe_tag}.txt"
    combined_json = results_dir / f"error_baselines_{safe_tag}.json"
    combined_report.parent.mkdir(parents=True, exist_ok=True)
    combined_report.write_text(table + "\n", encoding="utf-8")
    write_combined_json(all_rows, combined_json)

    print()
    print(table)
    print()
    print(f"Wrote combined report: {combined_report}")
    print(f"Wrote combined JSON: {combined_json}")

    if all_rows:
        return 0
    return 0 if all(result.returncode == 0 for result in command_results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
