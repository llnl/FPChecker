#!/usr/bin/env python

import subprocess
import os
import shutil
import tempfile


THIS_DIR = os.path.dirname(os.path.abspath(__file__))
TEMP_DIRS = []


def has_positive_compute_instrumentation(output):
    for line in output.splitlines():
        prefix = "#FPCHECKER: Instrumented "
        suffix = " @ compute.cpp"
        if line.startswith(prefix) and line.endswith(suffix):
            count = int(line[len(prefix):line.index(suffix)])
            if count > 0:
                return True
    return False


def make_isolated_dir(makefile_name):
    build_dir = tempfile.mkdtemp(prefix="fpc_bb_", dir=THIS_DIR)
    TEMP_DIRS.append(build_dir)
    shutil.copy2(os.path.join(THIS_DIR, "main.cpp"), build_dir)
    shutil.copy2(os.path.join(THIS_DIR, "compute.cpp"), build_dir)
    shutil.copy2(os.path.join(THIS_DIR, "compute.h"), build_dir)
    shutil.copy2(os.path.join(THIS_DIR, makefile_name), build_dir)
    return build_dir


def run_make(makefile_name):
    build_dir = make_isolated_dir(makefile_name)
    return run_command("make -f " + makefile_name, cwd=build_dir).decode("utf-8")

def setup_module(module):
    os.chdir(THIS_DIR)

def teardown_module(module):
    for build_dir in TEMP_DIRS:
        shutil.rmtree(build_dir, ignore_errors=True)

def run_command(cmd, cwd=None):
    try:
        cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True, cwd=cwd)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise
    return cmdOutput

def test_0():
    # --- compile code ---
    output = run_make("Makefile_0")

    flag_1 = False
    flag_2 = False
    for line in output.splitlines():
        if "#FPCHECKER: Instrumented 0 @ main.cpp" in line:
            flag_1 = True
        if "#FPCHECKER: Instrumented 0 @ compute.cpp" in line:
            flag_2 = True
    assert flag_1
    assert flag_2

def test_1():
    # --- compile code ---
    output = run_make("Makefile_1")

    flag_1 = False
    flag_2 = has_positive_compute_instrumentation(output)
    for line in output.splitlines():
        if "#FPCHECKER: Instrumented 0 @ main.cpp" in line:
            flag_1 = True
    assert flag_1
    assert flag_2

def test_2():
    # --- compile code ---
    output = run_make("Makefile_2")

    flag_1 = False
    flag_2 = has_positive_compute_instrumentation(output)
    for line in output.splitlines():
        if "#FPCHECKER: Instrumented 0 @ main.cpp" in line:
            flag_1 = True
    assert flag_1
    assert flag_2

def test_3():
    # --- compile code ---
    output = run_make("Makefile_3")

    flag_1 = False
    flag_2 = False
    for line in output.splitlines():
        if "#FPCHECKER: Instrumented 0 @ main.cpp" in line:
            flag_1 = True
        if "#FPCHECKER: Instrumented 0 @ compute.cpp" in line:
            flag_2 = True
    assert flag_1
    assert flag_2