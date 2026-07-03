#!/usr/bin/env python3
"""
Build and run PolyBench FPChecker benchmarks, then compare printed baseline
norm errors against FPChecker rounding errors at the corresponding norm lines.

Run from:
  /g/g90/laguna/fpchecker/FPChecker/experiments/benchmark
"""

from __future__ import annotations

import argparse
import json
import math
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


DEFAULT_ROOT = Path("PolyBenchC-4.2.1")
DEFAULT_INSTALL_BIN = Path("/g/g90/laguna/fpchecker/FPChecker/build/install/bin")
FLOAT_RE = r"[+-]?(?:(?:\d+(?:\.\d*)?)|(?:\.\d+))(?:[eE][+-]?\d+)?|[+-]?(?:inf|infinity|nan)"
BASELINE_RE = re.compile(
    rf"^\s*(?:FPChecker-style\s+)?Norm error(?P<label>[^:]*):\s*(?P<value>{FLOAT_RE})\s*$",
    re.IGNORECASE,
)
NORM_ASSIGN_RE = re.compile(r"\b(?P<var>norm[A-Za-z0-9_]*)\s*=\s*SQRT_FUN\s*\(")
NONFINITE_TOKEN_RE = re.compile(
    r"(?<![A-Za-z0-9_.+-])(?P<value>[+-]?(?:inf(?:inity)?|nan))(?![A-Za-z0-9_])",
    re.IGNORECASE,
)


@dataclass
class NormSite:
    label: str
    file_name: str
    line: int
    source: str


@dataclass
class BaselineError:
    label: str
    value: float
    raw_label: str


@dataclass
class Comparison:
    benchmark: str
    label: str
    baseline: Optional[float]
    fpchecker: Optional[float]
    relative_error: Optional[float]
    site: Optional[NormSite]
    diagnostics: str
    status: str


def normalize_label(label: str) -> str:
    value = label.strip().lower()
    value = re.sub(r"\([^)]*\)", "", value).strip()
    if value.startswith("in "):
        value = value[3:].strip()
    value = value.replace("_", " ")
    return re.sub(r"\s+", "", value)


def label_from_norm_variable(var_name: str) -> str:
    suffix = var_name[len("norm") :]
    suffix = suffix.lstrip("_")
    return normalize_label(suffix)


def parse_float(text: str) -> float:
    value = text.strip().lower()
    if value in {"inf", "+inf", "infinity", "+infinity"}:
        return math.inf
    if value in {"-inf", "-infinity"}:
        return -math.inf
    if value in {"nan", "+nan", "-nan"}:
        return math.nan
    return float(value)


