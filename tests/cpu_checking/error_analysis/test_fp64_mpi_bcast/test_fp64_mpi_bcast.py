#!/usr/bin/env python

import subprocess
import os
from error_analysis import report

def setup_module(module):
    THIS_DIR = os.path.dirname(os.path.abspath(__file__))
    os.chdir(THIS_DIR)

def teardown_module(module):
    cmd = ["make clean"]
    cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)

def test_bcast_error_preservation():
    # --- compile code ---
    cmd = ["make"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- run code with MPI ---
    cmd = ["mpirun --oversubscribe -np 1 ./bcast_test"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- parse FP64 reference error from stdout ---
    long_double_total_error = 0.0
    for line in cmdOutput.decode().splitlines():
        if "FP64 total error" in line:
            long_double_total_error = float(line.split()[-1])
            break

    # --- load JSON error report ---
    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)

    # Find the error at line 38 (double final_result = bcast_val + 1.0)
    # After MPI_Bcast, the error metadata for bcast_val should be preserved.
    # The hook re-associates error from the buffer address to itself (in-place).
    rounding_error = 0.0
    found = False
    for i in range(len(data)):
        print('i', i, data[i])
        if data[i]['file'].endswith('bcast_test.cpp'):
            if data[i]['line'] == 38:
                rounding_error = data[i]['error']
                found = True
                break

    print('found =', found)
    print('rounding_error =', rounding_error)
    print('long_double_total_error =', long_double_total_error)

    assert found, "No error entry found for bcast_test.cpp at line 38"
    diff = abs(long_double_total_error - rounding_error)
    print("Diff =", diff)
    accepted_threshold = 1e-7
    assert diff < accepted_threshold
