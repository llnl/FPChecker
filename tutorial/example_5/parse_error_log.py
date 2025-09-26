#! /usr/bin/env python3

import json
import sys

def print_line_and_relative_error(json_file_path):
    with open(json_file_path, 'r') as f:
        data = json.load(f)
    print("line|relative_error")
    for entry in data:
        print(f"{entry['line']}|{entry['relative_error']}")


if __name__ == "__main__":
    file_name = sys.argv[1]
    print(f"Parsing: {file_name}")
    print_line_and_relative_error(file_name)