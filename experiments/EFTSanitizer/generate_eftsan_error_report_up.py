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
import re
import shutil
import shlex
import statistics
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_ROOT = Path("PolyBenchC-4.2.1-eftsan")
DEFAULT_REPORT = Path("eftsan_error_report.txt")
DEFAULT_JSON = Path("eftsan_errors_combined.json")
DEFAULT_EFTSAN_ENV = "llvm10"
DEFAULT_EFTSAN_ENVS = ("llvm10", "eftsan_env")
LEGACY_LLVM10 = Path("/g/g90/sharmin1/conda_env/llvm10/bin")
LEGACY_LLVM10_LIB = Path("/g/g90/sharmin1/conda_env/llvm10/lib")
DEFAULT_OPT_LEVEL = "O0"
EFTSAN_PASS_FLAGS = (
    "-eftsan",
    "-eftsan-enable-debugging",
    "-eftsan-detect-all-rounding-errors",
    "-eftsan-detect-all-exceptions",
)
FLOAT_RE = r"[-+]?(?:(?:[0-9]*\.[0-9]+|[0-9]+\.?)(?:[eE][-+]?[0-9]+)?|inf(?:inity)?|nan)"


@dataclass
class EftsanEntry:
    benchmark: str
    file: str
    line: int
    baseline_error: Optional[float]
    error: Optional[float]
    absolute_difference: Optional[float]
    status: str


@dataclass
class DirectSource:
    build_source: Path
    report_source: Path
    source_line_by_build_line: Dict[int, int]


LONG_DOUBLE_MATH_REPLACEMENTS = {
    "acosl": "acos",
    "asinl": "asin",
    "atan2l": "atan2",
    "atanl": "atan",
    "ceill": "ceil",
    "cosl": "cos",
    "coshl": "cosh",
    "expl": "exp",
    "exp2l": "exp2",
    "fabsl": "fabs",
    "floorl": "floor",
    "fmodl": "fmod",
    "hypotl": "hypot",
    "log10l": "log10",
    "log2l": "log2",
    "logl": "log",
    "powl": "pow",
    "sinl": "sin",
    "sinhl": "sinh",
    "sqrtl": "sqrt",
    "tanl": "tan",
    "tanhl": "tanh",
}


def commented_out(line: str) -> str:
    if not line.strip():
        return line
    indent = line[: len(line) - len(line.lstrip())]
    return f"{indent}/* EFTSan direct build: reference path omitted. */"


def find_print_array_ranges(lines: Sequence[str]) -> List[Tuple[int, int]]:
    ranges: List[Tuple[int, int]] = []
    index = 0
    while index < len(lines):
        if not re.match(r"\s*void\s+print_array\b", lines[index]):
            index += 1
            continue

        start = index
        brace_depth = 0
        saw_body = False
        while index < len(lines):
            brace_depth += lines[index].count("{")
            if "{" in lines[index]:
                saw_body = True
            brace_depth -= lines[index].count("}")
            if saw_body and brace_depth == 0:
                ranges.append((start, index))
                break
            index += 1
        index += 1
    return ranges


def reference_names_for_print_array(block_lines: Sequence[str]) -> List[str]:
    names = set()
    for line in block_lines:
        if "long double" not in line:
            continue
        long_double_tail = line.split("long double", 1)[1]
        for match in re.finditer(r"\bPOLYBENCH_\dD\(([A-Za-z_][A-Za-z0-9_]*)", long_double_tail):
            names.add(match.group(1))
        scalar_match = re.search(r"\blong\s+double\s+\*?\s*([A-Za-z_][A-Za-z0-9_]*)", line)
        if scalar_match:
            name = scalar_match.group(1)
            if (
                not name.startswith("POLYBENCH_")
                and (
                    name.endswith("_double")
                    or name.endswith("_long_double")
                    or name == "max_shadow"
                    or name.startswith("norm_error")
                )
            ):
                names.add(name)
    changed = True
    while changed:
        changed = False
        for line in block_lines:
            if not references_any_name(line, names):
                continue
            for match in re.finditer(
                r"\b(?:double|long\s+double)\s+(norm_error[A-Za-z0-9_]*)\s*=",
                line,
            ):
                if match.group(1) not in names:
                    names.add(match.group(1))
                    changed = True
    return sorted(names, key=len, reverse=True)


def references_any_name(line: str, names: Sequence[str]) -> bool:
    return any(re.search(rf"\b{re.escape(name)}\b", line) for name in names)


