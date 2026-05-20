#!/usr/bin/env python3

import math
import subprocess
import sys


def parse_metrics(output: str):
    values = {}
    for line in output.splitlines():
        if "=" not in line:
            continue
        k, v = line.split("=", 1)
        k = k.strip()
        v = v.strip()
        if k in {"root_small", "variance", "objective"}:
            values[k] = float(v)
    return values


def rel_err(a: float, b: float) -> float:
    return abs(a - b) / max(abs(b), 1e-30)


def run_cmd(path: str, args: list):
    proc = subprocess.run([path] + args, check=True, capture_output=True, text=True)
    # Filter out FPChecker diagnostic lines from stdout
    clean = "\n".join(
        l for l in proc.stdout.splitlines()
        if not l.startswith("#FPCHECKER")
    )
    return parse_metrics(clean), clean


def print_rel_errors(title: str, values: dict, reference: dict):
    print(f"\n=== Relative error ({title} vs fp64) ===")
    for key in ["root_small", "variance"]:
        e = rel_err(values[key], reference[key])
        print(f"{key}: {e:.6e}")


def main():
    if len(sys.argv) < 6:
        print("Usage: compare_accuracy.py <fp32_exe> <mixed_exe> <fp64_exe> <n> <amp> [a] [b] [c]")
        return 1

    fp32_exe  = sys.argv[1]
    mixed_exe = sys.argv[2]
    fp64_exe  = sys.argv[3]
    run_args  = sys.argv[4:]   # n amp [a b c]

    fp32_vals,  fp32_raw  = run_cmd(fp32_exe,  run_args)
    mixed_vals, mixed_raw = run_cmd(mixed_exe, run_args)
    fp64_vals,  fp64_raw  = run_cmd(fp64_exe,  run_args)

    print("=== FP32 output ===")
    print(fp32_raw.strip())
    print("\n=== Mixed precision output ===")
    print(mixed_raw.strip())
    print("\n=== FP64 reference output ===")
    print(fp64_raw.strip())

    print_rel_errors("fp32", fp32_vals, fp64_vals)
    print_rel_errors("mixed", mixed_vals, fp64_vals)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
