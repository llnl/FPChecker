#!/usr/bin/env python3

import json
import math
import os
import pathlib
import shutil
import subprocess
import sys


APP_NAME = "Quicksilver"
SOURCE_NAME = "EnergySpectrum.cc"
RUN_TIMEOUT_SECONDS = int(os.environ.get("QS_CANCELLATION_TIMEOUT", "120"))
INSTALL_BIN_CANDIDATES = [
    pathlib.Path(os.environ["FPCHECKER_INSTALL_BIN"])
    if "FPCHECKER_INSTALL_BIN" in os.environ
    else None,
    pathlib.Path(os.environ["FPCHECKER_HOME"]) / "bin"
    if "FPCHECKER_HOME" in os.environ
    else None,
    pathlib.Path("/g/g90/sharmin1/fpchecker/FPChecker/build/install/bin"),
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

    if shutil.which("clang++-fpchecker", path=env.get("PATH", "")) is None:
        raise RuntimeError(
            "clang++-fpchecker was not found. Add it to PATH or set "
            "FPCHECKER_INSTALL_BIN before running this script from " + str(app_dir)
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
            "command timed out after " + str(timeout) + " seconds: " + " ".join(cmd)
        )
    if completed.returncode != 0:
        print(completed.stdout)
        raise RuntimeError("command failed: " + " ".join(cmd))
    return completed.stdout


def latest_rounding_json(app_dir):
    logs_dir = app_dir / ".fpc_logs"
    files = list(logs_dir.glob("rounding_error_*.json"))
    if not files:
        raise RuntimeError("no FPChecker rounding JSON found in " + str(logs_dir))
    return max(files, key=lambda path: path.stat().st_mtime)


def is_source_file(entry_file, source_path):
    entry_path = pathlib.Path(entry_file)
    if entry_path.is_absolute():
        try:
            return entry_path.resolve() == source_path.resolve()
        except OSError:
            return entry_path.name == SOURCE_NAME
    return entry_path.name == SOURCE_NAME


def aggregate_rounding_errors(json_path, source_path):
    with open(json_path, "r") as f:
        data = json.load(f)

    by_line = {}
    for entry in data:
        if not is_source_file(entry["file"], source_path):
            continue
        line = int(entry["line"])
        current = by_line.setdefault(line, {"error": 0.0, "rel": 0.0})
        error = float(entry["error"])
        rel = float(entry["relative_error"])
        if abs(error) > abs(current["error"]):
            current["error"] = error
        if rel > current["rel"]:
            current["rel"] = rel
    return by_line


def rank_lines(lines, metric):
    def sort_key(line):
        value = metric(line)
        if math.isinf(value):
            return (0, 0.0, line)
        return (1, -value, line)

    ordered = sorted(lines, key=sort_key)
    return {line: index + 1 for index, line in enumerate(ordered)}


def find_injection_lines(source_path):
    lines = source_path.read_text().splitlines()
    injections = []
    for line_no, line in enumerate(lines, 1):
        if "// Injection" not in line:
            continue
        marker = line.split("// Injection", 1)[1].strip()
        injection_type = marker if marker else "unknown"
        injections.append((line_no, line.strip(), injection_type))
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
    source_path = app_dir / SOURCE_NAME
    injection_lines = find_injection_lines(source_path)
    if not injection_lines:
        raise RuntimeError("no // Injection lines found in " + str(source_path))

    env = configure_environment(app_dir)

    print("Running injected build and execution...")
    shutil.rmtree(app_dir / ".fpc_logs", ignore_errors=True)
    run_command(["make", "clean"], app_dir, env)
    run_command(["make"], app_dir, env)
    injected_output = run_command(["./run.sh"], app_dir, env, RUN_TIMEOUT_SECONDS)
    injected_json = latest_rounding_json(app_dir)
    injected_errors = aggregate_rounding_errors(injected_json, source_path)

    def raw_rel(line):
        return injected_errors.get(line, {"rel": 0.0})["rel"]

    raw_ranks = rank_lines(set(injected_errors), raw_rel)

    rows = []
    for line_no, _, injection_type in injection_lines:
        raw_rank = raw_ranks.get(line_no, "missing")
        rel_error = raw_rel(line_no)
        rows.append([
            APP_NAME,
            f"{SOURCE_NAME}:{line_no}",
            injection_type,
            f"{rel_error:.6e}",
            raw_rank,
            "Yes" if isinstance(raw_rank, int) and raw_rank <= 5 else "No",
            "Yes" if isinstance(raw_rank, int) and raw_rank <= 10 else "No",
        ])

    print_table(rows)

    diagnostics = []
    if "#FPCHECKER_ERROR" in injected_output:
        diagnostics.append("injected run emitted #FPCHECKER_ERROR")
    if diagnostics:
        print()
        print("Diagnostics: " + "; ".join(diagnostics))

    print()
    print("Ranks are computed within " + SOURCE_NAME + ".")
    print("Rows are ranked by injected-run current relative error.")
    print("In top-5? and In top-10? report whether the injected line is within that rank cutoff.")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        sys.exit(1)
