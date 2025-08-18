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

def test_1():
    # --- compile code ---
    flags = "add"
    line = 12

    compile_code(flags)

    # --- run code ---
    cmdOutput = run_and_get_output()

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

def test_2():
    # --- compile code ---
    flags = "sub"
    line = 24

    compile_code(flags)

    # --- run code ---
    cmdOutput = run_and_get_output()

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


def test_3():
    flags = "mul"
    line = 36
    compile_code(flags)
    cmdOutput = run_and_get_output()

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

def test_4():
    flags = "div"
    line = 48
    compile_code(flags)
    cmdOutput = run_and_get_output()

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