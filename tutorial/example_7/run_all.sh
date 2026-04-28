#!/bin/bash -x

export CG_MATRIX=matrices/5_matrix_3.998e+04.csv

echo "=== FP32 ==="
./cg 500 1e-6

echo "=== Mixed ==="
./cg_mixed 500 1e-6

echo "=== FP64 ==="
./cg_FP64 500 1e-6
