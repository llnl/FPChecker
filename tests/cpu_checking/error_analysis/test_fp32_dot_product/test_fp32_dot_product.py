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
    cmd = ["./main 1000"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    fp64_total_error = 0.0
    for line in cmdOutput.decode().splitlines():
        if "Total Error (FMA)" in line:
            fp64_total_error = float(line.split()[3])
            break

    rounding_error = 0.0
    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)
    for i in range(len(data)):
      print('i', i, data[i])
      if data[i]['file'].endswith('vector_dot_product.cpp'):
        if data[i]['line'] == 26:
            rounding_error = data[i]['error']
            break

    print('rounding_error =', rounding_error)
    print('fp64_total_error =', fp64_total_error)
    diff = abs(fp64_total_error - rounding_error)
    print("Diff =", diff)

    accepted_threshold = 1e-7
    assert diff < accepted_threshold

