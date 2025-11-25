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

# #FPCHECKER: *** WARNING *** Function _Z3fooPfm calls other functions with floating-point values!

    found_warning_1 = False
    for line in cmdOutput.decode().splitlines():
        if "#FPCHECKER:" in line:
            if "*** WARNING ***" in line and "foo" in line and "calls functions that return floating-point values!" in line:
                found_warning_1 = True
    
    assert found_warning_1

