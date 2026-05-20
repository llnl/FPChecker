#!/usr/bin/env python

import subprocess
import os
import sys

def setup_module(module):
    THIS_DIR = os.path.dirname(os.path.abspath(__file__))
    os.chdir(THIS_DIR)

# Example of the putput we want to test that is correct:

#$ fpchecker-show 
#========================================
#         FPChecker Configuration        
#========================================
#
#Installation path: /Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install
#
#Add the following to CFLAGS and/or CXXFLAGS:
#
#(1) For exceptions checking:
#-g -include /Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/src/Runtime_cpu.h -fpass-plugin=/Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/lib/libfpchecker_cpu.dylib
#
#(2) For rounding error tracking:
#-g -include /Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/src/Runtime_error.h -fpass-plugin=/Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/lib/libfpchecker_error.dylib
#
#Wrappers are located here:
#/Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/bin/clang-fpchecker
#/Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/bin/clang++-fpchecker
#/Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/bin/mpicc-fpchecker
#/Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/bin/mpicxx-fpchecker

def test_1():
    # --- compile code ---
    cmd = ["fpchecker-show"]
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        exit()

    lines = cmdOutput.decode("utf-8").splitlines()

    pattern = "For exceptions checking"
    flags = None
    for i in range(len(lines)):
        if pattern in lines[i]:
            flags = lines[i+1].strip()
            break
    assert flags is not None
    expected_flags_part1 = "-g"
    expected_flags_part_include = " -include "
    expected_flags_part2 = " -fpass-plugin="
    assert expected_flags_part1 in flags
    assert expected_flags_part_include in flags
    assert expected_flags_part2 in flags

    pattern = "For rounding error tracking"
    flags = None
    for i in range(len(lines)):
        if pattern in lines[i]:
            flags = lines[i+1].strip()
            break
    assert flags is not None
    expected_flags_part1 = "-g"
    expected_flags_part_include = " -include "
    expected_flags_vectorize = "-fno-vectorize -fno-slp-vectorize"
    expected_flags_part2 = " -fpass-plugin="
    assert expected_flags_part1 in flags
    assert expected_flags_part_include in flags
    assert expected_flags_vectorize in flags
    assert expected_flags_part2 in flags

    # Check that after "Wrappers are located here:" we have four lines with the wrappers
    pattern = "Wrappers are located here:"
    wrappers = []
    for i in range(len(lines)):
        if pattern in lines[i]:
            wrappers = lines[i+1:i+5]
            break
    assert len(wrappers) == 4
    expected_wrappers = [
        "clang-fpchecker",
        "clang++-fpchecker",
        "mpicc-fpchecker",
        "mpicxx-fpchecker"
    ]
    for wrapper, expected in zip(wrappers, expected_wrappers):
        assert expected in wrapper

