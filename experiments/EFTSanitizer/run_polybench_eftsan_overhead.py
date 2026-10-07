#!/usr/bin/env python3
"""
Measure EFTSanitizer runtime overhead on PolyBench.

For each benchmark, precision mode, and optimization level, this script builds:

  1. native: LLVM10 clang, no EFTSan pass
  2. eftsan: LLVM10 clang -> llvm-link -> opt -eftsan -> llc -> clang

Both executables are compiled with -DPOLYBENCH_TIME.  The reported overhead is:

  (EFTSanitizerTime - LLVM10ClangTime) / LLVM10ClangTime
"""

from __future__ import annotations

import argparse
import math
import os
import re
import resource
import shlex
import shutil
import signal
import statistics
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Optional, Sequence


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_ROOT = SCRIPT_DIR / "PolyBenchC-4.2.1-eftsan"
DEFAULT_WORK_DIR = SCRIPT_DIR / "work_eftsan_overhead"
DEFAULT_LLVM10 = Path("/g/g90/sharmin1/conda_env/llvm10/bin")
DEFAULT_LLVM10_LIB = Path("/g/g90/sharmin1/conda_env/llvm10/lib")
DEFAULT_EFT_PASS = SCRIPT_DIR / "llvm_pass" / "build" / "EFTSan" / "libEFTSanitizer.so"
DEFAULT_RUNTIME_DIR = SCRIPT_DIR / "runtime" / "obj"
DEFAULT_EXTRA_OPT_FLAGS = "-fno-vectorize -fno-slp-vectorize"
DEFAULT_EFTSAN_PASS_FLAGS = "-eftsan -eftsan-detect-all-rounding-errors"
DEFAULT_FORBID_FUNCTIONS = {
    # "cholesky": ("kernel_cholesky",),
    # "lu": ("kernel_lu",),
    # "ludcmp": ("kernel_ludcmp",),
    # "trisolv": ("kernel_trisolv",),
    "trmm": ("kernel_trmm",),
}
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
class EftsanOverheadResult:
    filename: str
    mode: str
    opt_level: str
    clang_time: Optional[float]
    eftsan_time: Optional[float]
    overhead: Optional[float]
    native_status: str
    eftsan_status: str


def normalize_opt_level(value: str) -> str:
    opt = value.strip()
    if opt.lower() == "both":
        return "both"
    if opt.startswith("-"):
        opt = opt[1:]
    if opt in {"0", "1", "2", "3"}:
        opt = f"O{opt}"
    if opt not in {"O0", "O1", "O2", "O3"}:
        raise argparse.ArgumentTypeError("expected O0, O1, O2, O3, or both")
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
        list(cmd),
        cwd=str(cwd),
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
        check=False,
    )


