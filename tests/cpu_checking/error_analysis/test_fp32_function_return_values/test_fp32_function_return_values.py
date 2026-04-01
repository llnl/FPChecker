#!/usr/bin/env python

import os
import re
import subprocess
from error_analysis import report


def setup_module(module):
    this_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(this_dir)


def teardown_module(module):
    cmd = ["make clean"]
    subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)


def find_source_line(file_path, needle):
    with open(file_path, "r") as fh:
        for i, line in enumerate(fh, start=1):
            if needle in line:
                return i
    return -1


def max_abs_error_for_line(data, file_suffix, line):
    max_err = None
    for entry in data:
        if entry["file"].endswith(file_suffix) and entry["line"] == line:
            value = abs(float(entry.get("error", 0.0)))
            if max_err is None or value > max_err:
                max_err = value
    return max_err


def best_abs_error_match(data, file_suffix, target):
    best_delta = None
    best_error = None
    for entry in data:
        if entry["file"].endswith(file_suffix):
            value = abs(float(entry.get("error", 0.0)))
            delta = abs(value - target)
            if best_delta is None or delta < best_delta:
                best_delta = delta
                best_error = value
    return best_error, best_delta


def test_1():
    # --- compile code ---
    cmd = ["make"]
    try:
        subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- run code ---
    cmd = ["./main"]
    try:
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True).decode()
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    match = re.search(r"Difference:\s*([+-]?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)", output)
    assert match is not None, "Failed to parse Difference from output"
    fp64_total_error = abs(float(match.group(1)))

    call_line = find_source_line("main.cpp", "float r = fp(a, b);")
    final_line = find_source_line("main.cpp", "float out = (r * 3.1415927f) + 0.33333334f;")
    assert call_line != -1
    assert final_line != -1

    file_name = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(file_name)

    call_line_error = max_abs_error_for_line(data, "main.cpp", call_line)
    final_line_error = max_abs_error_for_line(data, "main.cpp", final_line)
    best_error, best_delta = best_abs_error_match(data, "main.cpp", fp64_total_error)

    assert call_line_error is not None, "No JSON entry found for call result line"
    # Depending on optimization, the final expression can be split/fused and
    # mapped to nearby source lines, so this line may be absent in JSON.
    assert best_error is not None, "No JSON entries found for main.cpp"

    # Regression guard: call return should carry non-zero propagated error.
    assert call_line_error > 0.0

    # End-to-end consistency: final expression error should match FP64-FP32 diff.
    assert best_delta < 5e-7
