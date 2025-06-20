
import subprocess
import os
import sys
import re
import numpy as np

from dynamic import report

def setup_module(module):
    THIS_DIR = os.path.dirname(os.path.abspath(__file__))
    os.chdir(THIS_DIR)

def teardown_module(module):
    cmd = ["make clean"]
    cmdOutput = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)

def parse_fpchecker_output():
    try:
        make_result = subprocess.check_output(["make"], stderr=subprocess.STDOUT)
        make_output = make_result.decode()
    except subprocess.CalledProcessError as e:
        print("[COMPILE ERROR]", e.output.decode())
        return None

    # Find the number of instrumented operations
    matches = re.findall(r"#FPCHECKER: Instrumented (\d+) @", make_output)
    if not matches:
        return None

    last_count = int(matches[-1])
    final_var = f"res_{last_count - 1}"
    print(f"#FPCHECKER Intrumented: Final variable: {final_var}")

    # Run the instrumented binary
    try:
        result = subprocess.check_output(["./main", "200", "0.0002"], stderr=subprocess.STDOUT)
        runtime_output = result.decode()
        # print(runtime_output)
    except subprocess.CalledProcessError as e:
        print("[RUNTIME ERROR]", e.output.decode())
        return

    #===== Runtime Output =====
    # Extract accumulated error for final variable
    # pattern = rf"#FPCHECKER: Accumulated error for {final_var} = ([\d.eE+-]+)"
    pattern = rf'{final_var} = ([\d.eE+-]+)'
    match = re.search(pattern, runtime_output)
    if match:
        error_value = match.group(1)
        print(f"{final_var} = {error_value}")
    else:
        print(f"[FAIL] Could not find accumulated error for {final_var}")
    
    return np.float64(error_value)

def test_fp32_precision_loss():
    found = False
    precision_error = parse_fpchecker_output()
    # print(precision_error)
    assert precision_error is not None
   
    assert abs(precision_error) > 1e-9, f"Precision error too small: {precision_error}"
    found = True

    assert found