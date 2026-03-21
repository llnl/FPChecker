#!/usr/bin/env python3

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path


JSON_PATH_RE = re.compile(r"Writing errors per line to:\s*(\S+)")


def extract_json_path(run_output: str, workdir: Path) -> Path:
    match = JSON_PATH_RE.search(run_output)
    if not match:
        raise RuntimeError("Could not find 'Writing errors per line to:' in program output")
    path = Path(match.group(1))
    if not path.is_absolute():
        path = workdir / path
    return path


def read_values_from_json(json_path: Path, target_line: int | None) -> list[float]:
    with json_path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    if not isinstance(data, list):
        raise RuntimeError(f"Unexpected JSON structure in {json_path}: expected top-level list")

    chosen_entry = None
    if target_line is not None:
        for entry in data:
            if isinstance(entry, dict) and entry.get("line") == target_line:
                chosen_entry = entry
                break
        if chosen_entry is None:
            raise RuntimeError(f"Line {target_line} not found in {json_path}")
    else:
        for entry in data:
            if isinstance(entry, dict) and "values" in entry:
                chosen_entry = entry
                break
        if chosen_entry is None:
            raise RuntimeError(f"No entry with 'values' found in {json_path}")

    values = chosen_entry.get("values")
    if not isinstance(values, list):
        raise RuntimeError(f"Invalid 'values' field in {json_path}")

    return [float(v) for v in values]


def run_dotprod(workdir: Path, csv_name: str, save_line: int) -> tuple[Path, str]:
    env = os.environ.copy()
    env["FPC_SAVE_LINE_ERRORS"] = str(save_line)
    env["DP_VECS"] = f"./{csv_name}"

    result = subprocess.run(
        ["./dotprod"],
        cwd=workdir,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )

    combined_output = (result.stdout or "") + (result.stderr or "")
    if result.returncode != 0:
        raise RuntimeError(
            f"dotprod failed for {csv_name} with exit code {result.returncode}\n{combined_output}"
        )

    json_path = extract_json_path(combined_output, workdir)
    if not json_path.exists():
        raise RuntimeError(f"JSON path reported but file not found: {json_path}")

    return json_path, combined_output


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run dotprod on numbered CSVs and report zero-error frequency per iteration."
    )
    parser.add_argument("--start", type=int, default=1, help="First CSV index (default: 1)")
    parser.add_argument("--end", type=int, default=5, help="Last CSV index (default: 5)")
    parser.add_argument(
        "--save-line",
        type=int,
        default=62,
        help="Value for FPC_SAVE_LINE_ERRORS (default: 62)",
    )
    parser.add_argument(
        "--target-line",
        type=int,
        default=62,
        help="Line number entry to read from errors_per_line JSON (default: 62)",
    )
    parser.add_argument(
        "--show-command-output",
        action="store_true",
        help="Print dotprod output for each run",
    )
    parser.add_argument(
        "--plot-file",
        default="zero_frequency.png",
        help="Output image path for the iteration-vs-percentage plot (default: zero_frequency.png)",
    )
    args = parser.parse_args()

    if args.start > args.end:
        print("Error: --start must be <= --end", file=sys.stderr)
        return 1

    workdir = Path(__file__).resolve().parent
    files = [f"{i}.csv" for i in range(args.start, args.end + 1)]

    all_values: list[list[float]] = []

    for csv_name in files:
        csv_path = workdir / csv_name
        if not csv_path.exists():
            print(f"{csv_name}: SKIP (file not found)", file=sys.stderr)
            continue

        try:
            json_path, output = run_dotprod(workdir, csv_name, args.save_line)
            values = read_values_from_json(json_path, args.target_line)
            all_values.append(values)
            print(f"{csv_name}: read {len(values)} values from {json_path}")
            if args.show_command_output:
                print(f"----- output for {csv_name} -----")
                print(output.rstrip())
                print("--------------------------------")
        except Exception as exc:
            print(f"{csv_name}: ERROR: {exc}", file=sys.stderr)

    total = len(all_values)
    if total == 0:
        print("No successful runs; cannot compute frequency.", file=sys.stderr)
        return 1

    max_len = max(len(v) for v in all_values)
    percentages: list[float] = []
    print("\nZero-frequency per iteration:")
    for i in range(max_len):
        zero_count = 0
        for values in all_values:
            if i < len(values) and values[i] == 0.0:
                zero_count += 1
        ratio = zero_count / total
        percentages.append(ratio)
        print(f"Iter {i}: {zero_count}/{total} ({ratio:.6g})")

    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print(
            "matplotlib is required for plotting. Install it with: pip install matplotlib",
            file=sys.stderr,
        )
        return 1

    x_values = list(range(max_len))
    plt.figure(figsize=(10, 4))
    plt.plot(x_values, percentages, marker="o", linewidth=1.5)
    plt.xlabel("Iteration")
    plt.ylabel("Percentage of zero values")
    plt.title("Zero-value frequency by iteration")
    plt.ylim(0.0, 1.0)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    plot_path = Path(args.plot_file)
    if not plot_path.is_absolute():
        plot_path = workdir / plot_path
    plt.savefig(plot_path, dpi=150)
    print(f"Plot saved to: {plot_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())