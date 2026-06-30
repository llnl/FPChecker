# How to use this example of CG of mixed, FP32, and FP64 versions

## To use FPChecker
export PATH=/usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install/bin:$PATH


## Compiling CG

There are three versions of CG in tutorial/example_7.

They can be compiled by:

```
make cg cg_mixed cg_FP64
```

## How to run CG

```
export CG_MATRIX=matrices/5_matrix_3.998e+04.csv
echo "=== FP32 ==="
./cg 500 1e-6
```

## How to generate new matrices

The generate_cg_matrices.py script can generate matrices and store them in ./matrices.
The variable 'n = 100' sets the size of the matrices.

## Measuring accuracy and speed

- Accuracy: we use the "Final Residual Norm (||Ax - b||):" in the output
- Speed: we multiply "Average time per iteration" by "Converged in X iterations" from the output. This should give a metric in seconds.

## Matrix sizes
You can try sizes, 500, 750, 1000, 2500, 5000, 10000. The script is needed to generate them. The condition number can be changed as needed.
