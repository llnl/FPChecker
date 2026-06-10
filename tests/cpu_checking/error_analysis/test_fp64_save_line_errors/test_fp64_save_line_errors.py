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

def test_1():
    # --- compile code ---
    cmd = ["make"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- run code ---
    cmd = ["FPC_SAVE_LINE_ERRORS=11,12 ./main"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    long_double_total_error = 0.0
    for line in cmdOutput.decode().splitlines():
        if "Total Error (AVERAGE)" in line:
            long_double_total_error = float(line.split()[3])
            break

    rounding_error = 0.0
    fileName = report.findErrorsPerLineFile('.fpc_logs')
    data = report.loadReport(fileName)

    assert len(data) == 2

    assert data[0]['line'] == 11
    assert data[1]['line'] == 12

    assert len(data[0]['values']) == 5
    assert len(data[1]['values']) == 5

