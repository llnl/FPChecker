#!/usr/bin/env python

import pytest
import subprocess
import os
import sys
#sys.path.append('..')
#sys.path.append('.')
from error_analysis import report

def setup_module(module):
    THIS_DIR = os.path.dirname(os.path.abspath(__file__))
    os.chdir(THIS_DIR)

def teardown_module(module):
    cmd = ["make clean"]
    cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)

def compile_code(flags):
    cmd = ["make clean && make " + flags]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

def run_and_get_output():
    cmd = ["./main"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
        return cmdOutput
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

def assert_rounding_error_for_operation(flags, line):
    compile_code(flags)
    run_and_get_output()

    error_found = False
    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)

    for i in range(len(data)):
        print('i', i, data[i])
        if data[i]['file'].endswith('basic_operations.cpp') and data[i]['line'] == line:
            if abs(data[i]['error']) > 0:
                error_found = True
            break

    assert error_found, "Rounding error not found in basic_operations.cpp"


def test_basic_operations_all_in_one():
    if not report.has_extended_fp64_reference():
        pytest.skip("Platform long double does not provide extra precision over double")

    # Keep all operation checks in one test to avoid ordering/timing issues
    # when this folder is executed in parallel with a process-based runner.
    cases = [
        ("add", 12),
        ("sub", 24),
        ("mul", 36),
        ("div", 48),
    ]

    for flags, line in cases:
        assert_rounding_error_for_operation(flags, line)