def omit_reference_print_array_code(text: str) -> str:
    lines = text.splitlines()
    output = lines[:]
    for start, end in find_print_array_ranges(lines):
        ref_names = reference_names_for_print_array(lines[start : end + 1])
        if not ref_names:
            continue

        body_started = False
        skip_block_depth = 0
        skip_next_statement = False
        for index in range(start, end + 1):
            line = lines[index]
            if not body_started:
                if "{" in line:
                    body_started = True
                continue

            if skip_block_depth:
                output[index] = commented_out(line)
                skip_block_depth += line.count("{") - line.count("}")
                if skip_block_depth <= 0:
                    skip_block_depth = 0
                continue

            if skip_next_statement:
                output[index] = commented_out(line)
                if line.strip():
                    skip_next_statement = False
                continue

            if not references_any_name(line, ref_names):
                continue

            output[index] = commented_out(line)
            stripped = line.strip()
            if re.match(r"^(if|for|while)\b", stripped):
                if "{" in line:
                    skip_block_depth = line.count("{") - line.count("}")
                else:
                    skip_next_statement = True
    return "\n".join(output) + ("\n" if text.endswith("\n") else "")


def conda_env_candidates(env_name: str) -> List[Path]:
    value = Path(env_name).expanduser()
    if value.is_absolute() or "/" in env_name:
        return [value]

    candidates: List[Path] = []
    conda_prefix = os.environ.get("CONDA_PREFIX")
    if conda_prefix:
        prefix = Path(conda_prefix)
        if prefix.name == env_name:
            candidates.append(prefix)
        candidates.append(prefix / "envs" / env_name)
        candidates.append(prefix.parent / env_name)
        candidates.append(prefix.parent / "envs" / env_name)

    home = Path.home()
    candidates.extend(
        [
            home / "conda_env" / env_name,
            home / "miniconda3" / "envs" / env_name,
            home / "anaconda3" / "envs" / env_name,
            Path("/opt/conda/envs") / env_name,
            Path("/opt/miniconda3/envs") / env_name,
            Path("/opt/anaconda3/envs") / env_name,
        ]
    )

    unique: List[Path] = []
    seen = set()
    for candidate in candidates:
        text = str(candidate)
        if text not in seen:
            unique.append(candidate)
            seen.add(text)
    return unique


def has_llvm10_tools(bin_dir: Path) -> bool:
    return all((bin_dir / tool).is_file() for tool in ("clang", "opt", "llvm-link", "llc"))


def resolve_gdb(env: Dict[str, str]) -> Path:
    gdb = shutil.which("gdb", path=env.get("PATH"))
    if gdb is None:
        raise RuntimeError(
            "gdb not found; EFTSan error extraction requires gdb. "
            "Install gdb in the container or use an image that includes it."
        )
    return Path(gdb).resolve()


def resolve_llvm10_paths(
    llvm10_arg: Optional[str],
    llvm10_lib_arg: Optional[str],
    env_name: str,
) -> Tuple[Path, Path]:
    llvm10_env = os.environ.get("LLVM10")
    llvm10_lib_env = os.environ.get("LLVM10_LIB")
    if llvm10_arg or llvm10_lib_arg or llvm10_env or llvm10_lib_env:
        llvm10_value = llvm10_arg or llvm10_env
        llvm10_lib_value = llvm10_lib_arg or llvm10_lib_env
        if llvm10_value:
            llvm10 = Path(llvm10_value).expanduser()
        elif llvm10_lib_value:
            llvm10 = Path(llvm10_lib_value).expanduser().parent / "bin"
        else:
            llvm10 = LEGACY_LLVM10
        llvm10_lib = Path(llvm10_lib_value).expanduser() if llvm10_lib_value else llvm10.parent / "lib"
        return llvm10.resolve(), llvm10_lib.resolve()

    env_names = []
    for candidate in [env_name, *DEFAULT_EFTSAN_ENVS]:
        if candidate and candidate not in env_names:
            env_names.append(candidate)

    searched: List[Path] = []
    for name in env_names:
        for prefix in conda_env_candidates(name):
            searched.append(prefix)
            bin_dir = prefix / "bin"
            lib_dir = prefix / "lib"
            if has_llvm10_tools(bin_dir) and lib_dir.is_dir():
                return bin_dir.resolve(), lib_dir.resolve()

    if has_llvm10_tools(LEGACY_LLVM10) and LEGACY_LLVM10_LIB.is_dir():
        return LEGACY_LLVM10.resolve(), LEGACY_LLVM10_LIB.resolve()

    searched_text = ", ".join(str(path) for path in searched)
    raise RuntimeError(
        "proper environment not found; running is not working for EFTSan. "
        f"Expected llvm10 or eftsan_env with clang/opt/llvm-link/llc. Searched {searched_text}"
    )


