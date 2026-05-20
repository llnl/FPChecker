#!/usr/bin/env python

import subprocess
import os
import sys
#sys.path.append('..')
#sys.path.append('.')
#import report
from dynamic import report

def setup_module(module):
    THIS_DIR = os.path.dirname(os.path.abspath(__file__))
    os.chdir(THIS_DIR)

def teardown_module(module):
    cmd = ["make -f Makefile.1 clean"]
    cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)

def run_command(cmd):
    ret = 0
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        ret = e.returncode
    return ret

def test_conditional_operations():
    # --- compile code ---
    cmd = ["make -f Makefile.1"]
    run_command(cmd)

    cmd = ["FPC_TRAP_INFINITY_NEG=1 FPC_TRAP_DIVISION_ZERO=1 ./main"]
    assert run_command(cmd) == 0

    # --- compile code ---
    cmd = ["make -f Makefile.2"]
    run_command(cmd)

    cmd = ["FPC_TRAP_INFINITY_NEG=1 FPC_TRAP_DIVISION_ZERO=1 ./main"]
    assert run_command(cmd) == 0
