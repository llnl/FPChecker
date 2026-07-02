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

def test_1():
    if not report.has_extended_fp64_reference():
        pytest.skip("Platform long double does not provide extra precision over double")

    # --- compile code ---
    cmd = ["make"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- run code ---
    cmd = ["./main"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    relative_error_1 = 0.0
    relative_error_2 = 0.0
    relative_error_3 = 0.0
    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)
    for i in range(len(data)):
      print('i', i, data[i])
      if data[i]['file'].endswith('solver.cpp'):
        if data[i]['line'] == 71:
            relative_error_1 = data[i]['relative_error']
        if data[i]['line'] == 74:
            relative_error_2 = data[i]['relative_error']
        if data[i]['line'] == 75:
            relative_error_3 = data[i]['relative_error']
            break

    accepted_threshold = 1e-7
    assert relative_error_1 < accepted_threshold
    assert relative_error_2 < accepted_threshold
    assert relative_error_3 < accepted_threshold
    assert (relative_error_1 != 0) or (relative_error_2 != 0) or (relative_error_3 != 0)

