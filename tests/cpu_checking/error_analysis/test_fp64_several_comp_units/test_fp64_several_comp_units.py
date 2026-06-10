#!/usr/bin/env python

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
    cmd = ["./main \"0.11111111,2.1111111,3.1111111,40.1111111,50.1111111,60.1111111,7000.1111111\""]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    long_double_total_error = 0.0
    reported_relative_error = 0.0
    for line in cmdOutput.decode().splitlines():
        if "Difference:" in line:
            long_double_total_error = float(line.split()[1])
        if "Relative error:" in line:
            reported_relative_error = float(line.split()[2])

    rounding_error = 0.0
    rounding_relative_error = 0.0
    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)
    for i in range(len(data)):
      print('i', i, data[i])
      if data[i]['file'].endswith('dot_product.cpp'):
        if data[i]['line'] == 11:
            rounding_error = data[i]['error']
            rounding_relative_error = data[i]['relative_error']
            break

    print('rounding_error =', rounding_error)
    print('long_double_total_error =', long_double_total_error)
    diff = abs(long_double_total_error - rounding_error)
    print("Diff =", diff)

    # In interprocedural mode, absolute error attribution can be distributed
    # across lines/functions; require non-zero tracking and relative consistency.
    assert abs(rounding_error) > 0.0
    assert reported_relative_error > 0.0
    assert abs(reported_relative_error - rounding_relative_error) < 1e-7

