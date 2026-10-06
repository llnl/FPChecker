#!/usr/bin/env python3

import json
import math
import os
import pathlib
import shutil
import subprocess
import sys


APP_NAME = "miniAMR"

SOURCE_PATHS = [
    pathlib.Path("openmp") / "init.c",
    pathlib.Path("openmp") / "driver.c",
    pathlib.Path("openmp") / "stencil.c",
    pathlib.Path("openmp") / "check_sum.c",
    pathlib.Path("openmp") / "comm.c",
]

RUN_TIMEOUT_SECONDS = int(os.environ.get("MINIAMR_CANCELLATION_TIMEOUT", "120"))
RUN_COMMAND = [
    "mpirun",
    "-np",
    "1",
    "./miniAMR.x",
    "--npx",
    "1",
    "--npy",
    "1",
    "--npz",
    "1",
    "--nx",
    "8",
    "--ny",
    "8",
    "--nz",
    "8",
    "--num_tsteps",
    "20",
    "--num_refine",
    "2",
    "--max_blocks",
    "1000",
]
INSTALL_BIN_CANDIDATES = [
    pathlib.Path(os.environ["FPCHECKER_INSTALL_BIN"])
    if "FPCHECKER_INSTALL_BIN" in os.environ
    else None,
    pathlib.Path(os.environ["FPCHECKER_HOME"]) / "bin"
    if "FPCHECKER_HOME" in os.environ
    else None,
    pathlib.Path("/g/g90/sharmin1/tutorial/install/bin"),
    pathlib.Path(__file__).resolve().parents[3] / "install" / "bin",
    pathlib.Path(__file__).resolve().parents[3] / "build-current" / "install" / "bin",
    pathlib.Path(__file__).resolve().parents[3] / "build" / "install" / "bin",
]


def configure_environment(app_dir):
    env = os.environ.copy()

    path_entries = [
        str(path)
        for path in INSTALL_BIN_CANDIDATES
        if path is not None and path.exists()
    ]
    if path_entries:
        env["PATH"] = os.pathsep.join(path_entries + [env.get("PATH", "")])

    if shutil.which("mpicc-fpchecker", path=env.get("PATH", "")) is None:
        raise RuntimeError(
            "mpicc-fpchecker was not found. Add it to PATH or set "
            "FPCHECKER_INSTALL_BIN before running this script from " + str(app_dir)
        )

    if shutil.which("mpicc", path=env.get("PATH", "")) is None:
        raise RuntimeError("mpicc was not found in PATH")

    env["CC"] = "FPC_INSTRUMENT_ERR_TRACKING_FP64=1 mpicc-fpchecker"
    env["FPC_INSTRUMENT_ERR_TRACKING_FP64"] = "1"

    env["OMP_NUM_THREADS"] = "1"
    env["OMP_DYNAMIC"] = "FALSE"
    env["OMP_MAX_ACTIVE_LEVELS"] = "1"

    try:
        mpi_show = subprocess.check_output(
            ["mpicc", "-show"],
            env=env,
            text=True,
            stderr=subprocess.STDOUT,
        )
    except (OSError, subprocess.CalledProcessError):
        mpi_show = ""

    library_paths = []
    for token in mpi_show.split():
        if token.startswith("-L") and len(token) > 2:
            library_paths.append(token[2:])
        elif token == "-Wl,-rpath":
            continue
        elif token.startswith("-Wl,") and pathlib.Path(token[4:]).is_dir():
            library_paths.append(token[4:])

    if "CONDA_PREFIX" in env:
        conda_lib = pathlib.Path(env["CONDA_PREFIX"]) / "lib"
        if conda_lib.exists():
            library_paths.append(str(conda_lib))

    if library_paths:
        env["LD_LIBRARY_PATH"] = os.pathsep.join(
            library_paths + [env.get("LD_LIBRARY_PATH", "")]
        )

    return env