def run_cmd(
    cmd: Sequence[str],
    cwd: Path,
    env: Dict[str, str],
    timeout: int,
) -> subprocess.CompletedProcess[str]:
    try:
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
    except OSError as exc:
        return subprocess.CompletedProcess(cmd, 127, stdout=str(exc), stderr=None)


def run_cmd_with_input(
    cmd: Sequence[str],
    cwd: Path,
    env: Dict[str, str],
    timeout: int,
    input_file: Path,
    output_file: Path,
) -> subprocess.CompletedProcess[str]:
    with input_file.open("rb") as stdin, output_file.open("wb") as stdout:
        try:
            return subprocess.run(
                cmd,
                cwd=str(cwd),
                env=env,
                stdin=stdin,
                stdout=stdout,
                stderr=subprocess.PIPE,
                timeout=timeout,
                check=False,
            )
        except OSError as exc:
            return subprocess.CompletedProcess(
                cmd,
                127,
                stdout=None,
                stderr=str(exc).encode("utf-8", errors="replace"),
            )


def last_line(output: str) -> str:
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    for line in reversed(lines):
        if re.match(r"^#[0-9]+ +0x[0-9a-fA-F]+", line):
            continue
        if line.startswith("Stack dump:"):
            continue
        if re.match(r"^[0-9]+\. +Running pass ", line):
            continue
        return line[:120]
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
        return [
            path for path in benchmark_dirs if any(part.endswith("_fp64") for part in path.parts)
        ]
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


def benchmark_base_name(benchmark_dir: Path) -> str:
    name = benchmark_dir.name
    if name.endswith("_fp64"):
        return name[: -len("_fp64")]
    return name


def find_source_file(benchmark_dir: Path) -> Optional[Path]:
    preferred = benchmark_dir / f"{benchmark_base_name(benchmark_dir)}.c"
    if preferred.is_file():
        return preferred
    sources = sorted(
        path
        for path in benchmark_dir.glob("*.c")
        if not path.name.startswith(".") and path.name != "polybench.c"
    )
    if len(sources) == 1:
        return sources[0]
    return None


def remove_long_double_from_instrumented_source(text: str) -> str:
    """Keep LLVM10/EFTSan away from x86_fp80 while preserving double work."""
    text = re.sub(r"\blong\s+double\b", "double", text)
    for old, new in LONG_DOUBLE_MATH_REPLACEMENTS.items():
        text = re.sub(rf"\b{old}\b", new, text)
    text = re.sub(
        r"(?<![A-Za-z0-9_])((?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][-+]?[0-9]+)?|[0-9]+[eE][-+]?[0-9]+)[lL]\b",
        r"\1",
        text,
    )
    text = re.sub(r"%((?:[-+ #0]*)(?:\d+|\*)?(?:\.(?:\d+|\*))?)L([aAeEfFgG])", r"%\1\2", text)
    return text


def append_marker_stop_guard(lines: List[str], indent: str) -> None:
    marker_line = len(lines)
    lines.append(f"{indent}extern volatile int __eftsan_stop_line;")
    lines.append(f"{indent}if (__eftsan_stop_line == {marker_line}) return;")


def prepare_direct_build_source(source_file: Path, build_dir: Path) -> DirectSource:
    original_text = source_file.read_text(encoding="utf-8", errors="replace")
    text = omit_reference_print_array_code(original_text)
    text = remove_long_double_from_instrumented_source(text)
    has_existing_markers = bool(discover_norm_marker_lines(source_file))
    output_lines: List[str] = []
    source_line_by_build_line: Dict[int, int] = {}
    for source_number, original in enumerate(text.splitlines(), start=1):
        output_lines.append(original)
        assignment = parse_norm_assignment(original)
        if assignment is None:
            continue

        lhs, rhs = assignment
        indent = original[: len(original) - len(original.lstrip())]
        if rhs in {f"0+{lhs}", f"{lhs}+0"}:
            source_line_by_build_line[len(output_lines)] = source_number
            append_marker_stop_guard(output_lines, indent)
            continue

        if has_existing_markers:
            continue
        if not re.match(r"^(?:SQRT_FUN|sqrt|sqrtf|sqrtl)\(.*\)$", rhs):
            continue
        output_lines.append(f"{indent}{lhs} = 0 + {lhs};")
        source_line_by_build_line[len(output_lines)] = source_number
        append_marker_stop_guard(output_lines, indent)

    output_lines.append("")
    output_lines.append("volatile int __eftsan_stop_line = 0;")

    build_source = build_dir / source_file.name
    build_source.write_text("\n".join(output_lines) + "\n", encoding="utf-8")
    return DirectSource(
        build_source=build_source,
        report_source=source_file,
        source_line_by_build_line=source_line_by_build_line,
    )


