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
    cmd = ["fpc-create-report"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

# FPCHECKER: Error: File '/path/does/not/exist/file.cpp' does not exist.
    found = False
    for line in cmdOutput.decode().splitlines():
        if "File '/path/does/not/exist/file.cpp' does not exist" in line:
            found = True
            break
    assert found

