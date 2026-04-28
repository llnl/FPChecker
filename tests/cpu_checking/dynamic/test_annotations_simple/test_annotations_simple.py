#!/usr/bin/env python

import subprocess
import os
import shutil
import pytest
from dynamic import report

THIS_DIR = os.path.dirname(os.path.abspath(__file__))

def run_command(cmd, cwd):
    try:
        cmd_output = subprocess.check_output(cmd, stderr=subprocess.STDOUT, cwd=cwd, shell=True)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise
    return cmd_output

def foundNaN(logs_dir, line_number):
    found = False
    file_name = report.findReportFile(logs_dir)
    data = report.loadReport(file_name)
    for i in range(len(data)):
      print('i', i, data[i])
      if data[i]['file'].endswith('compute.cpp'):
        if data[i]['nan'] > 0:
          if data[i]['line'] == line_number:
            found = True
            break

    return found

def prepare_workspace(tmp_path, makefile_name):
    workspace = tmp_path / makefile_name
    workspace.mkdir()

    shutil.copy2(os.path.join(THIS_DIR, 'main.cpp'), workspace / 'main.cpp')
    shutil.copy2(os.path.join(THIS_DIR, 'compute.cpp'), workspace / 'compute.cpp')
    shutil.copy2(os.path.join(THIS_DIR, 'compute.h'), workspace / 'compute.h')
    shutil.copy2(os.path.join(THIS_DIR, makefile_name), workspace / makefile_name)

    return workspace

@pytest.mark.parametrize(
    "makefile_name,expect_nan",
    [
        ("Makefile_0", False),
        ("Makefile_1", False),
        ("Makefile_2", False),
        ("Makefile_3", True),
        ("Makefile_4", False),
        ("Makefile_5", True),
    ],
)
def test_annotations_simple_cases(tmp_path, makefile_name, expect_nan):
    workspace = prepare_workspace(tmp_path, makefile_name)

    run_command("make -f " + makefile_name, cwd=str(workspace))
    run_command("./main", cwd=str(workspace))

    line_number = 64
    logs_dir = str(workspace / ".fpc_logs")
    assert foundNaN(logs_dir, line_number) == expect_nan