def run_shell(
    command: str,
    cwd: Path,
    env: dict[str, str],
    timeout: int,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["bash", "-c", command],
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
    if not lines:
        return ""
    return lines[-1][:160]


def append_log(log_path: Path, title: str, output: str) -> None:
    with log_path.open("a", encoding="utf-8", errors="replace") as fp:
        fp.write(f"$ {title}\n")
        if output:
            fp.write(output)
            if not output.endswith("\n"):
                fp.write("\n")


def write_math_func_file(bench_dir: Path, source_file: Path) -> None:
    try:
        text = source_file.read_text(encoding="utf-8", errors="replace")
    except OSError:
        text = ""

    names: list[str] = []
    for patterns, functions in [
        (("SQRT_FUN", "sqrtf", "sqrt"), ("sqrt", "sqrtf")),
        (("EXP_FUN", "expf", "exp"), ("exp", "expf")),
        (("POW_FUN", "powf", "pow"), ("pow", "powf")),
    ]:
        if any(pattern in text for pattern in patterns):
            names.extend(function for function in functions if function not in names)

    (bench_dir / "mathFunc.txt").write_text(
        "".join(f"{name}\n" for name in names),
        encoding="utf-8",
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
        if find_source_file(bench_dir) is not None:
            benchmarks.append(bench_dir)
    return sorted(benchmarks)


def filter_benchmarks(benchmarks: Iterable[Path], patterns: Sequence[str]) -> list[Path]:
    selected = []
    for bench in benchmarks:
        text = str(bench)
        if not patterns or any(pattern in text for pattern in patterns):
            selected.append(bench)
    return selected


def benchmark_base_name(bench_dir: Path) -> str:
    name = bench_dir.name
    if name.endswith("_fp64"):
        return name[: -len("_fp64")]
    return name


def default_forbid_functions(bench_dir: Path) -> list[str]:
    base = benchmark_base_name(bench_dir)
    functions = list(DEFAULT_FORBID_FUNCTIONS.get(base, ()))
    if functions:
        functions.append(f"{functions[0]}_double")
        functions.append(f"{functions[0]}_long_double")
    return functions


def write_forbid_file(bench_dir: Path) -> None:
    existing: list[str] = []
    forbid_path = bench_dir / "forbid.txt"
    if forbid_path.is_file():
        existing = [
            line.strip()
            for line in forbid_path.read_text(encoding="utf-8", errors="replace").splitlines()
            if line.strip()
        ]

    names = [*existing]
    for name in default_forbid_functions(bench_dir):
        if name not in names:
            names.append(name)

    if names:
        forbid_path.write_text("".join(f"{name}\n" for name in names), encoding="utf-8")


def find_source_file(bench_dir: Path) -> Optional[Path]:
    preferred = bench_dir / f"{benchmark_base_name(bench_dir)}.c"
    if preferred.is_file():
        return preferred
    sources = sorted(
        path
        for path in bench_dir.glob("*.c")
        if not path.name.startswith(".") and path.name != "polybench.c"
    )
    if len(sources) == 1:
        return sources[0]
    return None


def copy_ignore(_: str, names: list[str]) -> set[str]:
    suffixes = (
        ".bc",
        ".core",
        ".err",
        ".json",
        ".linked.bc",
        ".log",
        ".o",
        ".opt.bc",
        ".out",
    )
    ignored = {
        ".git",
        ".fpc_log.txt",
        ".fpc_logs",
        "__pycache__",
        "core",
        "error.log",
        "eftsan_errors.json",
        "functions.txt",
        "mathFunc.txt",
    }
    ignored.update(name for name in names if name.endswith(suffixes))
    ignored.update(name for name in names if ".core" in name)
    return ignored.intersection(names)


def prepare_work_benchmark(
    source_root: Path,
    source_bench_dir: Path,
    run_work_dir: Path,
    mode: str,
    opt_level: str,
    variant: str,
) -> Path:
    rel_bench = source_bench_dir.relative_to(source_root)
    work_root = run_work_dir / mode / opt_level / variant / source_root.name
    work_bench_dir = work_root / rel_bench

    if work_bench_dir.exists():
        shutil.rmtree(work_bench_dir)
    work_bench_dir.parent.mkdir(parents=True, exist_ok=True)

    utilities_dir = work_root / "utilities"
    if not utilities_dir.exists():
        shutil.copytree(source_root / "utilities", utilities_dir, ignore=copy_ignore)

    shutil.copytree(source_bench_dir, work_bench_dir, ignore=copy_ignore)
    # For now, keep all benchmark functions eligible for EFTSan instrumentation.
    # if variant == "eftsan":
    #     write_forbid_file(work_bench_dir)
    return work_bench_dir


def opt_flags(opt_level: str, extra_flags: str) -> list[str]:
    flags = [f"-{opt_level}"]
    if extra_flags.strip():
        flags.extend(shlex.split(extra_flags))
    return flags


def data_type_flags(mode: str) -> list[str]:
    if mode == "fp32":
        return ["-DDATA_TYPE_IS_FLOAT"]
    return ["-DDATA_TYPE_IS_DOUBLE"]


def common_compile_flags(
    bench_dir: Path,
    utilities_dir: Path,
    mode: str,
    opt_level: str,
    extra_flags: str,
    dataset: str,
) -> list[str]:
    return [
        "-g",
        *opt_flags(opt_level, extra_flags),
        "-I.",
        f"-I{utilities_dir}",
        f"-D{dataset}",
        "-DPOLYBENCH_TIME",
        *data_type_flags(mode),
    ]


def make_environment(args: argparse.Namespace) -> dict[str, str]:
    env = os.environ.copy()
    env["CCACHE_DISABLE"] = "1"
    env["LLVM10"] = str(args.llvm10)
    env["LLVM10_LIB"] = str(args.llvm10_lib)
    runtime_path = str(args.runtime_dir)
    llvm_lib_path = str(args.llvm10_lib)
    old_ld_path = env.get("LD_LIBRARY_PATH", "")
    env["LD_LIBRARY_PATH"] = ":".join(
        item for item in [runtime_path, llvm_lib_path, old_ld_path] if item
    )
    old_path = env.get("PATH", "")
    env["PATH"] = ":".join(item for item in [str(args.llvm10), old_path] if item)
    return env


def build_native(
    bench_dir: Path,
    mode: str,
    opt_level: str,
    args: argparse.Namespace,
    env: dict[str, str],
) -> BuildResult:
    log_path = bench_dir / "native_build.log"
    log_path.write_text("", encoding="utf-8")

    source_file = find_source_file(bench_dir)
    if source_file is None:
        return BuildResult(False, "NO_SOURCE_FILE", "")

    utilities_dir = bench_dir
    for parent in bench_dir.parents:
        candidate = parent / "utilities"
        if candidate.is_dir():
            utilities_dir = candidate
            break

    base = benchmark_base_name(bench_dir)
    benchmark_bc = bench_dir / f"{base}.bc"
    polybench_bc = bench_dir / "polybench.bc"
    linked_bc = bench_dir / f"{base}.linked.bc"
    obj_file = bench_dir / f"{base}.o"
    target = bench_dir / base

    flags = common_compile_flags(
        bench_dir,
        utilities_dir,
        mode,
        opt_level,
        args.extra_opt_flags,
        args.dataset,
    )
    steps = [
        [
            str(args.llvm10 / "clang"),
            *flags,
            "-emit-llvm",
            "-c",
            str(source_file),
            "-o",
            str(benchmark_bc),
        ],
        [
            str(args.llvm10 / "clang"),
            *flags,
            "-emit-llvm",
            "-c",
            str(utilities_dir / "polybench.c"),
            "-o",
            str(polybench_bc),
        ],
        [
            str(args.llvm10 / "llvm-link"),
            str(benchmark_bc),
            str(polybench_bc),
            "-o",
            str(linked_bc),
        ],
        [
            str(args.llvm10 / "llc"),
            str(linked_bc),
            "-filetype=obj",
            "-o",
            str(obj_file),
        ],
        [
            str(args.llvm10 / "clang"),
            "-g",
            f"-{opt_level}",
            str(obj_file),
            "-o",
            str(target),
            "-lm",
        ],
    ]

    combined_output = []
    for cmd in steps:
        title = shlex.join(cmd)
        try:
            build = run_cmd(cmd, bench_dir, env, args.timeout)
        except subprocess.TimeoutExpired as exc:
            output = str(exc)
            append_log(log_path, title, output)
            return BuildResult(False, "BUILD_TIMEOUT", "\n".join([*combined_output, output]))

        append_log(log_path, title, build.stdout)
        combined_output.append(build.stdout)
        if build.returncode != 0:
            return BuildResult(
                False,
                f"BUILD_FAILED: {last_line(build.stdout)}",
                "\n".join(combined_output),
            )

    if not target.is_file() or not os.access(target, os.X_OK):
        return BuildResult(False, "NO_EXECUTABLE", "\n".join(combined_output))
    return BuildResult(True, "OK", "\n".join(combined_output))


def build_eftsan(
    bench_dir: Path,
    mode: str,
    opt_level: str,
    args: argparse.Namespace,
    env: dict[str, str],
) -> BuildResult:
    log_path = bench_dir / "eftsan_build.log"
    log_path.write_text("", encoding="utf-8")

    source_file = find_source_file(bench_dir)
    if source_file is None:
        return BuildResult(False, "NO_SOURCE_FILE", "")
    write_math_func_file(bench_dir, source_file)

    utilities_dir = bench_dir
    for parent in bench_dir.parents:
        candidate = parent / "utilities"
        if candidate.is_dir():
            utilities_dir = candidate
            break

    base = benchmark_base_name(bench_dir)
    benchmark_bc = bench_dir / f"{base}.bc"
    polybench_bc = bench_dir / "polybench.bc"
    linked_bc = bench_dir / f"{base}.linked.bc"
    opt_bc = bench_dir / f"{base}.opt.bc"
    obj_file = bench_dir / f"{base}.o"
    target = bench_dir / base

    flags = common_compile_flags(
        bench_dir,
        utilities_dir,
        mode,
        opt_level,
        args.extra_opt_flags,
        args.dataset,
    )
    steps = [
        [
            str(args.llvm10 / "clang"),
            *flags,
            "-emit-llvm",
            "-c",
            str(source_file),
            "-o",
            str(benchmark_bc),
        ],
        [
            str(args.llvm10 / "clang"),
            *flags,
            "-emit-llvm",
            "-c",
            str(utilities_dir / "polybench.c"),
            "-o",
            str(polybench_bc),
        ],
        [
            str(args.llvm10 / "llvm-link"),
            str(benchmark_bc),
            str(polybench_bc),
            "-o",
            str(linked_bc),
        ],
        [
            str(args.llvm10 / "opt"),
            "-load",
            str(args.eft_pass),
            *shlex.split(DEFAULT_EFTSAN_PASS_FLAGS),
        ],
        [
            str(args.llvm10 / "llc"),
            str(opt_bc),
            "-filetype=obj",
            "-o",
            str(obj_file),
        ],
        [
            str(args.llvm10 / "clang"),
            "-g",
            f"-{opt_level}",
            str(obj_file),
            "-o",
            str(target),
            f"-L{args.runtime_dir}",
            "-leftsanitizer",
            f"-L{args.llvm10_lib}",
            "-lmpfr",
            "-lgmp",
            "-lm",
            f"-Wl,-rpath,{args.runtime_dir}",
            f"-Wl,-rpath,{args.llvm10_lib}",
        ],
    ]

    combined_output = []
    for index, cmd in enumerate(steps):
        title = shlex.join(cmd)
        try:
            if index == 3:
                command = f"{shlex.join(cmd)} < {shlex.quote(str(linked_bc))} > {shlex.quote(str(opt_bc))}"
                result = run_shell(command, bench_dir, env, args.timeout)
                title = command
            else:
                result = run_cmd(cmd, bench_dir, env, args.timeout)
        except subprocess.TimeoutExpired as exc:
            output = str(exc)
            append_log(log_path, title, output)
            return BuildResult(False, "BUILD_TIMEOUT", "\n".join([*combined_output, output]))

        append_log(log_path, title, result.stdout)
        combined_output.append(result.stdout)
        if result.returncode != 0:
            return BuildResult(
                False,
                f"BUILD_FAILED: {last_line(result.stdout)}",
                "\n".join(combined_output),
            )

    if not target.is_file() or not os.access(target, os.X_OK):
        return BuildResult(False, "NO_EXECUTABLE", "\n".join(combined_output))
    return BuildResult(True, "OK", "\n".join(combined_output))


def parse_polybench_time(output: str) -> Optional[float]:
    values = []
    for line in output.splitlines():
        match = TIME_RE.match(line)
        if match:
            values.append(float(match.group("value")))
    if not values:
        return None
    return values[-1]


def run_executable(
    bench_dir: Path,
    target_name: str,
    log_name: str,
    env: dict[str, str],
    timeout: int,
    stack_kb: int,
) -> RunResult:
    log_path = bench_dir / log_name
    log_path.write_text("", encoding="utf-8")
    executable = bench_dir / target_name
    if not executable.is_file():
        return RunResult("NO_EXECUTABLE", None, "")

    command = f"./{executable.name}"
    stack_bytes = stack_kb * 1024

    def set_stack_limit() -> None:
        soft, hard = resource.getrlimit(resource.RLIMIT_STACK)
        if hard == resource.RLIM_INFINITY or stack_bytes <= hard:
            resource.setrlimit(resource.RLIMIT_STACK, (stack_bytes, hard))
        elif stack_bytes <= soft:
            resource.setrlimit(resource.RLIMIT_STACK, (stack_bytes, hard))

    try:
        run = subprocess.run(
            [command],
            cwd=str(bench_dir),
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=timeout,
            check=False,
            preexec_fn=set_stack_limit,
        )
    except subprocess.TimeoutExpired as exc:
        output = str(exc)
        append_log(log_path, f"ulimit -s {stack_kb}; {command}", output)
        return RunResult("RUN_TIMEOUT", None, output)

    append_log(log_path, f"ulimit -s {stack_kb}; {command}", run.stdout)
    if run.returncode < 0:
        signum = -run.returncode
        try:
            sig_name = signal.Signals(signum).name
        except ValueError:
            sig_name = f"signal {signum}"
        append_log(log_path, "status", f"{sig_name}\n")
        return RunResult(f"RUN_{sig_name}", None, run.stdout)
    if run.returncode != 0:
        return RunResult(f"RUN_FAILED: {last_line(run.stdout)}", None, run.stdout)

    time_value = parse_polybench_time(run.stdout)
    if time_value is None or not math.isfinite(time_value):
        return RunResult("NO_TIME_OUTPUT", None, run.stdout)
    return RunResult("OK", time_value, run.stdout)


def build_and_run_native(
    bench_dir: Path,
    mode: str,
    opt_level: str,
    args: argparse.Namespace,
    env: dict[str, str],
) -> RunResult:
    build = build_native(bench_dir, mode, opt_level, args, env)
    if not build.ok:
        return RunResult(build.status, None, build.output)
    return run_executable(
        bench_dir,
        benchmark_base_name(bench_dir),
        "native_run.log",
        env,
        args.timeout,
        args.stack_kb,
    )


def build_and_run_eftsan(
    bench_dir: Path,
    mode: str,
    opt_level: str,
    args: argparse.Namespace,
    env: dict[str, str],
) -> RunResult:
    build = build_eftsan(bench_dir, mode, opt_level, args, env)
    if not build.ok:
        return RunResult(build.status, None, build.output)
    return run_executable(
        bench_dir,
        benchmark_base_name(bench_dir),
        "eftsan_run.log",
        env,
        args.timeout,
        args.stack_kb,
    )


def measure_one(
    source_bench_dir: Path,
    source_root: Path,
    mode: str,
    opt_level: str,
    args: argparse.Namespace,
    env: dict[str, str],
) -> EftsanOverheadResult:
    filename = str(source_bench_dir.relative_to(source_root))

    native_dir = prepare_work_benchmark(
        source_root, source_bench_dir, args.run_work_dir, mode, opt_level, "native"
    )
    eftsan_dir = prepare_work_benchmark(
        source_root, source_bench_dir, args.run_work_dir, mode, opt_level, "eftsan"
    )

    native_run = build_and_run_native(native_dir, mode, opt_level, args, env)
    eftsan_run = build_and_run_eftsan(eftsan_dir, mode, opt_level, args, env)

    overhead = None
    if native_run.time is not None and eftsan_run.time is not None and native_run.time > 0.0:
        overhead = (eftsan_run.time - native_run.time) / native_run.time

    return EftsanOverheadResult(
        filename=filename,
        mode=mode,
        opt_level=opt_level,
        clang_time=native_run.time,
        eftsan_time=eftsan_run.time,
        overhead=overhead,
        native_status=native_run.status,
        eftsan_status=eftsan_run.status,
    )


def fmt_float(value: Optional[float]) -> str:
    if value is None:
        return "-"
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "inf" if value > 0 else "-inf"
    return f"{value:.6e}"


def valid_results(results: Sequence[EftsanOverheadResult]) -> list[EftsanOverheadResult]:
    return [
        item
        for item in results
        if item.native_status == "OK"
        and item.eftsan_status == "OK"
        and item.clang_time is not None
        and item.eftsan_time is not None
        and item.overhead is not None
        and math.isfinite(item.clang_time)
        and math.isfinite(item.eftsan_time)
        and math.isfinite(item.overhead)
    ]


def summary_row(
    label: str,
    results: Sequence[EftsanOverheadResult],
    reducer: str,
) -> list[str]:
    valid = valid_results(results)
    if not valid:
        return [label, "-", "-", "-", "-", "-", f"{label.upper()} (0 passed)", "-"]

    if reducer == "mean":
        clang_time = statistics.mean(item.clang_time for item in valid if item.clang_time is not None)
        eftsan_time = statistics.mean(item.eftsan_time for item in valid if item.eftsan_time is not None)
        overhead = statistics.mean(item.overhead for item in valid if item.overhead is not None)
    else:
        clang_time = statistics.median(item.clang_time for item in valid if item.clang_time is not None)
        eftsan_time = statistics.median(item.eftsan_time for item in valid if item.eftsan_time is not None)
        overhead = statistics.median(item.overhead for item in valid if item.overhead is not None)

    return [
        label,
        "-",
        "-",
        fmt_float(clang_time),
        fmt_float(eftsan_time),
        fmt_float(overhead),
        f"{label.upper()} ({len(valid)} passed)",
        "-",
    ]


def print_results(results: Sequence[EftsanOverheadResult]) -> None:
    headers = [
        "Filename",
        "Mode",
        "Opt",
        "LLVM10ClangTime(s)",
        "EFTSanitizerTime(s)",
        "Overhead",
        "NativeStatus",
        "EFTSanitizerStatus",
    ]
    rows = [
        [
            item.filename,
            item.mode,
            item.opt_level,
            fmt_float(item.clang_time),
            fmt_float(item.eftsan_time),
            fmt_float(item.overhead),
            item.native_status,
            item.eftsan_status,
        ]
        for item in results
    ]
    rows.append(summary_row("Average", results, "mean"))

    widths = [len(header) for header in headers]
    for row in rows:
        for idx, value in enumerate(row):
            widths[idx] = max(widths[idx], len(value))

    print("  ".join(header.ljust(widths[idx]) for idx, header in enumerate(headers)))
    print("  ".join("-" * width for width in widths))
    for row in rows[:-1]:
        print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(row)))
    print("  ".join("-" * width for width in widths))
    print("  ".join(value.ljust(widths[idx]) for idx, value in enumerate(rows[-1])))


