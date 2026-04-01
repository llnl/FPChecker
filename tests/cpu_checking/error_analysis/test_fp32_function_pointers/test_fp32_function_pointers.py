#!/usr/bin/env python

import subprocess
import os
import re
#sys.path.append('..')
#sys.path.append('.')
from error_analysis import report

def setup_module(module):
    THIS_DIR = os.path.dirname(os.path.abspath(__file__))
    os.chdir(THIS_DIR)

def teardown_module(module):
    cmd = ["make clean"]
    cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)

def find_source_line(file_path, needle):
    with open(file_path, "r") as fh:
        for i, line in enumerate(fh, start=1):
            if needle in line:
                return i
    return -1


def best_abs_error_match(data, file_suffix, target):
    best_delta = None
    best_error = None
    for entry in data:
        if entry['file'].endswith(file_suffix):
            value = float(entry.get('error', 0.0))
            delta = abs(abs(value) - abs(target))
            if best_delta is None or delta < best_delta:
                best_delta = delta
                best_error = value
    return best_error, best_delta

def test_1():
    # --- compile code ---
    cmd = ["make"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- run code ---
    cmd = ["./main \"0.123f,2.123f,3.123f\" scaled"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    output = cmdOutput.decode()
    diff_match = re.search(r"Difference:\s*([+-]?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)", output)
    assert diff_match is not None, "Failed to parse Difference from program output"
    fp64_total_error = float(diff_match.group(1))

    target_line = find_source_line("main.cpp", "val_sum_f += values_f[i] + result_f;")
    assert target_line != -1, "Could not find FP32 accumulation line in main.cpp"

    rounding_error = None
    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)

    for i in range(len(data)):
        print('i', i, data[i])
        if data[i]['file'].endswith('main.cpp'):
            if data[i]['line'] == target_line:
                rounding_error = data[i]['error']
                break

    assert rounding_error is not None, "Could not find rounding error entry for target line"

    best_error, best_delta = best_abs_error_match(data, 'main.cpp', fp64_total_error)
    assert best_error is not None, "No JSON entries found for main.cpp"

    print('rounding_error =', rounding_error)
    print('fp64_total_error =', fp64_total_error)
    diff = abs(fp64_total_error - rounding_error)
    print("Diff =", diff)

    assert best_delta < 5e-7

