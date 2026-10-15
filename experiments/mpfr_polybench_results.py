#!/usr/bin/env python3

import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


EXPERIMENTS_DIR = Path(__file__).resolve().parent
POLYBENCH_DIR = EXPERIMENTS_DIR / "benchmark" / "PolyBenchC-4.2.1"
RQ_RESULTS_DIR = EXPERIMENTS_DIR / "RQ_results"

BENCHMARKS = [
    {
        "source": "covariance.c",
        "benchmark": "datamining/covariance",
        "mpfr_dir": POLYBENCH_DIR / "datamining" / "covariance_mpfr",
        "executable": "covariance",
    },
    {
        "source": "fdtd-2d.c",
        "benchmark": "stencils/fdtd-2d",
        "mpfr_dir": POLYBENCH_DIR / "stencils" / "fdtd-2d_mpfr",
        "executable": "fdtd-2d",
    },
    {
        "source": "adi.c",
        "benchmark": "stencils/adi",
        "mpfr_dir": POLYBENCH_DIR / "stencils" / "adi_mpfr",
        "executable": "adi_mpfr",
    },
]

OPTS = {
    "O0": "-O0 -g",
    "O2": "-O2 -g",
}

METHODS = [
    "MPFR error",
    "Double shadow",
    "FPChecker error",
    "EFTSan error",
]


def format_value(value):
    if value is None:
        return "-"
    return "{:.6e}".format(abs(float(value)))


def run_command(command, cwd, env):
    completed = subprocess.run(
        command,
        cwd=str(cwd),
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            "command failed in {cwd}: {cmd}\n{output}".format(
                cwd=cwd,
                cmd=" ".join(command),
                output=completed.stdout,
            )
        )
    return completed.stdout


def mpfr_runtime_env():
    env = os.environ.copy()
    env["CCACHE_DISABLE"] = "1"
    env.setdefault("CCACHE_DIR", "/tmp/fpchecker-ccache")
    candidates = []

    for variable in ("CONDA_PREFIX", "MPFR_HOME"):
        root = env.get(variable)
        if root:
            candidates.append(Path(root) / "lib")

    candidates.extend(
        [
            Path("/usr/lib64"),
            Path("/lib64"),
        ]
    )

    existing = [str(path) for path in candidates if path.is_dir()]
    if existing:
        env["LD_LIBRARY_PATH"] = os.pathsep.join(
            existing + [env.get("LD_LIBRARY_PATH", "")]
        )
    return env


def executable_works(executable, cwd, env):
    if not executable.exists():
        return False
    completed = subprocess.run(
        [str(executable)],
        cwd=str(cwd),
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )
    return completed.returncode == 0 and parse_mpfr_abs_error(completed.stdout) is not None


def build_mpfr(bench, opt_name, env):
    mpfr_dir = bench["mpfr_dir"]
    if not mpfr_dir.is_dir():
        raise RuntimeError("missing MPFR directory: " + str(mpfr_dir))

    run_command(["make", "clean"], mpfr_dir, env)
    run_command(["make", "OP=" + OPTS[opt_name]], mpfr_dir, env)


def parse_mpfr_abs_error(output):
    match = re.search(r"Abs error\s*:\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)", output)
    if not match:
        return None
    return float(match.group(1))


def run_mpfr(bench, opt_name):
    env = mpfr_runtime_env()
    mpfr_dir = bench["mpfr_dir"]
    executable = mpfr_dir / bench["executable"]

    build_mpfr(bench, opt_name, env)

    if not executable_works(executable, mpfr_dir, env):
        raise RuntimeError(
            "MPFR executable did not run or did not print Abs error: " + str(executable)
        )

    output = run_command([str(executable)], mpfr_dir, env)
    value = parse_mpfr_abs_error(output)
    if value is None:
        raise RuntimeError("could not parse MPFR Abs error from " + str(executable))
    return value


def snapshot_executable(bench):
    executable = bench["mpfr_dir"] / bench["executable"]
    if not executable.exists():
        return None
    return {
        "path": executable,
        "data": executable.read_bytes(),
        "mode": executable.stat().st_mode,
    }


def restore_mpfr_dir(bench, snapshot):
    mpfr_dir = bench["mpfr_dir"]
    executable = mpfr_dir / bench["executable"]

    for pattern in ("*.o", "*.out", "*.err"):
        for path in mpfr_dir.glob(pattern):
            path.unlink()

    if snapshot is None:
        if executable.exists():
            executable.unlink()
        return

    snapshot["path"].write_bytes(snapshot["data"])
    snapshot["path"].chmod(snapshot["mode"] & 0o777)


def load_rq_results(opt_name):
    path = RQ_RESULTS_DIR / (
        "error_tools_fp32_{}_fpchecker_eftsan.json".format(opt_name)
    )
    if not path.is_file():
        raise RuntimeError("missing RQ result file: " + str(path))

    with path.open("r") as stream:
        return json.load(stream)


def rq_value(results, benchmark, tool, field):
    matches = [
        entry
        for entry in results
        if entry.get("benchmark") == benchmark and entry.get("tool") == tool
    ]
    if not matches:
        return None
    value = matches[0].get(field)
    return None if value == "-" else value


def collect_rows():
    rq_results = {opt_name: load_rq_results(opt_name) for opt_name in OPTS}
    rows = []

    for bench in BENCHMARKS:
        snapshot = snapshot_executable(bench)
        try:
            mpfr_errors = {
                opt_name: run_mpfr(bench, opt_name)
                for opt_name in OPTS
            }
        finally:
            restore_mpfr_dir(bench, snapshot)

        method_values = {
            "MPFR error": mpfr_errors,
            "Double shadow": {
                opt_name: rq_value(
                    rq_results[opt_name],
                    bench["benchmark"],
                    "fpchecker",
                    "baseline_error",
                )
                for opt_name in OPTS
            },
            "FPChecker error": {
                opt_name: rq_value(
                    rq_results[opt_name],
                    bench["benchmark"],
                    "fpchecker",
                    "error",
                )
                for opt_name in OPTS
            },
            "EFTSan error": {
                opt_name: rq_value(
                    rq_results[opt_name],
                    bench["benchmark"],
                    "eftsan",
                    "error",
                )
                for opt_name in OPTS
            },
        }

        for method in METHODS:
            rows.append(
                [
                    bench["source"],
                    method,
                    format_value(method_values[method]["O0"]),
                    format_value(method_values[method]["O2"]),
                ]
            )

    return rows


def print_table(rows):
    headers = ["Source", "Method", "O0 abs error", "O2 abs error"]
    widths = [len(header) for header in headers]
    for row in rows:
        widths = [max(width, len(value)) for width, value in zip(widths, row)]

    separator = "+-" + "-+-".join("-" * width for width in widths) + "-+"

    def format_row(values):
        return "| " + " | ".join(
            value.ljust(widths[index]) for index, value in enumerate(values)
        ) + " |"

    print(separator)
    print(format_row(headers))
    print(separator)
    previous_source = None
    for row in rows:
        if previous_source is not None and row[0] != previous_source:
            print(separator)
        print(format_row(row))
        previous_source = row[0]
    print(separator)


def main():
    if shutil.which("make") is None:
        raise RuntimeError("make was not found in PATH")
    if shutil.which("clang") is None:
        raise RuntimeError("clang was not found in PATH")

    rows = collect_rows()
    print_table(rows)


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        sys.exit(1)
