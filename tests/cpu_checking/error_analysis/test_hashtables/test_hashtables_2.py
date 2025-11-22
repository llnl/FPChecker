#!/usr/bin/env python

import subprocess
import os
import sys

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
    cmd = ["./test_many_items 50000"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

# Expected output of the test:
#
#Registers inserted: 100
#Addresses inserted: 100
#Address            Register Name                  Error Value   Relative Error    Clock
#------------------ ------------------------- ---------------- ---------------- --------
#...

    found_registers_inserted = False
    found_addresses_inserted = False
    
    for line in cmdOutput.decode().splitlines():
        if "Registers inserted: 50000" in line:
            found_registers_inserted = True
        if "Addresses inserted: 50000" in line:
            found_addresses_inserted = True
    
    assert found_registers_inserted
    assert found_addresses_inserted