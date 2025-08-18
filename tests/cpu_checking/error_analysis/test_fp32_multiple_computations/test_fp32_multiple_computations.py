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
    cmd = ["./main"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    fp64_total_error_1 = 0.0
    for line in cmdOutput.decode().splitlines():
        if "Total error 1" in line:
            fp64_total_error_1 = float(line.split()[3])
            break

    fp64_total_error_2 = 0.0
    for line in cmdOutput.decode().splitlines():
        if "Total error 2" in line:
            fp64_total_error_2 = float(line.split()[3])
            break

    rounding_error_1 = 0.0
    rounding_error_2 = 0.0
    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)
    for i in range(len(data)):
      print('i', i, data[i])
      if data[i]['file'].endswith('multiple_comp.cpp'):
        if data[i]['line'] == 14:
            rounding_error_1 = data[i]['error']
        if data[i]['line'] == 15:
            rounding_error_2 = data[i]['error']

    print('rounding_error 1 =', rounding_error_1)
    print('fp64_total_error 1 =', fp64_total_error_1)
    diff = abs(fp64_total_error_1 - rounding_error_1)
    print("Diff 1 =", diff)

    accepted_threshold = 1e-7
    assert diff < accepted_threshold

    print('rounding_error 2 =', rounding_error_2)
    print('fp64_total_error 2 =', fp64_total_error_2)
    diff = abs(fp64_total_error_2 - rounding_error_2)
    print("Diff 2 =", diff)
    
    assert diff < accepted_threshold