def parse_norm_assignment(line: str) -> Optional[Tuple[str, str]]:
    line = re.sub(r"//.*", "", line)
    line = re.sub(r";.*", ";", line)
    if not line.strip().endswith(";"):
        return None
    statement = re.sub(r"\s+", "", line.strip()[:-1])
    lhs_rhs = statement.split("=")
    if len(lhs_rhs) != 2:
        return None
    lhs, rhs = lhs_rhs
    if lhs != "norm" and not re.match(r"^norm_[A-Za-z0-9_]+$", lhs):
        return None
    if "long_double" in lhs:
        return None
    return lhs, rhs


def discover_norm_marker_lines(source_file: Path) -> List[int]:
    lines: List[int] = []
    for number, original in enumerate(
        source_file.read_text(encoding="utf-8", errors="replace").splitlines(),
        start=1,
    ):
        assignment = parse_norm_assignment(original)
        if assignment is None:
            continue
        lhs, rhs = assignment
        if rhs in {f"0+{lhs}", f"{lhs}+0"}:
            lines.append(number)
    return lines


def discover_norm_sqrt_assignments(source_file: Path) -> List[Tuple[int, str]]:
    assignments: List[Tuple[int, str]] = []
    for number, original in enumerate(
        source_file.read_text(encoding="utf-8", errors="replace").splitlines(),
        start=1,
    ):
        assignment = parse_norm_assignment(original)
        if assignment is None:
            continue
        lhs, rhs = assignment
        if re.match(r"^(?:SQRT_FUN|sqrt|sqrtf|sqrtl)\(.*\)$", rhs):
            assignments.append((number, lhs))
    return assignments


def find_utilities_dir(benchmark_dir: Path) -> Optional[Path]:
    for parent in benchmark_dir.parents:
        candidate = parent / "utilities"
        if candidate.is_dir():
            return candidate
    return None


def data_type_define(benchmark_dir: Path) -> str:
    if any(part.endswith("_fp64") for part in benchmark_dir.parts):
        return "-DDATA_TYPE_IS_DOUBLE"
    return "-DDATA_TYPE_IS_FLOAT"


def write_math_func_file(output_dir: Path, source_file: Path) -> None:
    try:
        text = source_file.read_text(encoding="utf-8", errors="replace")
    except OSError:
        text = ""
    names: List[str] = []
    for patterns, functions in [
        (("SQRT_FUN", "sqrtf", "sqrt", "sqrtl"), ("sqrt", "sqrtf")),
        (("EXP_FUN", "expf", "exp", "expl"), ("exp", "expf")),
        (("POW_FUN", "powf", "pow", "powl"), ("pow", "powf")),
    ]:
        if any(pattern in text for pattern in patterns):
            names.extend(function for function in functions if function not in names)
    (output_dir / "mathFunc.txt").write_text(
        "".join(f"{name}\n" for name in names),
        encoding="utf-8",
    )


def write_forbid_file(output_dir: Path, source_file: Path) -> None:
    try:
        text = source_file.read_text(encoding="utf-8", errors="replace")
    except OSError:
        text = ""
    names = sorted(set(re.findall(r"\b([A-Za-z_][A-Za-z0-9_]*_long_double)\s*\(", text)))
    (output_dir / "forbid.txt").write_text(
        "".join(f"{name}\n" for name in names),
        encoding="utf-8",
    )


def validate_direct_eftsan_build_paths(
    llvm10: Path,
    llvm10_lib: Path,
    eft_pass: Path,
    runtime_dir: Path,
) -> Optional[str]:
    for tool in ("clang", "opt", "llvm-link", "llc"):
        path = llvm10 / tool
        if not path.is_file():
            return f"BUILD_FAILED: required LLVM10 tool not found: {path}"
    if not eft_pass.is_file():
        return f"BUILD_FAILED: EFTSanitizer pass not found: {eft_pass}"
    if not runtime_dir.is_dir():
        return f"BUILD_FAILED: EFTSanitizer runtime directory not found: {runtime_dir}"
    if not any((runtime_dir / name).is_file() for name in ("libeftsanitizer.so", "libeftsanitizer.a")):
        return f"BUILD_FAILED: EFTSanitizer runtime library not found in {runtime_dir}"
    if not llvm10_lib.is_dir():
        return f"BUILD_FAILED: LLVM10 library directory not found: {llvm10_lib}"
    return None


