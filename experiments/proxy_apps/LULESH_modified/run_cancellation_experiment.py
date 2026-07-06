#!/usr/bin/env python3

import json
import math
import os
import pathlib
import subprocess
import sys


APP_NAME = "LULESH"
SOURCE_NAME = "lulesh.cc"
INSTALL_BIN = pathlib.Path("/g/g90/laguna/fpchecker/FPChecker/build/install/bin")


def run_command(cmd, cwd, env):
    print("+ " + " ".join(cmd), flush=True)
    completed = subprocess.run(
        cmd,
        cwd=cwd,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
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


def aggregate_rounding_errors(json_path):
    with open(json_path, "r") as f:
        data = json.load(f)

    by_line = {}
    for entry in data:
        key = (entry["file"], int(entry["line"]))
        current = by_line.setdefault(key, {"error": 0.0, "rel": 0.0})
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
            return (0, 0.0, key[1])
        return (1, -value, key[1])

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
    source_file = str(source_path.resolve())
    injection_lines = find_injection_lines(source_path)
    if not injection_lines:
        raise RuntimeError("no // Injection lines found in " + str(source_path))

    env = os.environ.copy()
    env["PATH"] = str(INSTALL_BIN) + os.pathsep + env.get("PATH", "")

    print("Running injected build and execution...")
    run_command(["make", "clean"], app_dir, env)
    run_command(["make"], app_dir, env)
    injected_output = run_command(["./run.sh"], app_dir, env)
    injected_json = latest_rounding_json(app_dir)
    injected_errors = aggregate_rounding_errors(injected_json)

    source_keys = {
        key for key in set(injected_errors)
        if key[0] == source_file
    }

    def raw_rel(key):
        return injected_errors.get(key, {"rel": 0.0})["rel"]

    raw_ranks = rank_lines(source_keys, raw_rel)

    rows = []
    for line_no, _, injection_type in injection_lines:
        key = (source_file, line_no)
        raw_rank = raw_ranks.get(key, "missing")
        rel_error = raw_rel(key)
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
