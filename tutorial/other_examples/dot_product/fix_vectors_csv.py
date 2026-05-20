#!/usr/bin/env python3

import argparse
import glob
import os
import re
import sys


def split_and_clean(line: str) -> list[str]:
    # Trim line endings/spaces, remove trailing commas, then normalize tokens.
    cleaned = line.strip().rstrip(",").strip()
    if not cleaned:
        return []
    tokens = [token.strip() for token in cleaned.split(",")]
    return [token for token in tokens if token != ""]


def process_file(path: str) -> tuple[int, int]:
    with open(path, "r", encoding="utf-8") as f:
        raw_lines = f.readlines()

    if len(raw_lines) < 2:
        raise ValueError("expected at least 2 lines")

    v1 = split_and_clean(raw_lines[0])
    v2 = split_and_clean(raw_lines[1])

    with open(path, "w", encoding="utf-8") as f:
        f.write(",".join(v1) + "\n")
        f.write(",".join(v2) + "\n")

    return len(v1), len(v2)


def natural_key(path: str) -> tuple:
    name = os.path.basename(path)
    return tuple(int(part) if part.isdigit() else part.lower() for part in re.split(r"(\d+)", name))


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Normalize two-line vector CSV files and print vector sizes."
    )
    parser.add_argument(
        "files",
        nargs="*",
        help="CSV files to process. If omitted, processes *.csv in current directory.",
    )
    args = parser.parse_args()

    files = args.files
    if not files:
        files = sorted(glob.glob("*.csv"), key=natural_key)

    if not files:
        print("No CSV files found.", file=sys.stderr)
        return 1

    exit_code = 0
    for path in files:
        try:
            n1, n2 = process_file(path)
            print(f"{path}: v1.size={n1}, v2.size={n2}")
        except Exception as exc:
            print(f"{path}: ERROR: {exc}", file=sys.stderr)
            exit_code = 1

    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())