def build_with_llvm10_eftsan(
    benchmark_dir: Path,
    build_dir: Path,
    source_file: Path,
    env: Dict[str, str],
    timeout: int,
    cleanup: bool,
    llvm10: Path,
    llvm10_lib: Path,
    opt_flags: str,
) -> Optional[str]:
    utilities_dir = find_utilities_dir(benchmark_dir)
    if utilities_dir is None:
        return "NO_UTILITIES_DIR"

    eft_home = SCRIPT_DIR
    eft_pass = eft_home / "llvm_pass" / "build" / "EFTSan" / "libEFTSanitizer.so"
    runtime_dir = eft_home / "runtime" / "obj"
    status = validate_direct_eftsan_build_paths(llvm10, llvm10_lib, eft_pass, runtime_dir)
    if status is not None:
        return status

    build_dir.mkdir(parents=True, exist_ok=True)
    write_math_func_file(build_dir, source_file)
    write_forbid_file(build_dir, source_file)

    base = benchmark_base_name(benchmark_dir)
    benchmark_bc = build_dir / f"{base}.bc"
    polybench_bc = build_dir / "polybench.bc"
    linked_bc = build_dir / f"{base}.linked.bc"
    opt_bc = build_dir / f"{base}.opt.bc"
    obj_file = build_dir / f"{base}.o"
    target = build_dir / base
    opt_parts = shlex.split(opt_flags)

    common_flags = [
        "-g",
        *opt_parts,
        "-fno-inline",
        "-fno-vectorize",
        "-fno-slp-vectorize",
        "-ffp-contract=off",
        "-emit-llvm",
        f"-I{benchmark_dir}",
        f"-I{utilities_dir}",
        "-DMINI_DATASET",
        "-DPOLYBENCH_DUMP_ARRAYS",
        data_type_define(benchmark_dir),
    ]
    steps = [
        (
            "compile benchmark",
            [
            str(llvm10 / "clang"),
            *common_flags,
            "-c",
            str(source_file),
            "-o",
            str(benchmark_bc),
            ],
        ),
        (
            "compile polybench",
            [
            str(llvm10 / "clang"),
            *common_flags,
            "-c",
            str(utilities_dir / "polybench.c"),
            "-o",
            str(polybench_bc),
            ],
        ),
        (
            "link bitcode",
            [
            str(llvm10 / "llvm-link"),
            str(benchmark_bc),
            str(polybench_bc),
            "-o",
            str(linked_bc),
            ],
        ),
    ]
    for label, cmd in steps:
        result = run_cmd(cmd, build_dir, env, timeout)
        if result.returncode != 0:
            return f"BUILD_FAILED({label}): {last_line(result.stdout)}"

    opt_result = run_cmd_with_input(
        [str(llvm10 / "opt"), "-load", str(eft_pass), *EFTSAN_PASS_FLAGS],
        build_dir,
        env,
        timeout,
        linked_bc,
        opt_bc,
    )
    if opt_result.returncode != 0:
        stderr = opt_result.stderr.decode("utf-8", errors="replace")
        return f"BUILD_FAILED(opt eftsan): {last_line(stderr)}"

    final_steps = [
        (
            "compile object",
            [
            str(llvm10 / "llc"),
            str(opt_bc),
            "-filetype=obj",
            "-o",
            str(obj_file),
            ],
        ),
        (
            "link executable",
            [
            str(llvm10 / "clang"),
            "-g",
            *opt_parts,
            str(obj_file),
            "-o",
            str(target),
            f"-L{runtime_dir}",
            "-leftsanitizer",
            f"-L{llvm10_lib}",
            "-lmpfr",
            "-lgmp",
            "-lm",
            f"-Wl,-rpath,{runtime_dir}",
            f"-Wl,-rpath,{llvm10_lib}",
            ],
        ),
    ]
    for label, cmd in final_steps:
        result = run_cmd(cmd, build_dir, env, timeout)
        if result.returncode != 0:
            return f"BUILD_FAILED({label}): {last_line(result.stdout)}"
    if not target.is_file() or not os.access(target, os.X_OK):
        return "NO_EXECUTABLE"
    return None


