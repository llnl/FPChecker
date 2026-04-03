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
    "op_addition.cpp",
    "op_subtraction.cpp",
    "op_multiplication.cpp",
    "op_division.cpp",
    "op_fma_chain.cpp",
    "op_reciprocal_sum.cpp",
    "op_horner.cpp",
    "op_sqrt_mix.cpp",
    "op_lerp.cpp",
    "op_dot_product.cpp",
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


def test_multifile_rounding_error_report_layout():
    _run_cmd(["make"])
    _run_cmd(["./main"])

    rounding_file = report.findRoundingErrorFile(".fpc_logs")
    data = report.loadReport(rounding_file)

    files_with_entries = set()
    files_with_nonzero_error = set()
    for entry in data:
        file_name = os.path.basename(entry.get("file", ""))
        if file_name in EXPECTED_FILES:
            files_with_entries.add(file_name)
            if abs(float(entry.get("error", 0.0))) > 0.0:
                files_with_nonzero_error.add(file_name)

    assert len(files_with_entries) >= 10
    assert len(files_with_nonzero_error) >= 10

    _run_cmd(["fpc-create-report -t \"multifile layout\""])

    html_path = "./fpc-report/index.html"
    assert os.path.exists(html_path)

    with open(html_path, "r") as fd:
        html = fd.read()

    # Validate the multi-file list styling and generated per-file tables.
    assert "hierarchy_list_class" in html
    assert "file_list_box" in html

    table_count = len(re.findall(r'<table width="600" class="report_box_source" id="rounding_error_file_', html))
    assert table_count >= 10

    header_count = html.count('class="rounding_file_header_cell"><b>File:</b> ...')
    assert header_count >= 10

    for file_name in EXPECTED_FILES:
        assert file_name in html
