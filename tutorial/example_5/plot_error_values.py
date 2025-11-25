import json
from sys import argv
import matplotlib.pyplot as plt
import numpy as np
import os

def plot_errors_from_json(file_path):
    """
    Opens a JSON file, loads the data, and plots the 'values' for each 'line' 
    in the JSON array. The y-axis is shown in scientific notation.
    """
    if not os.path.exists(file_path):
        print(f"Error: File not found at {file_path}")
        return

    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
    except json.JSONDecodeError:
        print(f"Error: Could not decode JSON from {file_path}")
        return
    except Exception as e:
        print(f"An error occurred while reading the file: {e}")
        return

    for item in data:
        try:
            line_num = item['line']
            values = item['values']
            x_axis = np.arange(len(values))

            plt.figure(figsize=(10, 6))
            plt.plot(x_axis, values, marker='.', linestyle='-', markersize=4)

            # Force scientific notation on the y-axis
            ax = plt.gca()
            ax.ticklabel_format(style='sci', axis='y')

            # Optionally move the offset (10^n) to the right of the axis
            #ax.yaxis.set_offset_position('right')
            
            # switch to logarithmic y-axis (values must be > 0)
            ax.set_yscale('log')

            # preserve scientific notation for tick labels
            #ax.yaxis.set_major_formatter(plt.ScalarFormatter())
            #ax.yaxis.get_major_formatter().set_scientific(True)
            #ax.yaxis.get_major_formatter().set_powerlimits((0,0))
            #ax.minorticks_on()

            plt.title(f'Errors for Line {line_num}', fontsize=16)
            plt.xlabel('Index of Value', fontsize=12)
            plt.ylabel('Error Value (y-axis)', fontsize=12)
            plt.grid(True, linestyle='--', alpha=0.7)
            plt.tight_layout()
            plt.show()

        except KeyError as e:
            print(f"Skipping an item: Missing key {e} in object.")
        except Exception as e:
            print(f"An error occurred while plotting for line {item.get('line', 'Unknown')}: {e}")

if __name__ == "__main__":
    if len(argv) != 2:
        print("Usage: python plot_error_values.py <json_file>")
        exit(1)

    file_name = argv[1]
    plot_errors_from_json(file_name)
