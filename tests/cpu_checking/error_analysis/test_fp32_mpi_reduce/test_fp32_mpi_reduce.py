#!/usr/bin/env python

import pytest
import subprocess
import os
from error_analysis import report

def setup_module(module):
    THIS_DIR = os.path.dirname(os.path.abspath(__file__))
    os.chdir(THIS_DIR)

def teardown_module(module):
    cmd = ["make clean"]
    cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)

def test_reduce_error_propagation():
    # --- compile code ---
    cmd = ["make"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    # --- run code with MPI ---
    cmd = ["mpirun --oversubscribe -np 1 ./reduce_test"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        if report.mpi_runtime_broken(e.output):
            pytest.skip("OpenMPI runtime crashes in PMIx_Finalize on this environment")
        print(e.output)
        exit()

    # --- parse FP64 reference error from stdout ---
    fp64_total_error = 0.0
    for line in cmdOutput.decode().splitlines():
        if "FP64 total error" in line:
            fp64_total_error = float(line.split()[-1])
            break

    # --- load JSON error report ---
    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)

    # Find the error at line 39 (float final_result = reduced_sum + 1.0f)
    # This is the first FP operation AFTER MPI_Reduce that uses the result.
    rounding_error = 0.0
    found = False
    for i in range(len(data)):
        print('i', i, data[i])
        if data[i]['file'].endswith('reduce_test.cpp'):
            if data[i]['line'] == 39:
                rounding_error = data[i]['error']
                found = True
                break

    print('found =', found)
    print('rounding_error =', rounding_error)
    print('fp64_total_error =', fp64_total_error)

    assert found, "No error entry found for reduce_test.cpp at line 39"
    diff = abs(fp64_total_error - rounding_error)
    print("Diff =", diff)
    accepted_threshold = 1e-7
    assert diff < accepted_threshold