def run_cmd(
    cmd: Sequence[str],
    cwd: Path,
    env: Dict[str, str],
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


def discover_benchmarks(root: Path) -> List[Path]:
    benchmarks: List[Path] = []
    for makefile in root.rglob("Makefile"):
        bench_dir = makefile.parent
        if bench_dir.name.endswith("_fp64"):
            continue
        if any(part.endswith("_fp64") for part in bench_dir.parts):
            continue
        benchmarks.append(bench_dir)
    return sorted(benchmarks)


def parse_makefile_outputs(makefile: Path) -> List[str]:
    outputs: List[str] = []
    pattern = re.compile(r"(?:^|\s)-o\s+([^\s]+)")
    try:
        text = makefile.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return outputs
    for match in pattern.finditer(text):
        output = match.group(1)
        if output.startswith("$"):
            continue
        outputs.append(output)
    return outputs


def parse_makefile_sources(makefile: Path) -> List[str]:
    sources: List[str] = []
    pattern = re.compile(r"(?:^|\s)-c\s+([^\s]+\.c)")
    try:
        lines = makefile.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return sources
    seen = set()
    for line in lines:
        if line.lstrip().startswith("#"):
            continue
        for match in pattern.finditer(line):
            source = match.group(1)
            if "/" in source or source in seen:
                continue
            seen.add(source)
            sources.append(source)
    return sources


def find_executable(bench_dir: Path) -> Optional[Path]:
    preferred = [bench_dir.name]
    preferred.extend(parse_makefile_outputs(bench_dir / "Makefile"))

    for name in preferred:
        candidate = bench_dir / name
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate

    executables = []
    for child in bench_dir.iterdir():
        if child.is_file() and os.access(child, os.X_OK):
            if child.suffix in {".py", ".sh"}:
                continue
            executables.append(child)
    if len(executables) == 1:
        return executables[0]
    return None


def parse_baseline_errors(output: str) -> List[BaselineError]:
    errors: List[BaselineError] = []
    for line in output.splitlines():
        if "same FP32 outputs" in line:
            continue
        match = BASELINE_RE.match(line)
        if not match:
            continue
        raw_label = match.group("label").strip()
        errors.append(
            BaselineError(
                label=normalize_label(raw_label),
                value=parse_float(match.group("value")),
                raw_label=raw_label,
            )
        )
    return errors


def parse_norm_sites(bench_dir: Path) -> Dict[str, List[NormSite]]:
    sites: Dict[str, List[NormSite]] = {}
    source_names = parse_makefile_sources(bench_dir / "Makefile")
    if source_names:
        source_files = [bench_dir / name for name in source_names]
    else:
        source_files = sorted(bench_dir.glob("*.c"))

    for source_file in source_files:
        if not source_file.is_file():
            continue
        try:
            lines = source_file.read_text(encoding="utf-8", errors="replace").splitlines()
        except OSError:
            continue
        for line_no, line in enumerate(lines, start=1):
            match = NORM_ASSIGN_RE.search(line)
            if not match:
                continue
            label = label_from_norm_variable(match.group("var"))
            site = NormSite(
                label=label,
                file_name=source_file.name,
                line=line_no,
                source=line.strip(),
            )
            sites.setdefault(label, []).append(site)
    return sites


def latest_rounding_json(bench_dir: Path) -> Optional[Path]:
    log_dir = bench_dir / ".fpc_logs"
    if not log_dir.is_dir():
        return None
    files = list(log_dir.glob("rounding_error_*.json"))
    if not files:
        return None
    return max(files, key=lambda path: path.stat().st_mtime)


def inspect_json_value(value: object, flags: set[str]) -> None:
    if isinstance(value, float):
        if math.isinf(value):
            flags.add("JSON_INF")
        elif math.isnan(value):
            flags.add("JSON_NAN")
        return
    if isinstance(value, dict):
        for child in value.values():
            inspect_json_value(child, flags)
        return
    if isinstance(value, list):
        for child in value:
            inspect_json_value(child, flags)
        return
    if isinstance(value, str):
        lowered = value.strip().lower()
        if lowered in {"inf", "+inf", "-inf", "infinity", "+infinity", "-infinity"}:
            flags.add("JSON_INF")
        elif lowered in {"nan", "+nan", "-nan"}:
            flags.add("JSON_NAN")


def format_diagnostics(flags: Iterable[str]) -> str:
    ordered_flags = [
        "DIV0",
        "SHADOW_DIV0",
        "JSON_INF",
        "JSON_NAN",
        "REPORT_INF",
        "REPORT_NAN",
    ]
    values = [flag for flag in ordered_flags if flag in flags]
    return ",".join(values) if values else "-"


def output_diagnostics(output: str) -> set[str]:
    flags: set[str] = set()
    if "Division by zero" in output:
        flags.add("DIV0")
    if "Shadow denominator canceled to zero" in output:
        flags.add("SHADOW_DIV0")
    return flags


def nonfinite_text_diagnostics(text: str, prefix: str) -> set[str]:
    flags: set[str] = set()
    for match in NONFINITE_TOKEN_RE.finditer(text):
        value = match.group("value").lower()
        if "inf" in value:
            flags.add(f"{prefix}_INF")
        elif "nan" in value:
            flags.add(f"{prefix}_NAN")
    return flags


def report_diagnostics(bench_dir: Path, env: Dict[str, str], timeout: int) -> set[str]:
    if shutil.which("fpc-create-report", path=env.get("PATH", "")) is None:
        return set()
    try:
        report = run_cmd(["fpc-create-report", "-s", "rounding"], bench_dir, env, timeout)
    except OSError:
        return set()
    return nonfinite_text_diagnostics(report.stdout, "REPORT")


def load_fpchecker_errors(bench_dir: Path) -> Tuple[Dict[Tuple[str, int], float], set[str]]:
    json_file = latest_rounding_json(bench_dir)
    if json_file is None:
        return {}, set()
    with json_file.open("r", encoding="utf-8") as fp:
        data = json.load(fp)

    flags: set[str] = set()
    inspect_json_value(data, flags)

    errors: Dict[Tuple[str, int], float] = {}
    for entry in data:
        file_name = Path(str(entry.get("file", ""))).name
        line = int(entry.get("line", -1))
        try:
            error = float(entry.get("error", 0.0))
        except (TypeError, ValueError):
            continue
        errors[(file_name, line)] = error
    return errors, flags


def relative_difference(baseline: float, fpchecker: float) -> float:
    if math.isnan(baseline) or math.isnan(fpchecker):
        return math.nan
    if baseline == 0.0:
        return 0.0 if fpchecker == 0.0 else math.inf
    return abs(baseline - fpchecker) / abs(baseline)


def select_site(label: str, sites: Dict[str, List[NormSite]]) -> Optional[NormSite]:
    matches = sites.get(label, [])
    if len(matches) == 1:
        return matches[0]
    if label == "" and len(sites.get("", [])) == 1:
        return sites[""][0]
    return None


def git_root_for(path: Path) -> Optional[Path]:
    result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        cwd=str(path),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    if result.returncode != 0:
        return None
    return Path(result.stdout.strip())


def deleted_tracked_files(bench_dir: Path) -> set[str]:
    git_root = git_root_for(bench_dir)
    if git_root is None:
        return set()
    rel = bench_dir.resolve().relative_to(git_root.resolve())
    result = subprocess.run(
        ["git", "ls-files", "--deleted", "-z", "--", str(rel)],
        cwd=str(git_root),
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    if result.returncode != 0 or not result.stdout:
        return set()
    return {
        item.decode("utf-8", errors="replace")
        for item in result.stdout.split(b"\0")
        if item
    }


def restore_tracked_files(bench_dir: Path, files: Iterable[str]) -> None:
    file_list = sorted(files)
    if not file_list:
        return
    git_root = git_root_for(bench_dir)
    if git_root is None:
        return
    subprocess.run(
        ["git", "checkout", "--", *file_list],
        cwd=str(git_root),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )


def clean_benchmark(bench_dir: Path, env: Dict[str, str], timeout: int) -> None:
    deleted_before = deleted_tracked_files(bench_dir)
    run_cmd(["make", "clean"], bench_dir, env, timeout)
    for name in (".fpc_logs", ".fpc_log.txt", "fpc-report"):
        path = bench_dir / name
        if path.is_dir():
            shutil.rmtree(path, ignore_errors=True)
        elif path.exists():
            try:
                path.unlink()
            except OSError:
                pass
    deleted_after = deleted_tracked_files(bench_dir)
    restore_tracked_files(bench_dir, deleted_after - deleted_before)


def compare_benchmark(
    bench_dir: Path,
    root: Path,
    env: Dict[str, str],
    timeout: int,
    cleanup: bool,
) -> List[Comparison]:
    benchmark_name = str(bench_dir.relative_to(root))

    if cleanup:
        clean_benchmark(bench_dir, env, timeout)

    build = run_cmd(["make"], bench_dir, env, timeout)
    if build.returncode != 0:
        if cleanup:
            clean_benchmark(bench_dir, env, timeout)
        return [
            Comparison(
                benchmark=benchmark_name,
                label="",
                baseline=None,
                fpchecker=None,
                relative_error=None,
                site=None,
                diagnostics="-",
                status=f"BUILD_FAILED: {last_line(build.stdout)}",
            )
        ]

    executable = find_executable(bench_dir)
    if executable is None:
        if cleanup:
            clean_benchmark(bench_dir, env, timeout)
        return [
            Comparison(
                benchmark=benchmark_name,
                label="",
                baseline=None,
                fpchecker=None,
                relative_error=None,
                site=None,
                diagnostics="-",
                status="NO_EXECUTABLE",
            )
        ]

    run = run_cmd([f"./{executable.name}"], bench_dir, env, timeout)
    output = run.stdout
    diagnostics = output_diagnostics(output)
    if run.returncode != 0:
        if cleanup:
            clean_benchmark(bench_dir, env, timeout)
        return [
            Comparison(
                benchmark=benchmark_name,
                label="",
                baseline=None,
                fpchecker=None,
                relative_error=None,
                site=None,
                diagnostics=format_diagnostics(diagnostics),
                status=f"RUN_FAILED: {last_line(output)}",
            )
        ]

    baseline_errors = parse_baseline_errors(output)
    norm_sites = parse_norm_sites(bench_dir)
    fpchecker_errors, json_diagnostics = load_fpchecker_errors(bench_dir)
    diagnostics.update(json_diagnostics)
    diagnostics.update(report_diagnostics(bench_dir, env, timeout))
    diagnostics_text = format_diagnostics(diagnostics)

    comparisons: List[Comparison] = []
    if not baseline_errors:
        comparisons.append(
            Comparison(
                benchmark=benchmark_name,
                label="",
                baseline=None,
                fpchecker=None,
                relative_error=None,
                site=None,
                diagnostics=diagnostics_text,
                status="NO_BASELINE_ERRORS",
            )
        )
    else:
        for baseline in baseline_errors:
            site = select_site(baseline.label, norm_sites)
            if site is None:
                comparisons.append(
                    Comparison(
                        benchmark=benchmark_name,
                        label=baseline.raw_label or "(default)",
                        baseline=baseline.value,
                        fpchecker=None,
                        relative_error=None,
                        site=None,
                        diagnostics=diagnostics_text,
                        status="NO_MATCHING_NORM_LINE",
                    )
                )
                continue

            fpchecker = fpchecker_errors.get((site.file_name, site.line))
            if fpchecker is None:
                comparisons.append(
                    Comparison(
                        benchmark=benchmark_name,
                        label=baseline.raw_label or "(default)",
                        baseline=baseline.value,
                        fpchecker=None,
                        relative_error=None,
                        site=site,
                        diagnostics=diagnostics_text,
                        status="NO_FPCHECKER_ENTRY",
                    )
                )
                continue

            comparisons.append(
                Comparison(
                    benchmark=benchmark_name,
                    label=baseline.raw_label or "(default)",
                    baseline=baseline.value,
                    fpchecker=fpchecker,
                    relative_error=relative_difference(baseline.value, fpchecker),
                    site=site,
                    diagnostics=diagnostics_text,
                    status="OK",
                )
            )

    if cleanup:
        clean_benchmark(bench_dir, env, timeout)
    return comparisons


def last_line(output: str) -> str:
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    if not lines:
        return ""
    return lines[-1][:120]


def fmt_float(value: Optional[float]) -> str:
    if value is None:
        return "-"
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "inf" if value > 0 else "-inf"
    return f"{value:.6e}"


def print_results(comparisons: Sequence[Comparison]) -> None:
    headers = [
        "Benchmark",
        "Output",
        "Baseline",
        "FPChecker",
        "RelDiff",
        "Source",
        "Diagnostics",
        "Status",
    ]
    rows = []
    for item in comparisons:
        source = "-"
        if item.site is not None:
            source = f"{item.site.file_name}:{item.site.line}"
        rows.append(
            [
                item.benchmark,
                item.label,
                fmt_float(item.baseline),
                fmt_float(item.fpchecker),
                fmt_float(item.relative_error),
                source,
                item.diagnostics,
                item.status,
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

    ok = sum(1 for item in comparisons if item.status == "OK")
    total = len(comparisons)
    failed = total - ok
    print()
    print(f"Summary: {ok}/{total} comparisons OK, {failed} missing/failed.")


def filter_benchmarks(benchmarks: Iterable[Path], patterns: Sequence[str]) -> List[Path]:
    if not patterns:
        return list(benchmarks)
    selected = []
    for bench in benchmarks:
        text = str(bench)
        if any(pattern in text for pattern in patterns):
            selected.append(bench)
    return selected


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Compare PolyBench baseline norm errors against FPChecker rounding traces."
    )
    parser.add_argument(
        "--root",
        default=str(DEFAULT_ROOT),
        help="PolyBench root, relative to the current directory by default.",
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
        help="Timeout in seconds for each make/run/clean command.",
    )
    parser.add_argument(
        "--no-clean",
        action="store_true",
        help="Do not run make clean or remove FPChecker output directories.",
    )
    parser.add_argument(
        "--stop-on-failure",
        action="store_true",
        help="Stop after the first benchmark with a missing/failed comparison.",
    )
    args = parser.parse_args(argv)

    root = Path(args.root).resolve()
    if not root.is_dir():
        print(f"error: benchmark root not found: {root}", file=sys.stderr)
        return 2

    env = os.environ.copy()
    if DEFAULT_INSTALL_BIN.is_dir():
        env["PATH"] = f"{DEFAULT_INSTALL_BIN}:{env.get('PATH', '')}"

    benchmarks = filter_benchmarks(discover_benchmarks(root), args.benchmark)
    if not benchmarks:
        print("error: no benchmarks matched", file=sys.stderr)
        return 2

    all_comparisons: List[Comparison] = []
    for index, bench_dir in enumerate(benchmarks, start=1):
        benchmark_name = str(bench_dir.relative_to(root))
        print(f"[{index}/{len(benchmarks)}] {benchmark_name}", flush=True)
        try:
            comparisons = compare_benchmark(
                bench_dir=bench_dir,
                root=root,
                env=env,
                timeout=args.timeout,
                cleanup=not args.no_clean,
            )
        except subprocess.TimeoutExpired:
            comparisons = [
                Comparison(
                    benchmark=benchmark_name,
                    label="",
                    baseline=None,
                    fpchecker=None,
                    relative_error=None,
                    site=None,
                    diagnostics="-",
                    status="TIMEOUT",
                )
            ]
            if not args.no_clean:
                clean_benchmark(bench_dir, env, args.timeout)
        all_comparisons.extend(comparisons)

        if args.stop_on_failure and any(item.status != "OK" for item in comparisons):
            break

    print()
    print_results(all_comparisons)
    return 0 if all(item.status == "OK" for item in all_comparisons) else 1


if __name__ == "__main__":
    raise SystemExit(main())