def build_native_baseline(
    benchmark_dir: Path,
    build_dir: Path,
    source_file: Path,
    env: Dict[str, str],
    timeout: int,
    llvm10: Path,
    opt_flags: str,
) -> Tuple[Optional[str], Optional[Path]]:
    utilities_dir = find_utilities_dir(benchmark_dir)
    if utilities_dir is None:
        return "NO_UTILITIES_DIR", None

    build_dir.mkdir(parents=True, exist_ok=True)
    base = benchmark_base_name(benchmark_dir)
    benchmark_obj = build_dir / f"{base}.baseline.o"
    polybench_obj = build_dir / "polybench.baseline.o"
    target = build_dir / f"{base}.baseline"
    opt_parts = shlex.split(opt_flags)
    common_flags = [
        "-g",
        *opt_parts,
        "-fno-inline",
        "-fno-vectorize",
        "-fno-slp-vectorize",
        "-ffp-contract=off",
        f"-I{benchmark_dir}",
        f"-I{utilities_dir}",
        "-DMINI_DATASET",
        "-DPOLYBENCH_DUMP_ARRAYS",
        data_type_define(benchmark_dir),
    ]
    steps = [
        (
            "compile baseline benchmark",
            [
                str(llvm10 / "clang"),
                *common_flags,
                "-c",
                str(source_file),
                "-o",
                str(benchmark_obj),
            ],
        ),
        (
            "compile baseline polybench",
            [
                str(llvm10 / "clang"),
                *common_flags,
                "-c",
                str(utilities_dir / "polybench.c"),
                "-o",
                str(polybench_obj),
            ],
        ),
        (
            "link baseline executable",
            [
                str(llvm10 / "clang"),
                "-g",
                *opt_parts,
                str(benchmark_obj),
                str(polybench_obj),
                "-o",
                str(target),
                "-lm",
            ],
        ),
    ]
    for label, cmd in steps:
        result = run_cmd(cmd, build_dir, env, timeout)
        if result.returncode != 0:
            return f"BUILD_FAILED({label}): {last_line(result.stdout)}", None
    if not target.is_file() or not os.access(target, os.X_OK):
        return "NO_BASELINE_EXECUTABLE", None
    return None, target


def collect_native_baselines(
    benchmark_dir: Path,
    build_dir: Path,
    source_file: Path,
    env: Dict[str, str],
    timeout: int,
    llvm10: Path,
    opt_flags: str,
    report_lines: Optional[Sequence[int]] = None,
) -> Tuple[Dict[int, Optional[float]], Optional[str]]:
    build_status, target = build_native_baseline(
        benchmark_dir=benchmark_dir,
        build_dir=build_dir,
        source_file=source_file,
        env=env,
        timeout=timeout,
        llvm10=llvm10,
        opt_flags=opt_flags,
    )
    if build_status is not None or target is None:
        return {}, build_status

    lines = list(report_lines) if report_lines is not None else discover_norm_lines(source_file)
    run = run_cmd([str(target)], benchmark_dir, env, timeout)
    if run.returncode == 0:
        values = [
            parse_float(match.group(1))
            for match in re.finditer(
                rf"Norm error[^:]*:\s*({FLOAT_RE})",
                run.stdout,
                re.IGNORECASE,
            )
        ]
        if values:
            return {
                line: values[index] if index < len(values) else None
                for index, line in enumerate(lines)
            }, None

    baselines: Dict[int, Optional[float]] = {}
    for line in lines:
        norm_error_line, norm_error_var = find_norm_error(source_file, line)
        if norm_error_line is None or norm_error_var is None:
            baselines[line] = None
            continue
        gdb_args = [
            "gdb",
            "-q",
            "-batch",
            "-ex",
            "set pagination off",
            "-ex",
            "set breakpoint pending on",
            "-ex",
            f"break {source_file.resolve()}:{norm_error_line}",
            "-ex",
            "run",
            "-ex",
            f"print {norm_error_var}",
            str(target),
        ]
        result = run_cmd(gdb_args, benchmark_dir, env, timeout)
        if result.returncode != 0:
            return baselines, f"BASELINE_FAILED: {last_line(result.stdout)}"
        baseline_match = None
        for match in re.finditer(r"^\$[0-9]+ = (.*)$", result.stdout, re.MULTILINE):
            baseline_match = match
        baselines[line] = (
            parse_float(baseline_match.group(1)) if baseline_match else None
        )
    return baselines, None


def discover_norm_lines(source_file: Path) -> List[int]:
    return discover_norm_marker_lines(source_file)


def find_norm_error(source_file: Path, start_line: int) -> Tuple[Optional[int], Optional[str]]:
    pattern = re.compile(r"^\s*(?:double|long\s+double)\s+(norm_error[_A-Za-z0-9]*)\s*=")
    for number, line in enumerate(
        source_file.read_text(encoding="utf-8", errors="replace").splitlines(),
        start=1,
    ):
        if number <= start_line:
            continue
        match = pattern.match(line)
        if match:
            return number + 1, match.group(1)
    return None, None


