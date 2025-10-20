#!//usr/bin/env python3

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

if __name__ == "__main__":
    n = 100  # size of the matrix
    c = 2*np.cos(np.pi/(n+1))

    # delta factors to control conditioning
    delta = [100.0, 1.0, 0.5, 1e-1, 1e-2, 1e-4, 1e-6, 1e-8]

    # Create directory to save matrices if it doesn't exist
    dir_name = "./matrices"
    if not os.path.exists(dir_name):
        os.makedirs(dir_name)

    i = 0
    for d in delta:
        A = tridiag_toeplitz(n, a=c+d, b=-1.0)
        kappa = cond2_spd(A)
        print(f"delta={d:>8}, kappa2={kappa:.3e}")
        print(A)
        # Save matrix to file as CSV
        np.savetxt(os.path.join(dir_name, f"{i}_matrix_{kappa:.3e}.csv"), A, delimiter=",")
        i += 1

