
# Mixed Precision in LULESH

## One-time setup

The instructions below assume the FPChecker repository root is:
```
/Users/lagunaperalt1/IGNACIO/FPChecker
```

The FPChecker build and install directories are:
```
/Users/lagunaperalt1/IGNACIO/FPChecker/build
/Users/lagunaperalt1/IGNACIO/FPChecker/build/install
```

If `build/install` does not exist yet, build and install FPChecker first:
```
cd /Users/lagunaperalt1/IGNACIO/FPChecker
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=./install ..
make -j 4
make install
```

Before compiling or running the instrumented LULESH binary, make sure the FPChecker tools are on `PATH`:
```
export PATH=/Users/lagunaperalt1/IGNACIO/FPChecker/build/install/bin:$PATH
```

## Compiling the code

There are currently 3 versions produced by the Makefile:
FP32: non-instrumented 32-bit precision
FP64: non-instrumented 64-bit precision
fpchecker:  instrumented 32-bit precision (with FPChecker)

## How to compile the code with FPChecker

Make sure the clang++ is available by loading the conda env:
```
source ~/.bash_profile 
conda activate tutorial_env
export PATH=/Users/lagunaperalt1/IGNACIO/FPChecker/build/install/bin:$PATH
```

This should be the right clang++:
```
 $ which clang++
/opt/miniconda3/envs/tutorial_env/bin/clang++
```

Then run (for the instrumented version):
```
cd /Users/lagunaperalt1/IGNACIO/FPChecker/temp/LULESH
make clean
make fpchecker
```

This creates the instrumented executable:
```
/Users/lagunaperalt1/IGNACIO/FPChecker/temp/LULESH/lulesh2.0_fpchecker
```

## How to run the code
Use this command for uninstrumented versions:
```
lulesh2.0_fp32 -p -i 5 -s 16
```
or 
```
lulesh2.0_fp64 -p -i 5 -s 16
```

For the instrumented version, run from the LULESH directory so the trace files are written to the local `.fpc_logs` directory:
```
cd /Users/lagunaperalt1/IGNACIO/FPChecker/temp/LULESH
source ~/.bash_profile
conda activate tutorial_env
export PATH=/Users/lagunaperalt1/IGNACIO/FPChecker/build/install/bin:$PATH
./lulesh2.0_fpchecker -p -i 5 -s 16
```

If you want to rerun the instrumented version and keep only the new traces, remove the old traces first:
```
cd /Users/lagunaperalt1/IGNACIO/FPChecker/temp/LULESH
rm -rf .fpc_logs fpc-report
```

Options meaning:
-p: print the output
-i: number of iterations. 5 to 10 is good for profiling with FPChecker. If we are measuring overhead in non-instrumented runs, running more iterations is better, between 20-50.
-s: size of the problem. It must be a cube value. 16 = 2*2*4. For non-instrumented runs, larger sizes are better: 27, 64 for example.

## How to create an error report

After executing the instrumented code, run this from the same `temp/LULESH` directory:
```
export PATH=/Users/lagunaperalt1/IGNACIO/FPChecker/build/install/bin:$PATH
fpc-create-report -s rounding
```

This prints the rounding error report to the terminal using the traces stored in:
```
/Users/lagunaperalt1/IGNACIO/FPChecker/temp/LULESH/.fpc_logs
```

## How to interpret the error reports

There are 4 columns:
- Line number
- Code
- Error (absolute rounding error accumulated)
- Rel. Error (relative rounding error accumulated)

## How to interpret the output of LULESH

Here's an example:

```
cycle = 1, time = 7.863039e-05, dt=7.863039e-05
cycle = 2, time = 1.729869e-04, dt=9.435647e-05
cycle = 3, time = 2.053287e-04, dt=3.234180e-05
cycle = 4, time = 2.323366e-04, dt=2.700791e-05
cycle = 5, time = 2.564131e-04, dt=2.407651e-05
Run completed:
   Problem size        =  9
   MPI tasks           =  1
   Iteration count     =  5
   Final Origin Energy =  2.383040e+05
   Testing Plane 0 of Energy Array on rank 0:
        MaxAbsDiff   = 1.831055e-04
        TotalAbsDiff = 1.837910e-04
        MaxRelDiff   = 1.096908e-04

Elapsed time         =      0.002 (s)
Grind time (us/z/c)  = 0.53799725 (per dom)  (  0.001961 overall)
FOM                  =  1858.7456 (z/s)
```

We are interested in two classes of quantities: accuracy and performance:

- Accuracy: for accuracy we look at MaxAbsDiff, and TotalAbsDiff, MaxRelDiff.
- Performance: for performance we look at Elapsed time and FOM (figure of merit in zones per second). For Elapsed time, lower the better. For FOM, higher the better.

## What accuracy is acceptable

Measure the relative error of the Diff values:
```
error = abs(diff_FP64 - diff_mixed) / abs(diff_FP64)
```
We can consider different levels of accuracy where the error
is at levels 1e-2, 1e-3, 1e-5, 1e-7.

## Goal of the project

Our goal is to find a mixed precision version of the code 
that is "good" in terms of accuracy and performance.
These are different cases:

- Case 1: the accuracy is close to the FP64 accuracy; performance is better than both FP64 and FP32.
- Case 2: the accuracy is close to the FP64 accuracy; performance is better than only FP64.
- Case 3: the accuracy is close to the FP64 accuracy; performance is better than only FP32.

Note that in all cases we want accuracy as close to FP64 as possible. the ideal case is Case 1.


## How to run the experiments

- When finding a mixed precision configuration we should use a size not too large for instrumentation and obtaining the error report. 
This is because the instrumented program has a lot of overhead.

- Use a larger size for the evaluation for mixed, FP32 and FP64. That way we expose more opportunities to see the difference between mixed and the others.

## Evaluation sizes

For the evaluation use large sizes: 64, 96, 128, 256.
Use 100 iterations.

## Code changes to find a mixed precision version

1. Generate an error report with the instrumented version.
2. Find lines of code that have high relative error. These lines show instability where more precision is needed. Consider that some imght have Infinity because FPChecker computed the relative error possibly the denominator was zero.
3. Promote those lines to higher precision (FP64).
4. Run the three versions (mixed, fp32, fp64) and check if it satisfies Case 1.
5. If yes, stop. If not, continue again (steps 1 or 2) until we find a case 1.
6. When done, report the accuracy and speedup.