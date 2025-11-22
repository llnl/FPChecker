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
    cmd = ["./test_json"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

# Expected output of the test:
#
#Address            Register Name                  Error Value   Relative Error    Clock File Name             Line
#------------------ ------------------------- ---------------- ---------------- -------- -------------------- -------
#0x0000000000001000 -                                    0.034           0.0024        4 test_json.c              5
#-                  register_1                            0.01            0.001        1 test_json.c              3
#-                  register_2                            0.02            0.002        2 test_json.c              4
#-                  register_3                           0.034           0.0024        3 test_json.c              4

    found_separator = False
    found_line_1 = False
    found_line_3 = False
    found_line_2 = False
    found_address_1 = False
    for line in cmdOutput.decode().splitlines():
        if "---" in line:
            found_separator = True
            continue

        if found_separator:
            data = line.split()
            if len(data) == 7:
                address = data[0]
                register_name = data[1]
                error_value = float(data[2])
                relative_error = float(data[3])
                clock = int(data[4])
                line_number = int(data[6])

                if register_name == "register_1":
                    assert error_value == 0.01
                    assert relative_error == 0.001
                    assert clock == 1
                    assert line_number == 3
                    found_line_1 = True
                elif register_name == "register_2":
                    assert error_value == 0.02
                    assert relative_error == 0.002
                    assert clock == 2
                    assert line_number == 4
                    found_line_2 = True
                elif register_name == "register_3":
                    assert error_value == 0.034
                    assert relative_error == 0.0024
                    assert clock == 3
                    assert line_number == 4
                    found_line_3 = True
                elif address == "0x0000000000001000":
                    assert error_value == 0.034
                    assert relative_error == 0.0024
                    assert clock == 4
                    assert line_number == 5
                    found_address_1 = True

    assert found_address_1
    assert found_line_1
    assert found_line_2
    assert found_line_3