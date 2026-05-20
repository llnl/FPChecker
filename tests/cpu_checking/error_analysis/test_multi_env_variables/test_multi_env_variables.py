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

# Error: FPC_INSTRUMENT and FPC_INSTRUMENT_ERR_TRACKING are set! Only one instrumentation type can be used at a time.
    found_error = False
    for line in cmdOutput.decode().splitlines():
        if "FPC_INSTRUMENT and FPC_INSTRUMENT_ERR_TRACKING are set" in line and "Only one instrumentation type can be used at a time." in line:
            found_error = True
            break

    assert found_error