#!/usr/bin/env python3
"""Run PolyBench/C with LLVM NSan probes at norm SQRT_FUN sites.

This script builds temporary instrumented copies of PolyBench benchmark source
files. It does not edit the benchmark directories. For every assignment like:

    norm = SQRT_FUN(sum);

the temporary source adds:

    nsan_check_value(norm);
    nsan_dump_value(norm);

The benchmark is then compiled with ``clang -fsanitize=numerical`` and run with
NSan thresholds suitable for the PolyBench norm-error comparison.
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
import tempfile
from dataclasses import dataclass
from decimal import Decimal, InvalidOperation
from pathlib import Path
from typing import Iterable, Optional, Sequence


DEFAULT_ROOT = Path("PolyBenchC-4.2.1")
DEFAULT_EXTRA_FLAGS = "-fno-vectorize -fno-slp-vectorize"
DEFAULT_NSAN_FLAGS = "-fsanitize=numerical"
DEFAULT_NSAN_OPTIONS = (
    "halt_on_error=0:"
     "log2_max_relative_error=100:"
    # "log2_absolute_error_threshold=10"
)
DEFAULT_LOG_DIR = Path("nsan_report")

FLOAT_RE = (
    r"[+-]?(?:(?:\d+(?:\.\d*)?)|(?:\.\d+))(?:[eE][+-]?\d+)?"
    r"|[+-]?(?:inf|infinity|nan)"
)
BASELINE_RE = re.compile(
    rf"^\s*(?:FPChecker-style\s+)?Norm error(?P<label>[^:]*):\s*(?P<value>{FLOAT_RE})\s*$",
    re.IGNORECASE,
)
NORM_SQRT_RE = re.compile(
    r"(?P<prefix>\b(?P<var>norm[A-Za-z0-9_]*)\s*=\s*)"
    r"SQRT_FUN\s*\((?P<arg>[^;]*)\)(?P<suffix>\s*;)"
)
DUMP_VALUE_RE = re.compile(rf"^value\s+dec:\s*(?P<value>{FLOAT_RE})\b", re.IGNORECASE)
DUMP_SHADOW_RE = re.compile(rf"^shadow\s+dec:\s*(?P<value>{FLOAT_RE})\b", re.IGNORECASE)
COMPILE_SOURCE_RE = re.compile(r"(?:^|\s)-c\s+(?P<source>[^\s]+\.c)")
OUTPUT_RE = re.compile(r"(?:^|\s)-o\s+(?P<output>[^\s]+)")
ASSIGNMENT_RE = re.compile(
    r"^(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*(?:\?=|:=|=)\s*(?P<value>.*)$"
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
class NSanResult:
    benchmark: str
    output: str
    mode: str
    opt: str
    baseline: Optional[float]
    native: Optional[float]
    shadow: Optional[float]
    nsan: Optional[float]
    delta: Optional[float]
    source: str
    status: str


def normalize_opt_level(value: str) -> str:
    opt = value.strip()
    if opt.startswith("-"):
        opt = opt[1:]
    if opt in {"0", "1", "2", "3"}:
        opt = f"O{opt}"
    if opt not in {"O0", "O1", "O2", "O3"}:
        raise argparse.ArgumentTypeError("expected one of O0, O1, O2, O3")
    return opt


def normalize_label(label: str) -> str:
    value = label.strip().lower()
    value = re.sub(r"\([^)]*\)", "", value).strip()
    if value.startswith("in "):
        value = value[3:].strip()
    value = value.replace("_", " ")
    return re.sub(r"\s+", "", value)


def label_from_norm_variable(var_name: str) -> str:
    suffix = var_name[len("norm") :].lstrip("_")
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


def parse_decimal(text: str) -> Decimal:
    value = text.strip().lower()
    if value in {"inf", "+inf", "infinity", "+infinity"}:
        return Decimal("Infinity")
    if value in {"-inf", "-infinity"}:
        return Decimal("-Infinity")
    if value in {"nan", "+nan", "-nan"}:
        return Decimal("NaN")
    try:
        return Decimal(value)
    except InvalidOperation:
        return Decimal(str(parse_float(text)))


def fmt_float(value: Optional[float]) -> str:
    if value is None:
        return "-"
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "inf" if value > 0 else "-inf"
    return f"{value:.6e}"


def parse_makefile_assignment(lines: Sequence[str], name: str, default: str) -> str:
    for line in lines:
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        match = ASSIGNMENT_RE.match(stripped)
        if match and match.group("name") == name:
            return match.group("value").strip()
    return default


def parse_makefile_sources(makefile: Path) -> list[str]:
    try:
        lines = makefile.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return []
    sources: list[str] = []
    seen: set[str] = set()
    for line in lines:
        if line.lstrip().startswith("#"):
            continue
        for match in COMPILE_SOURCE_RE.finditer(line):
            source = match.group("source")
            if source.startswith("$") or source in seen:
                continue
            seen.add(source)
            sources.append(source)
    return sources


def parse_makefile_outputs(makefile: Path) -> list[str]:
    try:
        text = makefile.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return []
    outputs: list[str] = []
    for match in OUTPUT_RE.finditer(text):
        output = match.group("output")
        if output.startswith("$") or output.endswith(".o"):
            continue
        if output not in outputs:
            outputs.append(output)
    return outputs


def executable_name(bench_dir: Path) -> str:
    outputs = parse_makefile_outputs(bench_dir / "Makefile")
    return outputs[-1] if outputs else bench_dir.name


def object_name_for(source: str, used: set[str]) -> str:
    stem = re.sub(r"[^A-Za-z0-9_.-]+", "_", source).replace("/", "_")
    if stem.endswith(".c"):
        stem = stem[:-2]
    candidate = f"{stem}.o"
    index = 2
    while candidate in used:
        candidate = f"{stem}.{index}.o"
        index += 1
    used.add(candidate)
    return candidate


def write_nsan_helper(header_path: Path) -> None:
    header_path.write_text(
        "\n".join(
            [
                "#ifndef NSAN_CHECK_HELPER_H",
                "#define NSAN_CHECK_HELPER_H",
                "void __nsan_check_float(float x);",
                "void __nsan_check_double(double x);",
                "void __nsan_check_longdouble(long double x);",
                "void __nsan_dump_float(float x);",
                "void __nsan_dump_double(double x);",
                "void __nsan_dump_longdouble(long double x);",
                "#define nsan_check_value(x) _Generic((x), float: __nsan_check_float, double: __nsan_check_double, long double: __nsan_check_longdouble)(x)",
                "#define nsan_dump_value(x) _Generic((x), float: __nsan_dump_float, double: __nsan_dump_double, long double: __nsan_dump_longdouble)(x)",
                "#endif",
                "",
            ]
        ),
        encoding="utf-8",
    )


def instrument_source_text(text: str) -> tuple[str, int]:
    count = 0

    def replace(match: re.Match[str]) -> str:
        nonlocal count
        count += 1
        var_name = match.group("var")
        return (
            f"{match.group('prefix')}SQRT_FUN({match.group('arg')})"
            f"{match.group('suffix')} nsan_check_value({var_name});"
            f" nsan_dump_value({var_name});"
        )

    return NORM_SQRT_RE.sub(replace, text), count


def prepare_sources(
    bench_dir: Path,
    sources: Sequence[str],
    temp_dir: Path,
) -> tuple[list[str], list[NormSite]]:
    source_dir = temp_dir / "sources"
    source_dir.mkdir(parents=True, exist_ok=True)
    prepared: list[str] = []
    norm_sites: list[NormSite] = []

    for source in sources:
        original = (bench_dir / source).resolve()
        if not original.is_file():
            prepared.append(source)
            continue

        text = original.read_text(encoding="utf-8", errors="replace")
        instrumented, count = instrument_source_text(text)
        if count == 0:
            prepared.append(source)
            continue

        target = source_dir / Path(source).name
        target.write_text(instrumented, encoding="utf-8")
        prepared.append(str(target))

        for line_no, line in enumerate(text.splitlines(), start=1):
            match = NORM_SQRT_RE.search(line)
            if not match:
                continue
            norm_sites.append(
                NormSite(
                    label=label_from_norm_variable(match.group("var")),
                    file_name=Path(source).name,
                    line=line_no,
                    source=line.strip(),
                )
            )

    return prepared, norm_sites


def discover_benchmarks(root: Path, mode: str) -> list[Path]:
    benchmarks: list[Path] = []
    for makefile in root.rglob("Makefile"):
        bench_dir = makefile.parent
        rel_parts = bench_dir.relative_to(root).parts
        is_fp64 = any(part.endswith("_fp64") for part in rel_parts)
        is_mpfr = any(part.lower().endswith("_mpfr") for part in rel_parts)
        if is_mpfr:
            continue
        if mode == "fp32" and is_fp64:
            continue
        if mode == "fp64" and not is_fp64:
            continue
        benchmarks.append(bench_dir)
    return sorted(benchmarks)


def filter_benchmarks(benchmarks: Iterable[Path], patterns: Sequence[str]) -> list[Path]:
    if not patterns:
        return list(benchmarks)
    selected: list[Path] = []
    for bench in benchmarks:
        text = str(bench)
        if any(pattern in text for pattern in patterns):
            selected.append(bench)
    return selected


def benchmark_mode(bench_dir: Path, root: Path) -> str:
    rel_parts = bench_dir.relative_to(root).parts
    return "fp64" if any(part.endswith("_fp64") for part in rel_parts) else "fp32"


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


def parse_baseline_errors(output: str) -> list[BaselineError]:
    errors: list[BaselineError] = []
    for line in output.splitlines():
        if "same FP32 outputs" in line or "same FP64 outputs" in line:
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


def parse_nsan_dumps(output: str, sites: Sequence[NormSite]) -> dict[str, tuple[float, float, float]]:
    diagnostics: dict[str, tuple[float, float, float]] = {}
    pending_native: Optional[Decimal] = None
    site_index = 0

    for line in output.splitlines():
        stripped = line.strip()
        native_match = DUMP_VALUE_RE.match(stripped)
        if native_match:
            pending_native = parse_decimal(native_match.group("value"))
            continue

        shadow_match = DUMP_SHADOW_RE.match(stripped)
        if not shadow_match or pending_native is None:
            continue

        if site_index >= len(sites):
            pending_native = None
            continue

        site = sites[site_index]
        shadow = parse_decimal(shadow_match.group("value"))
        diagnostics[site.label] = (
            float(pending_native),
            float(shadow),
            float(shadow - pending_native),
        )
        site_index += 1
        pending_native = None

    return diagnostics


def safe_benchmark_name(benchmark: str) -> str:
    return benchmark.replace("/", "__")


def save_output(output: str, log_root: Optional[Path], benchmark: str) -> None:
    if log_root is None:
        return
    log_root.mkdir(parents=True, exist_ok=True)
    (log_root / f"{safe_benchmark_name(benchmark)}.log").write_text(output, encoding="utf-8")


def save_instrumented_sources(
    sources: Sequence[str],
    temp_path: Path,
    log_root: Optional[Path],
    benchmark: str,
) -> None:
    if log_root is None:
        return
    destination = log_root / "instrumented" / safe_benchmark_name(benchmark)
    destination.mkdir(parents=True, exist_ok=True)
    source_root = temp_path / "sources"
    for source in sources:
        path = Path(source)
        if source_root in path.parents:
            shutil.copy2(path, destination / path.name)


def last_line(output: str) -> str:
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    return lines[-1][:160] if lines else ""


def build_and_run_benchmark(
    bench_dir: Path,
    root: Path,
    opt_name: str,
    opt_flags: str,
    clang: str,
    nsan_flags: str,
    nsan_options: str,
    timeout: int,
    cleanup_after: bool,
    log_root: Optional[Path],
) -> list[NSanResult]:
    benchmark = str(bench_dir.relative_to(root))
    mode = benchmark_mode(bench_dir, root)
    env = os.environ.copy()
    env["CCACHE_DISABLE"] = "1"
    env["NSAN_OPTIONS"] = nsan_options

    makefile = bench_dir / "Makefile"
    lines = makefile.read_text(encoding="utf-8", errors="replace").splitlines()
    sources = parse_makefile_sources(makefile)
    if not sources:
        sources = [path.name for path in sorted(bench_dir.glob("*.c"))]

    include_path = parse_makefile_assignment(lines, "INCLUDE_PATH", "-I. -I../../../utilities")
    dataset = parse_makefile_assignment(lines, "DATASET", "-DMINI_DATASET")
    dump = "-DPOLYBENCH_DUMP_ARRAYS"
    exe = executable_name(bench_dir)

    with tempfile.TemporaryDirectory(prefix="nsan-polybench-") as temp_name:
        temp_path = Path(temp_name)
        helper = temp_path / "nsan_check_helper.h"
        write_nsan_helper(helper)
        prepared_sources, norm_sites = prepare_sources(bench_dir, sources, temp_path)
        save_instrumented_sources(prepared_sources, temp_path, log_root, benchmark)

        if not norm_sites:
            return [
                NSanResult(
                    benchmark=benchmark,
                    output="-",
                    mode=mode,
                    opt=opt_name,
                    baseline=None,
                    native=None,
                    shadow=None,
                    nsan=None,
                    delta=None,
                    source="-",
                    status="NO_SQRT_NORM_SITE",
                )
            ]

        used_objects: set[str] = set()
        objects: list[str] = []
        combined_output: list[str] = []

        try:
            for source in prepared_sources:
                obj = object_name_for(source, used_objects)
                objects.append(obj)
                compile_cmd = [
                    *clang.split(),
                    *nsan_flags.split(),
                    "-g",
                    "-include",
                    str(helper),
                    "-c",
                    source,
                    "-o",
                    obj,
                    *opt_flags.split(),
                    *include_path.split(),
                    *dataset.split(),
                    dump,
                ]
                result = run_cmd(compile_cmd, bench_dir, env, timeout)
                combined_output.append(" ".join(compile_cmd))
                combined_output.append(result.stdout)
                if result.returncode != 0:
                    output = "\n".join(combined_output)
                    save_output(output, log_root, benchmark)
                    return [
                        NSanResult(
                            benchmark=benchmark,
                            output="-",
                            mode=mode,
                            opt=opt_name,
                            baseline=None,
                            native=None,
                            shadow=None,
                            nsan=None,
                            delta=None,
                            source="-",
                            status=f"BUILD_FAILED: {last_line(result.stdout)}",
                        )
                    ]

            link_cmd = [*clang.split(), *nsan_flags.split(), "-g", "-o", exe, *objects, "-lm"]
            result = run_cmd(link_cmd, bench_dir, env, timeout)
            combined_output.append(" ".join(link_cmd))
            combined_output.append(result.stdout)
            if result.returncode != 0:
                output = "\n".join(combined_output)
                save_output(output, log_root, benchmark)
                return [
                    NSanResult(
                        benchmark=benchmark,
                        output="-",
                        mode=mode,
                        opt=opt_name,
                        baseline=None,
                        native=None,
                        shadow=None,
                        nsan=None,
                        delta=None,
                        source="-",
                        status=f"LINK_FAILED: {last_line(result.stdout)}",
                    )
                ]

            run = run_cmd(["bash", "-lc", f"ulimit -s 8192; ./{exe}"], bench_dir, env, timeout)
            combined_output.append(f'ulimit -s 8192; NSAN_OPTIONS="{nsan_options}" ./{exe}')
            combined_output.append(run.stdout)
            combined_output.append(f"exit code: {run.returncode}")
        except subprocess.TimeoutExpired:
            output = "\n".join(combined_output)
            save_output(output, log_root, benchmark)
            return [
                NSanResult(
                    benchmark=benchmark,
                    output="-",
                    mode=mode,
                    opt=opt_name,
                    baseline=None,
                    native=None,
                    shadow=None,
                    nsan=None,
                    delta=None,
                    source="-",
                    status="TIMEOUT",
                )
            ]

        output = "\n".join(combined_output)
        save_output(output, log_root, benchmark)

        baseline_errors = parse_baseline_errors(run.stdout)
        nsan_dumps = parse_nsan_dumps(run.stdout, norm_sites)
        site_by_label = {site.label: site for site in norm_sites}

        rows: list[NSanResult] = []
        for baseline in baseline_errors:
            site = site_by_label.get(baseline.label)
            if site is None:
                rows.append(
                    NSanResult(
                        benchmark=benchmark,
                        output=baseline.raw_label or "(default)",
                        mode=mode,
                        opt=opt_name,
                        baseline=baseline.value,
                        native=None,
                        shadow=None,
                        nsan=None,
                        delta=None,
                        source="-",
                        status="NO_MATCHING_SQRT_SITE",
                    )
                )
                continue

            diagnostic = nsan_dumps.get(baseline.label)
            if diagnostic is None:
                rows.append(
                    NSanResult(
                        benchmark=benchmark,
                        output=baseline.raw_label or "(default)",
                        mode=mode,
                        opt=opt_name,
                        baseline=baseline.value,
                        native=None,
                        shadow=None,
                        nsan=None,
                        delta=None,
                        source=f"{site.file_name}:{site.line}",
                        status="NO_NSAN_DUMP",
                    )
                )
                continue

            native, shadow, nsan_error = diagnostic
            delta = abs(baseline.value - nsan_error)
            rows.append(
                NSanResult(
                    benchmark=benchmark,
                    output=baseline.raw_label or "(default)",
                    mode=mode,
                    opt=opt_name,
                    baseline=baseline.value,
                    native=native,
                    shadow=shadow,
                    nsan=nsan_error,
                    delta=delta,
                    source=f"{site.file_name}:{site.line}",
                    status="OK_NSAN_DUMP" if run.returncode == 0 else f"RUN_FAILED:{run.returncode}",
                )
            )

        if not rows:
            status = "NO_NORM_ERROR" if run.returncode == 0 else f"RUN_FAILED:{run.returncode}"
            rows.append(
                NSanResult(
                    benchmark=benchmark,
                    output="-",
                    mode=mode,
                    opt=opt_name,
                    baseline=None,
                    native=None,
                    shadow=None,
                    nsan=None,
                    delta=None,
                    source="-",
                    status=status,
                )
            )

        if cleanup_after:
            for obj in objects:
                (bench_dir / obj).unlink(missing_ok=True)
            (bench_dir / exe).unlink(missing_ok=True)

        return rows


def ok_results(results: Sequence[NSanResult]) -> list[NSanResult]:
    return [
        item
        for item in results
        if item.status.startswith("OK")
        and item.baseline is not None
        and item.native is not None
        and item.shadow is not None
        and item.nsan is not None
        and item.delta is not None
        and all(
            math.isfinite(value)
            for value in [item.baseline, item.native, item.shadow, item.nsan, item.delta]
        )
    ]


def numeric_values(results: Sequence[NSanResult], field: str) -> list[float]:
    values: list[float] = []
    for item in ok_results(results):
        value = getattr(item, field)
        if value is not None:
            values.append(value)
    return values


def aggregate_row(results: Sequence[NSanResult], label: str, use_median: bool) -> list[str]:
    valid = ok_results(results)
    if not valid:
        return [label, "-", "-", "-", "-", "-", "-", "-", "-", "0 passed", label.upper()]

    def aggregate(field: str) -> Optional[float]:
        values = numeric_values(valid, field)
        if not values:
            return None
        return statistics.median(values) if use_median else statistics.mean(values)

    return [
        label,
        "-",
        "-",
        "-",
        fmt_float(aggregate("baseline")),
        fmt_float(aggregate("native")),
        fmt_float(aggregate("shadow")),
        fmt_float(aggregate("nsan")),
        fmt_float(aggregate("delta")),
        f"{len(valid)}/{len(results)} passed",
        label.upper(),
    ]


def print_results(results: Sequence[NSanResult]) -> None:
    headers = [
        "Benchmark",
        "Output",
        "Mode",
        "Opt",
        "Baseline",
        "Native",
        "Shadow",
        "NSAN",
        "Delta",
        "Source",
        "Status",
    ]
    rows = [
        [
            item.benchmark,
            item.output,
            item.mode,
            item.opt,
            fmt_float(item.baseline),
            fmt_float(item.native),
            fmt_float(item.shadow),
            fmt_float(item.nsan),
            fmt_float(item.delta),
            item.source,
            item.status,
        ]
        for item in results
    ]
    summary_rows = [
        aggregate_row(results, "Mean", False),
        aggregate_row(results, "Median", True),
    ]

    all_rows = [*rows, *summary_rows]
    widths = [len(header) for header in headers]
    for row in all_rows:
        for idx, value in enumerate(row):
            widths[idx] = max(widths[idx], len(value))

    print("  ".join(header.ljust(widths[idx]) for idx, header in enumerate(headers)))
    print("  ".join("-" * width for width in widths))
    for row in rows:
        print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(row)))
    print("  ".join("-" * width for width in widths))
    for row in summary_rows:
        print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(row)))


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Run PolyBench/C with NSan probes at norm SQRT_FUN sites."
    )
    parser.add_argument("--mode", choices=("fp32", "fp64", "all"), default="all")
    parser.add_argument("--root", default=str(DEFAULT_ROOT))
    parser.add_argument("--benchmark", action="append", default=[])
    parser.add_argument("--opt-level", type=normalize_opt_level, default="O2")
    parser.add_argument("--opt-flags", default=None)
    parser.add_argument("--clang", default=os.environ.get("CLANG", "clang"))
    parser.add_argument("--nsan-flags", default=DEFAULT_NSAN_FLAGS)
    parser.add_argument("--nsan-options", default=DEFAULT_NSAN_OPTIONS)
    parser.add_argument("--timeout", type=int, default=300)
    parser.add_argument("--log-dir", default=str(DEFAULT_LOG_DIR))
    parser.add_argument("--no-logs", action="store_true")
    parser.add_argument("--no-clean-after", action="store_true")
    parser.add_argument("--stop-on-failure", action="store_true")
    args = parser.parse_args(argv)

    root = Path(args.root).resolve()
    if not root.is_dir():
        print(f"error: benchmark root not found: {root}", file=sys.stderr)
        return 2

    if shutil.which(args.clang.split()[0]) is None:
        print(f"error: clang not found: {args.clang}", file=sys.stderr)
        return 2

    opt_flags = args.opt_flags or f"-{args.opt_level} {DEFAULT_EXTRA_FLAGS}"
    modes = ["fp32", "fp64"] if args.mode == "all" else [args.mode]
    benchmarks: list[Path] = []
    for mode in modes:
        benchmarks.extend(discover_benchmarks(root, mode))
    benchmarks = filter_benchmarks(benchmarks, args.benchmark)

    if not benchmarks:
        print("error: no benchmarks matched", file=sys.stderr)
        return 2

    log_root = None if args.no_logs else Path(args.log_dir).resolve()

    # print(f"Using NSan compiler: {args.clang} {args.nsan_flags}")
    # print(f"Using Makefile OP override: {opt_flags}")
    # print(f"Using NSAN_OPTIONS: {args.nsan_options}")
    if log_root is not None:
        print(f"Saving logs to: {log_root}")

    results: list[NSanResult] = []
    for index, bench_dir in enumerate(benchmarks, start=1):
        benchmark_name = str(bench_dir.relative_to(root))
        print(f"[{index}/{len(benchmarks)}] {benchmark_name}", flush=True)
        benchmark_results = build_and_run_benchmark(
            bench_dir=bench_dir,
            root=root,
            opt_name=args.opt_level,
            opt_flags=opt_flags,
            clang=args.clang,
            nsan_flags=args.nsan_flags,
            nsan_options=args.nsan_options,
            timeout=args.timeout,
            cleanup_after=not args.no_clean_after,
            log_root=log_root,
        )
        results.extend(benchmark_results)
        if args.stop_on_failure and any(not item.status.startswith("OK") for item in benchmark_results):
            break

    print()
    print_results(results)
    return 0 if all(item.status.startswith("OK") for item in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