def run_command(cmd, cwd, env, timeout=None):
    print("+ " + " ".join(cmd), flush=True)

    try:
        completed = subprocess.run(
            cmd,
            cwd=cwd,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as exc:
        if exc.stdout:
            print(exc.stdout)
        raise RuntimeError(
            "command timed out after "
            + str(timeout)
            + " seconds: "
            + " ".join(cmd)
        )

    if completed.stdout:
        print(completed.stdout)

    if completed.returncode != 0:
        raise RuntimeError("command failed: " + " ".join(cmd))

    return completed.stdout


def latest_rounding_json(app_dir):
    logs_dir = app_dir / "openmp" / ".fpc_logs"
    files = list(logs_dir.glob("rounding_error_*.json"))

    if not files:
        log_dirs = sorted(str(path) for path in app_dir.rglob(".fpc_logs"))
        detail = ""
        if log_dirs:
            detail = " Existing .fpc_logs directories: " + ", ".join(log_dirs)

        raise RuntimeError(
            "no FPChecker rounding JSON found in "
            + str(logs_dir)
            + ". This means the run completed without FPChecker writing rounding logs. "
            + "Check that openmp/Makefile uses mpicc-fpchecker and that "
            + "main.c calls _FPC_PRINT_LOCATIONS_FP64 before MPI_Finalize."
            + detail
        )

    return max(files, key=lambda path: path.stat().st_mtime)


def verify_instrumented_executable(executable_path, env):
    checks = [
        ["nm", "-a", str(executable_path)],
        ["strings", str(executable_path)],
    ]

    combined_output = ""

    for cmd in checks:
        tool = shutil.which(cmd[0], path=env.get("PATH", ""))
        if tool is None:
            continue

        completed = subprocess.run(
            cmd,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        combined_output += completed.stdout

        if "_FPC_" in completed.stdout:
            return

    raise RuntimeError(
        str(executable_path)
        + " does not contain FPChecker runtime symbols after build. "
        + "The compile step was not instrumented, so no .fpc_logs can be produced."
    )


def normalize_file(entry_file):
    return pathlib.Path(entry_file).name


def aggregate_rounding_errors(json_path, selected_files):
    with open(json_path, "r") as f:
        data = json.load(f)

    by_line = {}

    for entry in data:
        source_name = normalize_file(entry["file"])

        if source_name not in selected_files:
            continue

        key = (source_name, int(entry["line"]))

        current = by_line.setdefault(
            key,
            {
                "error": 0.0,
                "rel": 0.0,
            },
        )

        error = float(entry["error"])
        rel = float(entry["relative_error"])

        if abs(error) > abs(current["error"]):
            current["error"] = error

        if rel > current["rel"]:
            current["rel"] = rel

    return by_line


def rank_lines(keys, metric):
    def sort_key(key):
        value = metric(key)

        if math.isinf(value):
            return (0, 0.0, key[0], key[1])

        return (1, -value, key[0], key[1])

    ordered = sorted(keys, key=sort_key)

    return {key: index + 1 for index, key in enumerate(ordered)}


def find_injection_lines(source_path):
    lines = source_path.read_text().splitlines()
    injections = []

    for line_no, line in enumerate(lines, 1):
        if "// Injection" not in line:
            continue

        marker = line.split("// Injection", 1)[1].strip()
        injection_type = marker if marker else "unknown"

        injections.append(
            (
                source_path.name,
                line_no,
                line.strip(),
                injection_type,
            )
        )

    return injections


def print_table(rows):
    headers = [
        "Proxy app",
        "Injection line",
        "Injection type",
        "Rel. error",
        "Raw rank",
        "In top-5?",
        "In top-10?",
    ]

    widths = [len(header) for header in headers]

    for row in rows:
        for i, value in enumerate(row):
            widths[i] = max(widths[i], len(str(value)))

    def format_row(values):
        return "  ".join(str(value).ljust(widths[i]) for i, value in enumerate(values))

    print()
    print(format_row(headers))
    print(format_row("-" * width for width in widths))

    for row in rows:
        print(format_row(row))


def main():
    app_dir = pathlib.Path(__file__).resolve().parent
    openmp_dir = app_dir / "openmp"

    source_names = {source_path.name for source_path in SOURCE_PATHS}

    injection_lines = []

    for relative_source_path in SOURCE_PATHS:
        source_path = app_dir / relative_source_path
        injection_lines.extend(find_injection_lines(source_path))

    if not injection_lines:
        raise RuntimeError("no // Injection lines found in selected miniAMR sources")

    env = configure_environment(app_dir)

    print("Using mpicc-fpchecker:")
    run_command(["which", "mpicc-fpchecker"], app_dir, env)

    print("Using mpicc:")
    run_command(["which", "mpicc"], app_dir, env)

    print("MPI compile/link command:")
    run_command(["mpicc", "-show"], app_dir, env)

    print("Running injected build and execution...")

    shutil.rmtree(app_dir / ".fpc_logs", ignore_errors=True)
    shutil.rmtree(openmp_dir / ".fpc_logs", ignore_errors=True)

    run_command(["make", "clean"], openmp_dir, env)

    build_output = run_command(["make"], openmp_dir, env)

    if "FPC_INSTRUMENT_ERR_TRACKING_FP64=1" not in build_output:
        raise RuntimeError(
            "miniAMR/openmp build output did not show "
            "FPC_INSTRUMENT_ERR_TRACKING_FP64=1. "
            "Check that the Makefile has this in CC."
        )

    if "mpicc-fpchecker" not in build_output:
        raise RuntimeError(
            "miniAMR/openmp build output did not show mpicc-fpchecker. "
            "Check that the Makefile uses mpicc-fpchecker in CC."
        )

    verify_instrumented_executable(openmp_dir / "miniAMR.x", env)

    injected_output = run_command(RUN_COMMAND, openmp_dir, env, RUN_TIMEOUT_SECONDS)
    (openmp_dir / "run.out").write_text(injected_output)

    injected_json = latest_rounding_json(app_dir)

    print()
    print("Using FPChecker JSON:")
    print(injected_json)

    injected_errors = aggregate_rounding_errors(injected_json, source_names)

    def raw_rel(key):
        return injected_errors.get(key, {"rel": 0.0})["rel"]

    raw_ranks = rank_lines(set(injected_errors), raw_rel)

    rows = []

    for source_name, line_no, _, injection_type in injection_lines:
        key = (source_name, line_no)

        raw_rank = raw_ranks.get(key, "missing")
        rel_error = raw_rel(key)

        rows.append(
            [
                APP_NAME,
                f"{source_name}:{line_no}",
                injection_type,
                f"{rel_error:.6e}",
                raw_rank,
                "Yes" if isinstance(raw_rank, int) and raw_rank <= 5 else "No",
                "Yes" if isinstance(raw_rank, int) and raw_rank <= 10 else "No",
            ]
        )

    print_table(rows)

    diagnostics = []

    if "#FPCHECKER_ERROR" in injected_output:
        diagnostics.append("injected run emitted #FPCHECKER_ERROR")

    if diagnostics:
        print()
        print("Diagnostics: " + "; ".join(diagnostics))

    print()
    print("Ranks are computed within " + ", ".join(str(path) for path in SOURCE_PATHS) + ".")
    print("Rows are ranked by injected-run current relative error.")
    print("In top-5? and In top-10? report whether the injected line is within that rank cutoff.")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        sys.exit(1)
