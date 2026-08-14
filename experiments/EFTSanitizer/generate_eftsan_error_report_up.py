#!/usr/bin/env python3
"""
Build and run EFTSanitizer PolyBench FP32 benchmarks, then report the
rounding errors captured by each benchmark directory's run.sh script.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import statistics
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_ROOT = Path("PolyBenchC-4.2.1-eftsan")
DEFAULT_REPORT = Path("eftsan_error_report.txt")
DEFAULT_JSON = Path("eftsan_errors_combined.json")
DEFAULT_LLVM10 = Path("/g/g90/sharmin1/conda_env/llvm10/bin")
DEFAULT_LLVM10_LIB = Path("/g/g90/sharmin1/conda_env/llvm10/lib")
DEFAULT_OPT_LEVEL = "O0"


@dataclass
class EftsanEntry:
    benchmark: str
    file: str
    line: int
    baseline_error: Optional[float]
    error: Optional[float]
    absolute_difference: Optional[float]
    status: str


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


def last_line(output: str) -> str:
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    return lines[-1][:120] if lines else ""


def fmt_float(value: Optional[float]) -> str:
    if value is None:
        return "-"
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "inf" if value > 0 else "-inf"
    return f"{value:.6e}"


def parse_float(value: object) -> Optional[float]:
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def absolute_difference(
    baseline_error: Optional[float],
    error: Optional[float],
) -> Optional[float]:
    if baseline_error is None or error is None:
        return None
    return abs(baseline_error - error)


def entry_status(error: Optional[float], baseline_error: Optional[float]) -> str:
    if error is None and baseline_error is None:
        return "BAD_ERROR_AND_BASELINE_VALUE"
    if error is None:
        return "BAD_ERROR_VALUE"
    if baseline_error is None:
        return "BAD_BASELINE_VALUE"
    return "OK"


def normalize_opt_level(value: str) -> str:
    opt = value.strip()
    if opt.startswith("-"):
        opt = opt[1:]
    if opt in {"0", "1", "2", "3"}:
        opt = f"O{opt}"
    if opt not in {"O0", "O1", "O2", "O3"}:
        raise argparse.ArgumentTypeError("expected one of O0, O1, O2, O3")
    return opt


def discover_benchmark_dirs(
    benchmarks_root: Path,
    include_missing_run_sh: bool,
    precision: str,
) -> List[Path]:
    if not benchmarks_root.is_dir():
        return []
    benchmark_dirs = sorted(makefile.parent for makefile in benchmarks_root.rglob("Makefile"))
    if precision == "fp64":
        benchmark_dirs = [
            path for path in benchmark_dirs if any(part.endswith("_fp64") for part in path.parts)
        ]
    else:
        benchmark_dirs = [
            path for path in benchmark_dirs if not any(part.endswith("_fp64") for part in path.parts)
        ]
    if include_missing_run_sh:
        return benchmark_dirs
    return [path for path in benchmark_dirs if (path / "run.sh").is_file()]


def filter_benchmark_dirs(benchmark_dirs: Iterable[Path], patterns: Sequence[str]) -> List[Path]:
    selected = []
    for benchmark_dir in benchmark_dirs:
        text = str(benchmark_dir)
        if not patterns or any(pattern in text for pattern in patterns):
            selected.append(benchmark_dir)
    return selected


def load_eftsan_json(json_file: Path) -> Tuple[List[dict], str]:
    try:
        with json_file.open("r", encoding="utf-8") as fp:
            data = json.load(fp)
    except OSError as exc:
        return [], f"json_read_failed:{exc}"
    except json.JSONDecodeError as exc:
        return [], f"json_parse_failed:{exc}"
    if not isinstance(data, list):
        return [], "json_not_list"
    return [item for item in data if isinstance(item, dict)], "-"


def compare_benchmark(
    benchmark_dir: Path,
    benchmarks_root: Path,
    env: Dict[str, str],
    timeout: int,
    cleanup: bool,
    llvm10: Path,
    llvm10_lib: Path,
    opt_flags: str,
) -> List[EftsanEntry]:
    benchmark = str(benchmark_dir.relative_to(benchmarks_root))
    run_script = benchmark_dir / "run.sh"
    if not run_script.is_file():
        return [
            EftsanEntry(
                benchmark=benchmark,
                file="-",
                line=-1,
                baseline_error=None,
                error=None,
                absolute_difference=None,
                status="NO_RUN_SH",
            )
        ]

    if cleanup:
        clean = run_cmd(["make", "clean"], benchmark_dir, env, timeout)
        if clean.returncode != 0:
            return [
                EftsanEntry(
                    benchmark=benchmark,
                    file="-",
                    line=-1,
                    baseline_error=None,
                    error=None,
                    absolute_difference=None,
                    status=f"CLEAN_FAILED: {last_line(clean.stdout)}",
                )
            ]

    build = run_cmd(
        ["make", f"LLVM10={llvm10}", f"LLVM10_LIB={llvm10_lib}", f"OPT_LEVEL={opt_flags}"],
        benchmark_dir,
        env,
        timeout,
    )
    if build.returncode != 0:
        return [
            EftsanEntry(
                benchmark=benchmark,
                file="-",
                line=-1,
                baseline_error=None,
                error=None,
                absolute_difference=None,
                status=f"BUILD_FAILED: {last_line(build.stdout)}",
            )
        ]

    run_env = env.copy()
    run_env["LLVM10_LIB"] = str(llvm10_lib)
    run = run_cmd(["./run.sh"], benchmark_dir, run_env, timeout)
    if run.returncode != 0:
        return [
            EftsanEntry(
                benchmark=benchmark,
                file="-",
                line=-1,
                baseline_error=None,
                error=None,
                absolute_difference=None,
                status=f"RUN_FAILED: {last_line(run.stdout)}",
            )
        ]

    json_file = benchmark_dir / "eftsan_errors.json"
    items, json_status = load_eftsan_json(json_file)
    if json_status != "-":
        return [
            EftsanEntry(
                benchmark=benchmark,
                file=str(json_file),
                line=-1,
                baseline_error=None,
                error=None,
                absolute_difference=None,
                status="JSON_FAILED",
            )
        ]
    if not items:
        return [
            EftsanEntry(
                benchmark=benchmark,
                file=str(json_file),
                line=-1,
                baseline_error=None,
                error=None,
                absolute_difference=None,
                status="NO_EFTSAN_ERRORS",
            )
        ]

    entries: List[EftsanEntry] = []
    for item in items:
        try:
            line = int(item.get("line", -1))
        except (TypeError, ValueError):
            line = -1
        error = parse_float(item.get("error"))
        baseline_error = parse_float(
            item.get("baseline_error", item.get("baseline"))
        )
        diff = absolute_difference(baseline_error, error)
        entries.append(
            EftsanEntry(
                benchmark=benchmark,
                file=str(item.get("file", "-")),
                line=line,
                baseline_error=baseline_error,
                error=error,
                absolute_difference=diff,
                status=entry_status(error, baseline_error),
            )
        )
    return entries


def build_rows(entries: Sequence[EftsanEntry]) -> Tuple[List[str], List[List[str]]]:
    headers = [
        "benchmark",
        "source",
        "line",
        "baseline_error",
        "EFTSan error",
        "absolute_difference",
        "status",
    ]
    rows = []
    for entry in entries:
        source = Path(entry.file).name if entry.file not in {"", "-"} else "-"
        rows.append(
            [
                entry.benchmark,
                source,
                "-" if entry.line < 0 else str(entry.line),
                fmt_float(entry.baseline_error),
                fmt_float(entry.error),
                fmt_float(entry.absolute_difference),
                entry.status,
            ]
        )
    return headers, rows


def passed_entries(entries: Sequence[EftsanEntry]) -> List[EftsanEntry]:
    passed = []
    for entry in entries:
        if (
            entry.status != "OK"
            or entry.baseline_error is None
            or entry.error is None
            or entry.absolute_difference is None
        ):
            continue
        if not (
            math.isfinite(entry.baseline_error)
            and math.isfinite(entry.error)
            and math.isfinite(entry.absolute_difference)
        ):
            continue
        passed.append(entry)
    return passed


def average_row(entries: Sequence[EftsanEntry]) -> List[str]:
    passed = passed_entries(entries)
    passed_count = len(passed)
    baseline_average = None
    error_average = None
    difference_average = None
    if passed_count:
        baseline_average = sum(entry.baseline_error for entry in passed) / passed_count
        error_average = sum(entry.error for entry in passed) / passed_count
        difference_average = sum(entry.absolute_difference for entry in passed) / passed_count

    return [
        "Average",
        "-",
        "-",
        fmt_float(baseline_average),
        fmt_float(error_average),
        fmt_float(difference_average),
        f"AVERAGE ({passed_count} passed)",
    ]


def median_row(entries: Sequence[EftsanEntry]) -> List[str]:
    passed = passed_entries(entries)
    passed_count = len(passed)
    return [
        "Median",
        "-",
        "-",
        fmt_float(
            statistics.median(entry.baseline_error for entry in passed)
            if passed
            else None
        ),
        fmt_float(
            statistics.median(entry.error for entry in passed)
            if passed
            else None
        ),
        fmt_float(
            statistics.median(entry.absolute_difference for entry in passed)
            if passed
            else None
        ),
        f"MEDIAN ({passed_count} passed)",
    ]


def format_table(entries: Sequence[EftsanEntry]) -> str:
    headers, rows = build_rows(entries)
    avg_row = average_row(entries)
    med_row = median_row(entries)
    widths = [len(header) for header in headers]
    for row in [*rows, avg_row, med_row]:
        for idx, value in enumerate(row):
            widths[idx] = max(widths[idx], len(value))

    separator = "  ".join("-" * width for width in widths)
    lines = [
        "  ".join(header.ljust(widths[idx]) for idx, header in enumerate(headers)),
        separator,
    ]
    lines.extend(
        "  ".join(value.ljust(widths[idx]) for idx, value in enumerate(row))
        for row in rows
    )
    lines.append(separator)
    lines.append("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(avg_row)))
    lines.append("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(med_row)))
    lines.append("")
    lines.append(summary_text(entries))
    return "\n".join(lines)


def summary_text(entries: Sequence[EftsanEntry]) -> str:
    ok = sum(1 for entry in entries if entry.status == "OK")
    total = len(entries)
    failed = total - ok
    return f"Summary: {ok}/{total} EFTSan entries OK, {failed} missing/failed."


def write_combined_json(entries: Sequence[EftsanEntry], json_path: Path) -> None:
    json_path.parent.mkdir(parents=True, exist_ok=True)
    payload = [
        {
            "benchmark": entry.benchmark,
            "file": entry.file,
            "line": entry.line,
            "baseline_error": entry.baseline_error,
            "error": entry.error,
            "absolute_difference": entry.absolute_difference,
            "status": entry.status,
        }
        for entry in entries
    ]
    with json_path.open("w", encoding="utf-8") as fp:
        json.dump(payload, fp, indent=2)
        fp.write("\n")


def write_report(entries: Sequence[EftsanEntry], report_path: Path) -> None:
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(format_table(entries) + "\n", encoding="utf-8")


def acceptable_status(status: str) -> bool:
    return status == "OK"


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Build/run EFTSanitizer PolyBench benchmarks and report JSON errors."
    )
    parser.add_argument(
        "precision",
        nargs="?",
        choices=("fp32", "fp64"),
        default="fp32",
        help="Benchmark precision mode. Defaults to fp32; fp64 selects only *_fp64 directories.",
    )
    parser.add_argument(
        "--root",
        default=str(SCRIPT_DIR / DEFAULT_ROOT),
        help="EFTSan PolyBench root.",
    )
    parser.add_argument(
        "--benchmarks-root",
        "--solvers-root",
        dest="benchmarks_root",
        default=None,
        help="Benchmark discovery root. Defaults to <root>.",
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
        help="Timeout in seconds for each make/run command.",
    )
    parser.add_argument(
        "--no-clean",
        action="store_true",
        help="Do not run make clean before each build.",
    )
    parser.add_argument(
        "--include-missing-run-sh",
        action="store_true",
        help="Include benchmark directories without run.sh as NO_RUN_SH rows.",
    )
    parser.add_argument(
        "--stop-on-failure",
        action="store_true",
        help="Stop after the first benchmark with a missing/failed result.",
    )
    parser.add_argument(
        "--llvm10",
        default=os.environ.get("LLVM10", str(DEFAULT_LLVM10)),
        help="Directory containing the LLVM tools used to build EFTSan binaries.",
    )
    parser.add_argument(
        "--llvm10-lib",
        default=os.environ.get("LLVM10_LIB", str(DEFAULT_LLVM10_LIB)),
        help="Directory containing MPFR/GMP libraries for the LLVM10 environment.",
    )
    parser.add_argument(
        "--opt-level",
        type=normalize_opt_level,
        default=DEFAULT_OPT_LEVEL,
        help="Optimization level for EFTSan builds: O0, O1, O2, or O3. Defaults to O0.",
    )
    parser.add_argument(
        "--opt-flags",
        default=None,
        help="Full OPT_LEVEL value passed to make. Overrides --opt-level.",
    )
    parser.add_argument(
        "--report",
        default=str(SCRIPT_DIR / DEFAULT_REPORT),
        help="Text report path.",
    )
    parser.add_argument(
        "--json",
        default=str(SCRIPT_DIR / DEFAULT_JSON),
        help="Combined JSON report path.",
    )
    args = parser.parse_args(argv)

    root = Path(args.root).resolve()
    benchmarks_root = Path(args.benchmarks_root).resolve() if args.benchmarks_root else root
    if not root.is_dir():
        print(f"error: EFTSan PolyBench root not found: {root}", file=sys.stderr)
        return 2
    if not benchmarks_root.is_dir():
        print(f"error: benchmark root not found: {benchmarks_root}", file=sys.stderr)
        return 2

    llvm10 = Path(args.llvm10).resolve()
    llvm10_lib = Path(args.llvm10_lib).resolve()

    benchmark_dirs = filter_benchmark_dirs(
        discover_benchmark_dirs(
            benchmarks_root,
            args.include_missing_run_sh,
            args.precision,
        ),
        args.benchmark,
    )
    if not benchmark_dirs:
        print("error: no PolyBench benchmarks matched", file=sys.stderr)
        return 2

    env = os.environ.copy()
    env["LLVM10"] = str(llvm10)
    env["LLVM10_LIB"] = str(llvm10_lib)
    opt_flags = args.opt_flags if args.opt_flags is not None else f"-{args.opt_level}"

    all_entries: List[EftsanEntry] = []
    for index, benchmark_dir in enumerate(benchmark_dirs, start=1):
        benchmark = str(benchmark_dir.relative_to(benchmarks_root))
        print(f"[{index}/{len(benchmark_dirs)}] {benchmark}", flush=True)
        try:
            entries = compare_benchmark(
                benchmark_dir=benchmark_dir,
                benchmarks_root=benchmarks_root,
                env=env,
                timeout=args.timeout,
                cleanup=not args.no_clean,
                llvm10=llvm10,
                llvm10_lib=llvm10_lib,
                opt_flags=opt_flags,
            )
        except subprocess.TimeoutExpired:
            entries = [
                EftsanEntry(
                    benchmark=benchmark,
                    file="-",
                    line=-1,
                    baseline_error=None,
                    error=None,
                    absolute_difference=None,
                    status="TIMEOUT",
                )
            ]
        all_entries.extend(entries)

        if args.stop_on_failure and any(not acceptable_status(entry.status) for entry in entries):
            break

    table = format_table(all_entries)
    print()
    print(table)

    report_path = Path(args.report).resolve()
    json_path = Path(args.json).resolve()
    write_report(all_entries, report_path)
    write_combined_json(all_entries, json_path)
    print(f"Wrote report: {report_path}")
    print(f"Wrote JSON: {json_path}")

    return 0 if all(acceptable_status(entry.status) for entry in all_entries) else 1


if __name__ == "__main__":
    raise SystemExit(main())