def run_report_without_script(
    benchmark_dir: Path,
    env: Dict[str, str],
    timeout: int,
    llvm10_lib: Path,
    source_file: Optional[Path] = None,
    executable: Optional[Path] = None,
    log_dir: Optional[Path] = None,
    baseline_by_line: Optional[Dict[int, Optional[float]]] = None,
    report_source_file: Optional[Path] = None,
    source_line_by_build_line: Optional[Dict[int, int]] = None,
    output_json: Optional[Path] = None,
    stop_after_marker: bool = False,
) -> Optional[str]:
    source_file = source_file or find_source_file(benchmark_dir)
    if source_file is None:
        return "NO_SOURCE_FILE"
    report_source_file = report_source_file or source_file
    source_line_by_build_line = source_line_by_build_line or {}
    target = executable or (benchmark_dir / benchmark_base_name(benchmark_dir))
    if not target.is_file() or not os.access(target, os.X_OK):
        return f"RUN_FAILED: executable not found or not executable: {target}"

    lines = discover_norm_lines(source_file)
    if not lines:
        return f"RUN_FAILED: Could not find norm assignment lines in {source_file.name}."

    run_env = env.copy()
    run_env["LLVM10_LIB"] = str(llvm10_lib)
    old_ld_path = run_env.get("LD_LIBRARY_PATH", "")
    run_env["LD_LIBRARY_PATH"] = ":".join(
        item
        for item in [
            str(SCRIPT_DIR / "runtime" / "obj"),
            str(llvm10_lib),
            old_ld_path,
        ]
        if item
    )

    output_items: List[dict] = []
    failed_line = None
    failed_error: Optional[float] = None
    failed_baseline: Optional[float] = None
    failed_log: Optional[Path] = None
    failed_detail = ""
    for line in lines:
        log_file = (log_dir or benchmark_dir) / f"gdb_eft_line_{line}.log"
        log_file.parent.mkdir(parents=True, exist_ok=True)
        norm_error_line, norm_error_var = find_norm_error(source_file, line)
        gdb_args = [
            "gdb",
            "-q",
            "-batch",
            "-ex",
            "set pagination off",
            "-ex",
            "set breakpoint pending on",
            "-ex",
            f"break {source_file.resolve()}:{line}",
            "-ex",
            "run",
        ]
        if stop_after_marker:
            gdb_args.extend(["-ex", f"set variable __eftsan_stop_line = {line}"])
        gdb_args.extend(
            [
                "-ex",
                "break eftsan_sum",
                "-ex",
                "continue",
            ]
        )
        if (
            baseline_by_line is None
            and norm_error_line is not None
            and norm_error_var is not None
        ):
            gdb_args.extend(
                [
                    "-ex",
                    f"break {source_file.resolve()}:{norm_error_line}",
                    "-ex",
                    "disable 2",
                    "-ex",
                    "continue",
                    "-ex",
                    f"print {norm_error_var}",
                ]
        )
        gdb_args.append(str(target))
        result = run_cmd(gdb_args, log_dir or benchmark_dir, run_env, timeout)
        log_file.write_text(result.stdout, encoding="utf-8")

        err_match = re.search(r"c_err=([^,)]*)", result.stdout)
        error = parse_float(err_match.group(1)) if err_match else None
        baseline_match = None
        if baseline_by_line is None:
            for match in re.finditer(r"^\$[0-9]+ = (.*)$", result.stdout, re.MULTILINE):
                baseline_match = match
        if baseline_by_line is None:
            baseline = parse_float(baseline_match.group(1)) if baseline_match else None
        else:
            baseline = baseline_by_line.get(line)
        if error is None or baseline is None:
            failed_line = line
            failed_error = error
            failed_baseline = baseline
            failed_log = log_file
            failed_detail = last_line(result.stdout)
        report_line = source_line_by_build_line.get(line, line)
        output_items.append(
            {
                "file": str(report_source_file.resolve()),
                "line": report_line,
                "error": error,
                "baseline_error": baseline,
            }
        )

    json_file = output_json or (benchmark_dir / "eftsan_errors.json")
    with json_file.open("w", encoding="utf-8") as fp:
        json.dump(output_items, fp, indent=2)
        fp.write("\n")
    if failed_line is not None:
        report_line = source_line_by_build_line.get(failed_line, failed_line)
        detail = f", detail={failed_detail}" if failed_detail else ""
        return (
            f"RUN_FAILED: line={report_line}, error={fmt_float(failed_error)}, "
            f"baseline_error={fmt_float(failed_baseline)}, log={failed_log}{detail}"
        )
    return None


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
    direct_baselines: Optional[Dict[int, Optional[float]]] = None
    direct_build_dir: Optional[Path] = None
    direct_source: Optional[DirectSource] = None
    result_json = benchmark_dir / "eftsan_errors.json"

    work_root = SCRIPT_DIR / "work_error_report"
    work_root.mkdir(parents=True, exist_ok=True)
    direct_build_dir = Path(
        tempfile.mkdtemp(
            prefix=f"eftsan-{benchmark_base_name(benchmark_dir)}-",
            dir=str(work_root),
        )
    )
    build_dir = direct_build_dir
    executable = build_dir / benchmark_base_name(benchmark_dir)
    source_file = find_source_file(benchmark_dir)
    if source_file is None:
        build_status = "NO_SOURCE_FILE"
    else:
        direct_source = prepare_direct_build_source(source_file, build_dir)
        build_status = build_with_llvm10_eftsan(
            benchmark_dir=benchmark_dir,
            build_dir=build_dir,
            source_file=direct_source.build_source,
            env=env,
            timeout=timeout,
            cleanup=cleanup,
            llvm10=llvm10,
            llvm10_lib=llvm10_lib,
            opt_flags=opt_flags,
        )
    if build_status is None:
        baselines_by_report_line, build_status = collect_native_baselines(
            benchmark_dir=benchmark_dir,
            build_dir=build_dir / "baseline",
            source_file=direct_source.report_source,
            env=env,
            timeout=timeout,
            llvm10=llvm10,
            opt_flags=opt_flags,
            report_lines=[
                direct_source.source_line_by_build_line[line]
                for line in discover_norm_lines(direct_source.build_source)
            ],
        )
        direct_baselines = {
            build_line: baselines_by_report_line.get(report_line)
            for build_line, report_line in direct_source.source_line_by_build_line.items()
        }
        result_json = build_dir / "eftsan_errors.json"

    if build_status is not None:
        return [
            EftsanEntry(
                benchmark=benchmark,
                file="-",
                line=-1,
                baseline_error=None,
                error=None,
                absolute_difference=None,
                status=build_status,
            )
        ]

    run_env = env.copy()
    run_env["LLVM10_LIB"] = str(llvm10_lib)
    run_status = run_report_without_script(
        benchmark_dir=benchmark_dir,
        env=run_env,
        timeout=timeout,
        llvm10_lib=llvm10_lib,
        source_file=direct_source.build_source if direct_source else None,
        executable=executable,
        log_dir=direct_build_dir,
        baseline_by_line=direct_baselines,
        report_source_file=direct_source.report_source if direct_source else None,
        source_line_by_build_line=(
            direct_source.source_line_by_build_line if direct_source else None
        ),
        output_json=result_json,
        stop_after_marker=True,
    )
    if run_status is not None:
        return [
            EftsanEntry(
                benchmark=benchmark,
                file="-",
                line=-1,
                baseline_error=None,
                error=None,
                absolute_difference=None,
                status=run_status,
            )
        ]

    json_file = result_json
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
        help="Benchmark precision mode. fp64 selects *_fp64 directories and builds them with LLVM10/EFTSan.",
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
        help="Timeout in seconds for each build/run command.",
    )
    parser.add_argument(
        "--no-clean",
        action="store_true",
        help="Reserved for compatibility; direct EFTSan builds use a temporary work directory.",
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
        "--eftsan-env",
        default=os.environ.get("EFTSAN_ENV", DEFAULT_EFTSAN_ENV),
        help="Conda environment name or prefix for LLVM10/EFTSan. Defaults to llvm10, then eftsan_env.",
    )
    parser.add_argument(
        "--llvm10",
        default=None,
        help="Directory containing LLVM10 tools. Overrides --eftsan-env.",
    )
    parser.add_argument(
        "--llvm10-lib",
        default=None,
        help="Directory containing MPFR/GMP libraries. Overrides --eftsan-env.",
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
        help="Full compiler optimization flags. Overrides --opt-level.",
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

    try:
        llvm10, llvm10_lib = resolve_llvm10_paths(
            args.llvm10,
            args.llvm10_lib,
            args.eftsan_env,
        )
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

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
    old_path = env.get("PATH", "")
    env["PATH"] = ":".join(item for item in [str(llvm10), old_path] if item)
    old_ld_path = env.get("LD_LIBRARY_PATH", "")
    env["LD_LIBRARY_PATH"] = ":".join(
        item
        for item in [
            str(SCRIPT_DIR / "runtime" / "obj"),
            str(llvm10_lib),
            old_ld_path,
        ]
        if item
    )
    try:
        resolve_gdb(env)
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
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
