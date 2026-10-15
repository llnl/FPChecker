#!/usr/bin/python3

import os
import subprocess
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
MIXED_PRECISION_DIR = SCRIPT_DIR / "mixed_precision"

SEPARATOR = (
    "+--------------------------+------------+--------------+--------------+"
    "------------------------+"
)


def print_table_header():
    print(SEPARATOR)
    print(
        f"| {'Mode':<24} | {'Approach':<10} | {'FOM':<12} | "
        f"{'SpeedUp':<12} | {'Energy Rel. Error':<22} |"
    )
    print(SEPARATOR)


def print_table_row(mode, approach, fom, speedup, energy_rel_error):
    print(
        f"| {mode:<24} | {approach:<10} | {fom:>12} | "
        f"{speedup:<12} | {energy_rel_error:>22} |"
    )


def run_case(mode, approach, directory, command):
    #print(f"Running {mode} / {approach} in {directory}", file=sys.stderr)

    if not directory.is_dir():
        raise RuntimeError(f"missing experiment directory: {directory}")

    result = subprocess.run(
        command,
        cwd=directory,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=os.environ.copy(),
        universal_newlines=True,
        check=False,
    )

    if result.returncode != 0:
        raise RuntimeError(
            f"command failed in {directory}: {' '.join(command)}\n{result.stdout}"
        )

    for line in result.stdout.splitlines():
        fields = line.split()
        if fields and fields[0] == "MIXED":
            try:
                fom = fields[2]
                speedup = fields[3]
                energy_rel_error = fields[5]
            except IndexError as exc:
                raise RuntimeError(
                    f"malformed MIXED row from {directory}: {line}"
                ) from exc

            return mode, approach, fom, speedup, energy_rel_error

    raise RuntimeError(f"could not find MIXED row in output from {directory}\n{result.stdout}")


def main():
    cases = [
        (
            "Upcast (FP32 to FP64)",
            "Manual",
            MIXED_PRECISION_DIR / "LULESH_FP32" / "manual",
            ["./mixed_precision_cmd"],
        ),
        (
            "Upcast (FP32 to FP64)",
            "Agentic",
            MIXED_PRECISION_DIR / "LULESH_FP32" / "agent",
            ["./mixed_precision_cmd.sh"],
        ),
        (
            "Downcast (FP64 to FP32)",
            "Manual",
            MIXED_PRECISION_DIR / "LULESH_FP64" / "manual",
            ["./mixed_precision_cmd.sh"],
        ),
        (
            "Downcast (FP64 to FP32)",
            "Agentic",
            MIXED_PRECISION_DIR / "LULESH_FP64" / "agent",
            ["./compare_precision.sh"],
        ),
    ]

    rows = [run_case(*case) for case in cases]

    print_table_header()
    for index, row in enumerate(rows):
        print_table_row(*row)
        if index == 1:
            print(SEPARATOR)
    print(SEPARATOR)


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as exc:
        print(exc, file=sys.stderr)
        sys.exit(1)
