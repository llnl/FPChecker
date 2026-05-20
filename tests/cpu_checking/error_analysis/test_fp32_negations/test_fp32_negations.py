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

    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)

    error_1 = -1.0
    error_2 = -1.0

    for i in range(len(data)):
      print('i', i, data[i])
      if data[i]['file'].endswith('main.cpp'):
        if data[i]['line'] == 9:
            error_1 = data[i]['relative_error']
        elif data[i]['line'] == 10:
            error_2 = data[i]['relative_error']

    assert error_1 == 0.0
    assert error_2 == 0.0
