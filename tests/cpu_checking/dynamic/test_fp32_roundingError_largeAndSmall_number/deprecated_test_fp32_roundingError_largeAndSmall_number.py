

import subprocess
import os
import sys
import re
import numpy as np



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
        result = subprocess.check_output(["./main"], stderr=subprocess.STDOUT)
        runtime_output = result.decode()
        print(runtime_output)
    except subprocess.CalledProcessError as e:
        print("[RUNTIME ERROR]", e.output.decode())
        return

    #===== Runtime Output =====
    match = re.findall(r'#FPCHECKER: Accumulated error for (res_\d+) = ([\d.eE+-]+)', runtime_output)
    if match:
        final_var, error_value = match[-1]
        print(f"{final_var} = {error_value}")
        return np.float64(error_value)
    else:
        # fallback: try to find fallback_final_var in runtime output
        pattern = rf'{fallback_final_var} = ([\d.eE+-]+)'
        match = re.search(pattern, runtime_output)
        if match:
            error_value = match.group(1)
            print(f"{fallback_final_var} = {error_value} (fallback match)")
            return np.float64(error_value)
        else:
            print(f"[FAIL] Could not find accumulated error for {fallback_final_var} or any res_i")
            return None


def test_fp32_roundingError_addition():
    found = False
    adding_small_number = parse_fpchecker_output()
    assert adding_small_number is not None
   
    assert abs(adding_small_number) > 1e-9, f"Precision error too small: {adding_small_number}"
    found = True

    assert found