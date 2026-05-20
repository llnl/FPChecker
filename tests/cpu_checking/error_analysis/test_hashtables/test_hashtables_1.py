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

def compile_code():
    cmd = ["make"]
    try:
        subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()


def run_cmd(cmd):
    try:
        return subprocess.check_output([cmd], stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()


def assert_basic_functionality(cmdOutput):
    # Expected output of the test:
    #
    #Address            Register Name             Function Name                  Error Value   Relative Error    Clock File Name             Line
    #------------------ ------------------------- ------------------------- ---------------- ---------------- -------- -------------------- -------
    #0x0000000000001000 -                         -                                     0.02            0.002        3 test_basic_functiona    10
    #0x0000000000002000 -                         -                                     0.01            0.001        4 test_basic_functiona    10
    #-                  register_1                myfunction                            0.01            0.001        1 test_basic_functiona    10
    #-                  register_3_loaded         myfunction                            0.02            0.002        5 test_basic_functiona    10
    #-                  register_2                myfunction                            0.02            0.002        2 test_basic_functiona    10

    found_separator = False
    found_line_1 = False
    found_line_3 = False
    found_line_2 = False
    found_address_1 = False
    found_address_2 = False
    for line in cmdOutput.decode().splitlines():
        if "---" in line:
            found_separator = True
            continue

        if found_separator:
            data = line.split()
            if len(data) == 8:
                address = data[0]
                register_name = data[1]
                error_value = float(data[3])
                relative_error = float(data[4])
                clock = int(data[5])

                if register_name == "register_1":
                    assert error_value == 0.01
                    assert relative_error == 0.001
                    assert clock == 1
                    found_line_1 = True
                elif register_name == "register_2":
                    assert error_value == 0.02
                    assert relative_error == 0.002
                    assert clock == 2
                    found_line_2 = True
                elif register_name == "register_3_loaded":
                    assert error_value == 0.02
                    assert relative_error == 0.002
                    assert clock == 5
                    found_line_3 = True
                elif address == "0x0000000000001000":
                    assert error_value == 0.02
                    assert relative_error == 0.002
                    assert clock == 3
                    found_address_1 = True
                elif address == "0x0000000000002000":
                    assert error_value == 0.01
                    assert relative_error == 0.001
                    assert clock == 4
                    found_address_2 = True

    assert found_address_1
    assert found_address_2
    assert found_line_1
    assert found_line_2
    assert found_line_3


def assert_many_items(cmdOutput):
    # Expected output of the test:
    #
    #Registers inserted: 50000
    #Addresses inserted: 50000
    found_registers_inserted = False
    found_addresses_inserted = False

    for line in cmdOutput.decode().splitlines():
        if "Registers inserted: 50000" in line:
            found_registers_inserted = True
        if "Addresses inserted: 50000" in line:
            found_addresses_inserted = True

    assert found_registers_inserted
    assert found_addresses_inserted


def assert_json_output_and_report(cmdOutput):
    # Expected output of the test:
    #
    #Address            Register Name             Function Name                  Error Value   Relative Error    Clock File Name             Line
    #------------------ ------------------------- ------------------------- ---------------- ---------------- -------- -------------------- -------
    #0x0000000000001000 -                         -                                    0.034           0.0024        4 test_json.c              5
    #-                  register_1                myfunction                            0.01            0.001        1 test_json.c              3
    #-                  register_2                myfunction                            0.02            0.002        2 test_json.c              4
    #-                  register_3                myfunction                           0.034           0.0024        3 test_json.c              4

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
            if len(data) == 8:
                address = data[0]
                register_name = data[1]
                error_value = float(data[3])
                relative_error = float(data[4])
                clock = int(data[5])
                line_number = int(data[7])

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

    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)

    error_1 = 0.0
    error_2 = 0.0
    error_3 = 0.0

    for i in range(len(data)):
        print('i', i, data[i])
        if data[i]['file'].endswith('test_json.c'):
            if data[i]['line'] == 3:
                error_1 = data[i]['relative_error']
            elif data[i]['line'] == 4:
                error_2 = data[i]['relative_error']
            elif data[i]['line'] == 5:
                error_3 = data[i]['relative_error']

    assert error_1 == 0.001
    assert error_2 == 0.0024
    assert error_3 == 0.0024


def test_hashtables_all_in_one():
    # Consolidate all hashtable checks in a single test to avoid timing/order
    # issues when tests are scheduled in parallel.
    compile_code()

    output_basic = run_cmd("./test_basic_functionality")
    assert_basic_functionality(output_basic)

    output_many = run_cmd("./test_many_items 50000")
    assert_many_items(output_many)

    output_json = run_cmd("./test_json")
    assert_json_output_and_report(output_json)