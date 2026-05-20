#!/bin/bash -x

# CG options with and without preconditioner:
# -ksp_type cg
# -ksp_type cg -pc_type jacobi
# -ksp_type cg -pc_type icc

#PETSC_MATRIX=matrix.csv ./petsc_example -ksp_monitor -ksp_rtol 1.0e-5 -ksp_max_it 500 -ksp_type cg -pc_type jacobi
PETSC_MATRIX=matrix.csv ./petsc_example -ksp_monitor -ksp_rtol 1.0e-5 -ksp_max_it 500 -ksp_type cg
