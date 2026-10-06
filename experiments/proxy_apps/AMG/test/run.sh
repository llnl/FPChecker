#!/bin/bash -x

export OMP_NUM_THREADS=1

./amg -n 4 4 4 -P 1 1 1
