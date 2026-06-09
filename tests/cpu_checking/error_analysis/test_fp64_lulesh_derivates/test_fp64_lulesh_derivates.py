#!/usr/bin/env python

# import subprocess
# import os
# import sys
# #sys.path.append('..')
# #sys.path.append('.')
# from error_analysis import report

# def setup_module(module):
#     THIS_DIR = os.path.dirname(os.path.abspath(__file__))
#     os.chdir(THIS_DIR)

# def teardown_module(module):
#     cmd = ["make clean"]
#     cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)

# def test_1():
#     # --- compile code ---
#     cmd = ["make"]
#     try:
#         cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
#     except subprocess.CalledProcessError as e:
#         print(e.output)
#         exit()

#     # --- run code ---
#     cmd = ["./main"]
#     try:
#         cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
#     except subprocess.CalledProcessError as e:
#         print(e.output)
#         exit()

#     # Check relative error in this LULESH case is:
#     # (1) zero, or
#     # (2) a value smaller than 1e-6
#     #
#     # A more stable FP64 check should use absolute error near cancellation:
#     #
#     # abs_tol = 1e-14
#     # rel_tol = 1e-6
#     # error = abs(data[i]['error'])
#     # relative_error = data[i]['relative_error']
#     # if error > abs_tol and relative_error > rel_tol:
#     #     correct_error = False
#     #     print('Too large error:', error, 'relative error:', relative_error)
#     #     break

#     correct_error = True
#     fileName = report.findRoundingErrorFile('.fpc_logs')
#     data = report.loadReport(fileName)
#     for i in range(len(data)):
#       print('i', i, data[i])
#       relative_error = data[i]['relative_error']
#       if relative_error > 0.5:
#           correct_error = False
#           print('Too large relative error:', relative_error)
#           break
      
#     assert correct_error

#!/usr/bin/env python

import subprocess
import os
import sys
from error_analysis import report


def setup_module(module):
    THIS_DIR = os.path.dirname(os.path.abspath(__file__))
    os.chdir(THIS_DIR)


def teardown_module(module):
    cmd = ["make clean"]
    subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)


def test_1():
    # --- compile code ---
    cmd = ["make"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
        print(cmdOutput.decode())
    except subprocess.CalledProcessError as e:
        print(e.output.decode())
        exit()

    # --- run code ---
    cmd = ["./main"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
        print(cmdOutput.decode())
    except subprocess.CalledProcessError as e:
        print(e.output.decode())
        exit()

    # --- parse long-double reference errors from stdout ---
    # Expected:
    #   Error [0][0]: ...
    #   ...
    #   Error [2][7]: ...
    #   Volume_error: ...
    long_double_errors = []
    volume_error = None

    for line in cmdOutput.decode().splitlines():
        line = line.strip()

        if line.startswith("Error ["):
            value = float(line.split(":")[1].strip())
            long_double_errors.append(value)

        if line.startswith("Volume Error"):
            if ":" in line:
                value = float(line.split(":")[1].strip())
            else:
                value = float(line.split("=")[1].strip())
            volume_error = value

    assert volume_error is not None, "Missing printed Volume Error"
    long_double_errors.append(volume_error)

    print("long_double_errors =", long_double_errors)
    print("volume_error =", volume_error)

    # --- read FPChecker report ---
    rounding_errors = []
    fileName = report.findRoundingErrorFile('.fpc_logs')
    data = report.loadReport(fileName)

    for i in range(len(data)):
        print('i', i, data[i])

    # Source-code lines for the 24 b[i][j] assignments
    line_numbers = [
        75, 76, 77, 78, 79, 80, 81, 82,
        84, 85, 86, 87, 88, 89, 90, 91,
        93, 94, 95, 96, 97, 98, 99, 100
    ]

    # Source-code line for volume assignment
    line_number_volume = 103

    # --- collect FPChecker errors for 24 b[i][j] lines ---
    for line_no in line_numbers:
        rounding_error = None

        for i in range(len(data)):
            if data[i]['file'].endswith('main.cpp'):
                if data[i]['line'] == line_no:
                    rounding_error = data[i]['error']
                    break

        assert rounding_error is not None, "Missing FPChecker error for line {}".format(line_no)
        rounding_errors.append(rounding_error)

    # --- collect FPChecker error for volume line ---
    rounding_error_volume = None

    for i in range(len(data)):
        if data[i]['file'].endswith('main.cpp'):
            if data[i]['line'] == line_number_volume:
                rounding_error_volume = data[i]['error']
                break

    assert rounding_error_volume is not None, "Missing FPChecker error for volume line {}".format(line_number_volume)

    rounding_errors.append(rounding_error_volume)

    print("rounding_errors =", rounding_errors)
    print("rounding_error_volume =", rounding_error_volume)

    # --- compare 25 errors: 24 b-errors + 1 volume-error ---
    accepted_threshold = 1e-15

    for i in range(25):
        rounding_error = rounding_errors[i]
        long_double_error = long_double_errors[i]

        diff = abs(long_double_error - rounding_error)

        print("index =", i)
        print("rounding_error =", rounding_error)
        print("long_double_error =", long_double_error)
        print("Diff =", diff)

        assert diff < accepted_threshold