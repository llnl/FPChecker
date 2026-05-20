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
    cmd = ["./main 9.3"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)

    error = 1e10

    for i in range(len(data)):
      print('i', i, data[i])
      if data[i]['file'].endswith('main.cpp'):
        if data[i]['line'] == 17:
            error = data[i]['relative_error']

    assert error < 1e-6
