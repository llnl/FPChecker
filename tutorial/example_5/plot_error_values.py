#!/usr/bin/env python3

import argparse
import json
import os
from typing import List

import matplotlib.pyplot as plt
import numpy as np


def load_data(file_path: str):
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File not found: {file_path}")
    with open(file_path, "r") as f:
        return json.load(f)


def extract_line_values(data: List[dict], line: int):
    for item in data:
        if item.get("line") == line:
            return item.get("values", [])
    raise ValueError(f"Line {line} not found in JSON data")


def plot_values(
    values: List[float],
    line: int,
    output: str = None,
    show: bool = False,
):
    x_axis = np.arange(len(values))
    y_values = np.asarray(values, dtype=float)

    # Always use log scale on y-axis.
    # Log scale cannot represent non-positive values.
    non_positive = int(np.count_nonzero(y_values <= 0.0))
    if non_positive > 0:
        y_values = y_values.copy()
        y_values[y_values <= 0.0] = np.nan
        print(
            f"Note: {non_positive} non-positive points were hidden for log-scale plotting."
        )

    plt.figure(figsize=(11, 5.5))
    plt.plot(x_axis, y_values, linestyle="-", linewidth=1.0)

    ax = plt.gca()
    ax.set_yscale("log")
    ax.set_title(f"FPChecker relative error series for line {line}", fontsize=20)
    ax.set_xlabel("Execution index", fontsize=16)
    ax.set_ylabel("Relative error", fontsize=16)
    ax.tick_params(axis="both", which="major", labelsize=13)
    ax.tick_params(axis="both", which="minor", labelsize=11)
    ax.grid(True, linestyle="--", alpha=0.6)
    plt.tight_layout()

    if output:
        plt.savefig(output, dpi=150)
        print(f"Saved plot: {output}")

    if show:
        plt.show()
    else:
        plt.close()


def main():
    parser = argparse.ArgumentParser(
        description="Plot FPChecker errors_per_line JSON values for a selected source line"
    )
    parser.add_argument("json_file", help="Path to errors_per_line_*.json")
    parser.add_argument("--line", type=int, required=True, help="Source line to plot (example: 90)")
    parser.add_argument(
        "--output",
        default=None,
        help="Output image path (default: line_<line>_errors.png in current directory)",
    )
    parser.add_argument(
        "--show",
        action="store_true",
        help="Show interactive window in addition to saving",
    )
    args = parser.parse_args()

    output_file = args.output or f"line_{args.line}_errors.png"

    try:
        data = load_data(args.json_file)
        values = extract_line_values(data, args.line)
        if not values:
            raise ValueError(f"No values found for line {args.line}")

        print(
            f"Line {args.line}: points={len(values)}, min={min(values):.6e}, max={max(values):.6e}"
        )
        plot_values(
            values,
            args.line,
            output=output_file,
            show=args.show,
        )
    except Exception as exc:
        print(f"Error: {exc}")
        raise SystemExit(1)


if __name__ == "__main__":
    main()
