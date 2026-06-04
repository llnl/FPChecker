#!/usr/bin/env python

import subprocess
import os
import sys

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.abspath(os.path.join(THIS_DIR, "..", "..")))

from error_analysis import report

def setup_module(module):
    THIS_DIR = os.path.dirname(os.path.abspath(__file__))
    os.chdir(THIS_DIR)

def teardown_module(module):
    cmd = ["make clean"]
    cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)

def test_1():
    # --- compile code ---
    cmd = ["make"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- run code ---
    # For double input, do not use f suffix.
    cmd = ["./memcpy \"0.123,2.123,3.123\""]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    long_double_total_error = 0.0
    for line in cmdOutput.decode().splitlines():
        if "Difference" in line:
            long_double_total_error = float(line.split()[1])
            break

    rounding_error = 0.0
    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)

    for i in range(len(data)):
      print('i', i, data[i])
      if data[i]['file'].endswith('memcpy_fp64_test.cpp'):
        if data[i]['line'] == 56:
            rounding_error = data[i]['error']
            break

    print('rounding_error =', rounding_error)
    print('long_double_total_error =', long_double_total_error)
    diff = abs(long_double_total_error - rounding_error)
    print("Diff =", diff)

    accepted_threshold = 1e-15
    assert diff < accepted_threshold