#!/usr/bin/env python

import os
import re
import subprocess
import sys

try:
    from error_analysis import report
except ModuleNotFoundError:
    sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
    import report


EXPECTED_FILES = [
    "op_long_expression_a.cpp",
    "op_long_expression_b.cpp",
]


def setup_module(module):
    this_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(this_dir)


def teardown_module(module):
    cmd = ["make clean"]
    subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)


def _run_cmd(cmd):
    try:
        return subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output.decode())
        raise


def test_rounding_error_shell_table_truncation_and_format_multifile():
    _run_cmd(["make"])
    _run_cmd(["./main"])

    rounding_file = report.findRoundingErrorFile(".fpc_logs")
    data = report.loadReport(rounding_file)

    files_with_entries = set()
    for entry in data:
        files_with_entries.add(os.path.basename(entry.get("file", "")))

    for expected in EXPECTED_FILES:
        assert expected in files_with_entries

    out = _run_cmd(["fpc-create-report -s rounding_error"]).decode("utf-8")

    # Header with table columns separated by pipes.
    assert re.search(r"^Line\s+\|\s+Code\s+\|\s+Error\s+\|\s+Rel\. Error\s*$", out, re.MULTILINE)

    # Must contain at least one table section per expected file.
    file_headers = re.findall(r"^--- File:\s+(.+)$", out, re.MULTILINE)
    assert len(file_headers) >= len(EXPECTED_FILES)
    for expected in EXPECTED_FILES:
        assert any(os.path.basename(header.strip()) == expected for header in file_headers)

    # Row format must match: line | code | sci-error | sci-relative-error
    row_pattern = re.compile(
        r"^\s*\d+\s+\|\s+[^|]+\|\s+[-+]?\d\.\d{6}e[-+]\d{2}\s+\|\s+[-+]?\d\.\d{6}e[-+]\d{2}\s*$",
        re.MULTILINE,
    )
    rows = row_pattern.findall(out)
    assert len(rows) >= 2

    # With intentionally long source lines, truncation must happen at least once.
    assert re.search(r"\|\s+[^\n|]*\.\.\.\s+\|", out)
