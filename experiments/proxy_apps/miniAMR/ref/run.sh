#!/bin/bash -x

./miniAMR.x --num_refine 0 --num_tsteps 2 --stages_per_ts 2 --nx 4 --ny 4 --nz 4 --num_vars 4 --comm_vars 4 --checksum_freq 1 --report_perf 0
