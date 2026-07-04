#!/usr/bin/env python

import os
import re
import subprocess

from error_analysis import report


def setup_module(module):
    this_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(this_dir)


def teardown_module(module):
    subprocess.check_output(["make clean"], stderr=subprocess.STDOUT, shell=True)


def run_shell(cmd):
    return subprocess.check_output([cmd], stderr=subprocess.STDOUT, shell=True).decode()


def find_source_line(path, marker):
    with open(path, "r") as fh:
        for line_no, line in enumerate(fh, start=1):
            if marker in line:
                return line_no
    raise AssertionError(f"marker not found: {marker}")


def entry_for_line(data, file_suffix, line_no):
    matches = [
        entry
        for entry in data
        if entry["file"].endswith(file_suffix) and entry["line"] == line_no
    ]
    assert matches, f"no JSON entry for {file_suffix}:{line_no}"
    return matches[0]


def report_row_for_line(output, line_no):
    pattern = re.compile(rf"^\s*{line_no}\s+\|.+$", re.MULTILINE)
    match = pattern.search(output)
    assert match is not None, output
    return match.group(0)


def report_rows(output):
    return [
        line
        for line in output.splitlines()
        if re.match(r"^\s*\d+\s+\|", line)
    ]


def max_relative_error_from_row(row):
    return float(row.split("|")[-1])


def test_fp32_json_and_text_report_include_max_relative_error():
    run_shell("make")
    run_shell("./main")

    target_line = find_source_line("main.cpp", "TARGET_LINE")
    data = report.loadReport(report.findRoundingErrorFile(".fpc_logs"))
    entry = entry_for_line(data, "main.cpp", target_line)

    assert "relative_error" in entry
    assert "max_relative_error" in entry

    relative_error = float(entry["relative_error"])
    max_relative_error = float(entry["max_relative_error"])
    assert relative_error > 0.0
    assert max_relative_error > relative_error * 2.0

    text_report = run_shell("fpc-create-report -s rounding")
    assert "Max Rel. Error" in text_report
    row = report_row_for_line(text_report, target_line)
    assert len(row.split("|")) == 5

    ranked_report = run_shell("fpc-create-report -s rounding --rank-by-max-relative-error")
    rows = report_rows(ranked_report)
    assert len(rows) > 1
    max_relative_errors = [max_relative_error_from_row(row) for row in rows]
    assert max_relative_errors == sorted(max_relative_errors, reverse=True)
