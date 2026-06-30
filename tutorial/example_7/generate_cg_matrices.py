#!/usr/bin/env python3

import argparse
import os
import numpy as np

def tridiag_toeplitz(n, a, b=-1.0):
    A = np.zeros((n, n))
    i = np.arange(n)
    A[i, i] = a
    A[i[1:], i[:-1]] = b
    A[i[:-1], i[1:]] = b
    return A

def cond2_spd(A):
    w = np.linalg.eigvalsh(A)
    print("eigen:", w)
    return float(w.max() / w.min())

def cond2_tridiag_toeplitz(n, a, b=-1.0):
    lo = a + 2*b*np.cos(np.pi/(n+1))
    hi = a + 2*b*np.cos(n*np.pi/(n+1))
    return float(max(lo, hi) / min(lo, hi))

def write_tridiag_toeplitz(path, n, a, b, kappa):
    with open(path, "w", encoding="ascii") as f:
        f.write("# FPChecker example_7 compact tridiagonal Toeplitz matrix\n")
        f.write("format=tridiagonal_toeplitz\n")
        f.write(f"n={n}\n")
        f.write(f"diag={a:.17e}\n")
        f.write(f"offdiag={b:.17e}\n")
        f.write(f"kappa={kappa:.17e}\n")

def output_name(n, index, kappa, extension, include_size):
    prefix = f"n{n}_" if include_size else ""
    return f"{prefix}{index}_matrix_{kappa:.3e}.{extension}"

def parse_args():
    parser = argparse.ArgumentParser(description="Generate CG tutorial matrices.")
    parser.add_argument("--sizes", nargs="+", type=int, default=[100],
                        help="Matrix sizes to generate.")
    parser.add_argument("--deltas", nargs="+", type=float,
                        default=[100.0, 1.0, 0.5, 1e-1, 1e-2, 1e-4, 1e-6, 1e-8],
                        help="Diagonal shifts controlling conditioning.")
    parser.add_argument("--format", choices=["dense", "tri"], default="dense",
                        help="dense writes CSV; tri writes compact tridiagonal files.")
    parser.add_argument("--output-dir", default="./matrices",
                        help="Directory for generated matrices.")
    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()

    # Create directory to save matrices if it doesn't exist
    dir_name = args.output_dir
    if not os.path.exists(dir_name):
        os.makedirs(dir_name)

    include_size = len(args.sizes) > 1
    for n in args.sizes:
        c = 2*np.cos(np.pi/(n+1))
        for i, d in enumerate(args.deltas):
            a = c + d
            kappa = cond2_tridiag_toeplitz(n, a=a, b=-1.0)
            print(f"n={n}, delta={d:>8}, kappa2={kappa:.3e}")
            if args.format == "tri":
                path = os.path.join(dir_name, output_name(n, i, kappa, "tri", include_size))
                write_tridiag_toeplitz(path, n, a, -1.0, kappa)
            else:
                A = tridiag_toeplitz(n, a=a, b=-1.0)
                path = os.path.join(dir_name, output_name(n, i, kappa, "csv", include_size))
                np.savetxt(path, A, delimiter=",")
            print(path)
