#!/usr/bin/env python

import os
import re
import subprocess

from error_analysis import report


def setup_module(module):
    this_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(this_dir)


def teardown_module(module):
    run_shell("make clean")


def run_shell(cmd):
    return subprocess.check_output([cmd], stderr=subprocess.STDOUT, shell=True).decode()


def parse_difference(output):
    match = re.search(r"Difference:\s*([+-]?[0-9.]+(?:[eE][+-]?[0-9]+)?)", output)
    assert match is not None, output
    return float(match.group(1))


def find_source_line(path, marker):
    with open(path, "r") as fh:
        for line_no, line in enumerate(fh, start=1):
            if marker in line:
                return line_no
    raise AssertionError(f"marker not found: {marker}")


def errors_for_line(data, file_suffix, line_no):
    return [
        float(entry.get("error", 0.0))
        for entry in data
        if entry["file"].endswith(file_suffix) and entry["line"] == line_no
    ]


def test_o0_branch_is_canonicalized_for_shadow_control_flow():
    run_shell("make clean")
    compiler_path = run_shell("which clang++-fpchecker || true").strip()
    build_output = run_shell("make")
    output = run_shell("./main")
    expected_error = parse_difference(output)
    assert abs(expected_error + 10.0) < 1e-6

    data = report.loadReport(report.findRoundingErrorFile(".fpc_logs"))
    final_line = find_source_line("main.cpp", "FINAL_RESULT")
    final_errors = errors_for_line(data, "main.cpp", final_line)

    assert final_errors, "no FPChecker entry found for final expression"
    closest_error_delta = min(abs(value - expected_error) for value in final_errors)
    assert closest_error_delta < 1e-6, (
        f"expected an FPChecker error near {expected_error} on FINAL_RESULT, "
        f"but saw {final_errors}. This usually means clang++-fpchecker is using "
        f"an older libfpchecker_error without the O0 canonicalization pass.\n"
        f"clang++-fpchecker: {compiler_path}\n"
        f"build output:\n{build_output}\n"
        f"program output:\n{output}"
    )