def parse_args(argv: Optional[Sequence[str]]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Print EFTSanitizer runtime overhead for PolyBench."
    )
    parser.add_argument(
        "precision",
        nargs="?",
        choices=("fp32", "fp64", "both"),
        default=None,
        help="Precision mode. Defaults to fp32.",
    )
    parser.add_argument(
        "--mode",
        choices=("fp32", "fp64", "both"),
        default=None,
        help="Precision mode. Overrides the positional precision argument.",
    )
    parser.add_argument(
        "--root",
        default=str(DEFAULT_ROOT),
        help="EFTSan PolyBench root. Defaults to experiments/EFTSanitizer/PolyBenchC-4.2.1-eftsan.",
    )
    parser.add_argument(
        "--work-dir",
        type=Path,
        default=DEFAULT_WORK_DIR,
        help="Separate build workspace. Defaults to experiments/EFTSanitizer/work_eftsan_overhead.",
    )
    parser.add_argument(
        "--benchmark",
        action="append",
        default=[],
        help="Substring filter for benchmark paths. Can be passed more than once.",
    )
    parser.add_argument(
        "--opt-level",
        type=normalize_opt_level,
        default="O2",
        help="Optimization level: O0, O1, O2, O3, or both. Defaults to O2.",
    )
    parser.add_argument(
        "--extra-opt-flags",
        default=DEFAULT_EXTRA_OPT_FLAGS,
        help="Extra flags appended to -O*. Defaults to no-inline and no-vectorize flags.",
    )
    parser.add_argument(
        "--dataset",
        default="MINI_DATASET",
        help="PolyBench dataset macro without -D. Defaults to MINI_DATASET.",
    )
    parser.add_argument(
        "--llvm10",
        type=Path,
        default=Path(os.environ.get("LLVM10", str(DEFAULT_LLVM10))),
        help="Directory containing clang/opt/llvm-link/llc for the EFTSan build.",
    )
    parser.add_argument(
        "--llvm10-lib",
        type=Path,
        default=Path(os.environ.get("LLVM10_LIB", str(DEFAULT_LLVM10_LIB))),
        help="Directory containing LLVM10/MPFR/GMP libraries.",
    )
    parser.add_argument(
        "--eft-pass",
        type=Path,
        default=DEFAULT_EFT_PASS,
        help="EFTSanitizer LLVM pass shared library.",
    )
    parser.add_argument(
        "--runtime-dir",
        type=Path,
        default=DEFAULT_RUNTIME_DIR,
        help="Directory containing libeftsanitizer runtime objects/libraries.",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help="Timeout in seconds for each build step or run. Defaults to 300.",
    )
    parser.add_argument(
        "--stack-kb",
        type=int,
        default=8192,
        help="Stack size in KB used when running benchmarks. Defaults to 8192.",
    )
    return parser.parse_args(argv)


