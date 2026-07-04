#!/usr/bin/env python

import os
import re
import subprocess

import pytest

from error_analysis import report


def setup_module(module):
    this_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(this_dir)


def teardown_module(module):
    subprocess.check_output(["make clean"], stderr=subprocess.STDOUT, shell=True)


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


def has_entries_for(data, file_suffix):
    return any(entry["file"].endswith(file_suffix) for entry in data)


def test_fp64_stl_vector_shadow_crosses_copy_and_compilation_units():
    if not report.has_extended_fp64_reference():
        pytest.skip("Platform long double does not provide extra precision over double")

    run_shell("make")
    output = run_shell("./main")
    expected_error = parse_difference(output)
    assert abs(expected_error) > 1e-15

    data = report.loadReport(report.findRoundingErrorFile(".fpc_logs"))
    assert has_entries_for(data, "transform.cpp")
    assert has_entries_for(data, "consume.cpp")

    final_line = find_source_line("main.cpp", "FINAL_RESULT")
    final_errors = errors_for_line(data, "main.cpp", final_line)
    assert final_errors, "no FPChecker entry found for final expression"

    best_delta = min(abs(value - expected_error) for value in final_errors)
    assert best_delta < 5e-12