def check_required_paths(args: argparse.Namespace) -> bool:
    ok = True
    required_tools = ["clang", "opt", "llvm-link", "llc"]
    for tool in required_tools:
        path = args.llvm10 / tool
        if not path.is_file():
            print(f"error: required LLVM10 tool not found: {path}", file=sys.stderr)
            ok = False
    for label, path in [
        ("EFTSanitizer pass", args.eft_pass),
        ("EFTSanitizer runtime", args.runtime_dir),
        ("LLVM10 library directory", args.llvm10_lib),
    ]:
        if not path.exists():
            print(f"error: {label} not found: {path}", file=sys.stderr)
            ok = False
    return ok


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    args.root = Path(args.root).resolve()
    args.work_dir = args.work_dir.resolve()
    args.llvm10 = args.llvm10.resolve()
    args.llvm10_lib = args.llvm10_lib.resolve()
    args.eft_pass = args.eft_pass.resolve()
    args.runtime_dir = args.runtime_dir.resolve()

    mode_arg = args.mode or args.precision or "fp32"
    if not args.root.is_dir():
        print(f"error: EFTSan PolyBench root not found: {args.root}", file=sys.stderr)
        return 2
    if not check_required_paths(args):
        return 2

    args.run_work_dir = args.work_dir / f"run-{os.getpid()}"
    env = make_environment(args)
    results: list[EftsanOverheadResult] = []

    for mode in selected_modes(mode_arg):
        benchmarks = filter_benchmarks(discover_benchmarks(args.root, mode), args.benchmark)
        if not benchmarks:
            print(f"warning: no {mode} benchmarks matched", file=sys.stderr)
            continue
        for opt_level in selected_opts(args.opt_level):
            for index, bench_dir in enumerate(benchmarks, start=1):
                rel = bench_dir.relative_to(args.root)
                print(f"[{mode} {opt_level} {index}/{len(benchmarks)}] {rel}", flush=True)
                results.append(measure_one(bench_dir, args.root, mode, opt_level, args, env))

    if not results:
        print("error: no benchmark runs were attempted", file=sys.stderr)
        return 1

    print()
    print_results(results)
    print(f"\nWork directory: {args.run_work_dir}")
    return 0 if all(item.native_status == "OK" and item.eftsan_status == "OK" for item in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
