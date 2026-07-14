# EFTSanitizer: Fast Shadow Execution using Error Free Transformations

by [Sangeeta Chowdhary](https://github.com/sangeeta0201) and [Santosh Nagarakatte](https://people.cs.rutgers.edu/~sn349/)


## About EFTSanitizer

EFTSanitizer is a tool for fast shadow execution. It uses error free
transformations as the oracle to detect and debug numerical errors.
For more details, read our [OOPSLA 2022
paper](https://people.cs.rutgers.edu/~sn349/papers/eftsanitizer-oopsla-2022.pdf).

The easiest way to test out EFTSanitizer is to use the [OOPSLA 2022
artifact](https://doi.org/10.5281/zenodo.7080559)

### Quick Start

#### Installing EFTSanitizer

We provide details for installing EFTSanitizer on a machine running Ubuntu.

1. Install a list of prerequisites:

```
   apt-get update
   apt-get install apt-utils build-essential cmake gdb jgraph libgmp3-dev libmpfr-dev python python3 texlive-font-utils wget

```
2. Download and compile llvm:
   
```
   wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/llvm-10.0.0.src.tar.xz
   tar -xvf llvm-10.0.0.src.tar.xz 
   mv llvm-10.0.0.src llvm
   wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/clang-10.0.0.src.tar.xz
   tar -xvf clang-10.0.0.src.tar.xz 
   mv clang-10.0.0.src clang
   mkdir build
   cd build
   cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" ../llvm
   make -j2
   cd ..

```

3. Set the following environment variables
```
   export CXX=clang++
   export CC=clang
   export LLVM_HOME=<path to LLVM build directory>
   export PATH=$LLVM_HOME/bin:$PATH
```

4. Clone EFTSanitizer git repo.
```
  git clone git@github.com:rutgers-apl/EFTSanitizer.git

```

5. Compile EFTSanitizer pass

    If your compiler does not support C++11 by default, add the following line to llvm-pass/EFTSan/CMakefile

```
  target_compile_feature(EFTSanitizer PRIVATE cxx_range_for cxx_auto_type)

```
otherwise, use the followng line

```
  target_compile_features(EFTSanitizer PRIVATE )

``` 
Then build the pass with the following commands

``` 
   cd EFTSanitizer/llvm-pass
   mkdir build
   cd build
   cmake ../
   make
   cd ../..

```

6.  Build the EFTSanitizer runtime environment

```
   cd runtime 
   make
   cd ..

```

7. Set the following environment variables

```
   export EFT_HOME=<path to the EFTSanitizer Github directory>
   export LD_LIBRARY_PATH=$EFT_HOME/runtime/obj
   export LLVM_PASS_LIB=" $EFT_HOME/llvm_pass/build/EFTSan/libEFTSanitizer.so -eftsan "

  
```


8. Run the Correctness test suite

```
  cd $EFT_HOME/correctness_test
  python3 correctness_test.py

```

You will get the following output

```
Running test: sum-500
Running test: mini
Running test: simple-negate
Running test: cc
Running test: newtonraphson
Running test: newtonraphsonsqrt
Running test: SimpsonsRule
Running test: sqrtf
Running test: cordic_sin
Running test: indirect
Running test: compare-zero-self
Running test: muller
Running test: exact-ranges
Running test: redir
Running test: same-small
Running test: gromacs-bondfree-580
Running test: simpl-swallow-testd
Running test: newton
Running test: SinTaylorExpansion
Running test: gsl-modpi
Running test: multi-args
Running test: dotproduct2
Running test: small
Running test: var-test-2
Running test: gauss_pp
Running test: naturalnumber
Running test: cordic_cos
Running test: diff-roots-simple
Running test: MindlessG
Running test: simpl-swallow-testa
Running test: rootfinding1
Running test: diff-simple
Running test: tiny
Running test: some-trig
Running test: var-test
Running test: simpl-swallow-testc
Running test: dotproduct1
Running test: ContinuedFraction1
Running test: diff-roots
Running test: sum-50
Running test: onthefly
Running test: indirect1
Running test: second_order
Running test: simpl-swallow-testb
Running test: ParallelResistance
Running test: rootfinding2
Total number of benchmarks: 46(expected)
Number of benchmarks with high rounding error: 23(expected)
Number of benchmarks with NaN computation: 0(expected)
Number of benchmarks with Inf computation: 2(expected)
Number of benchmarks with branch flip: 5(expected)
Number of benchmarks with integer conversion error: 0(expected)

```



## Changes to the application to debug numerical errors

 
  If your application uses any math functions, create a mathFunc.txt
  file in the same directory and add the math functions in this file
  as shown below.

```
  $ cat mathFunc.txt 
   sqrt
   sqrtf
   cos
```

Compile the C/C++ application with EFTSanitizer pass with no compile
  time optimizations. By default we detect rounding errors and
  exceptions for function returned values with no debugging
  support. We show how to compile a simple test.c file with
  EFTSanitizer.
  
```  
  $ clang -g -O0 -fno-inline -emit-llvm -g -c -o test.c.bc test.c 
  $ opt -load $EFT_HOME/llvm_pass/build/EFTSan/libEFTSanitizer.so -eftsan < test.c.bc > test.opt.bc 
  $ llc test.opt.bc -filetype=obj -o test.c.o
  $ clang -g -O0 -g test.c.o -o test.fp.o  -O0 -g -std=c11 -L$EFT_HOME/runtime/obj 
     -leftsanitizer -lmpfr -lm  
```

  To enable debugging support compile with -eftsan-enable-debugging, 
  to enable exception detection for all FP instructions compile with -eftsan-detect-all-exceptions, 
  and to enable rounding error detection for all FP instructions compile with -eftsan-detect-all-rounding-errors
```
  $ opt -load  $EFT_HOME/llvm_pass/build/EFTSan/libEFTSanitizer.so -eftsan
    -eftsan-enable-debugging < test.c.bc > test.opt.bc 
```


## Case study to demonstrate the DAGs generated by EFTSanitizer


This case study demonstrate the ability to produce traces when there
is significant error as described in the paper. 

```
cd $EFT_HOME/case_studies
gdb ./gauss_pp_f.fp.o

```

In gdb, type the following commands in the order: (1) break 79, (2) r,
(3) c, (4) c, (5) c, (6) break eftsan_div, (7) c, and (8) call
eftsan_trace(res_)

```
(gdb) break 79

```

You will see the following output:

```
Breakpoint 1 at 0x4023e8: file gauss_pp_f.c, line 79.

(gdb) r
```

You will see the following output:

```
Breakpoint 1, MatrixSolve () at gauss_pp_f.c:79
79	    x[k] = sum / U[k][k];

(gdb) c
```

You will see following output:

```
Continuing.

Breakpoint 1, MatrixSolve () at gauss_pp_f.c:79
79	    x[k] = sum / U[k][k];
```

```
(gdb) c
```

You will see the following output:

```
Continuing.

Breakpoint 1, MatrixSolve () at gauss_pp_f.c:79
79	    x[k] = sum / U[k][k];

(gdb) c
```

You will see the following output:

```
Continuing.

Breakpoint 1, MatrixSolve () at gauss_pp_f.c:79
79	    x[k] = sum / U[k][k];

(gdb) break eftsan_div

```
You will get the following output:

```
Breakpoint 2 at 0xffff9318bc70: file handleReal.cpp, line 880.

(gdb) c

```

You will get the following output:

```
Continuing.

Breakpoint 2, eftsan_div (res=62.619915008544922, op1=1315.0181884765625, op2=21, res_=0xfffeb2c71870, op1_=0xfffeb2c71800, op2_=0xfffeb2c71838, 
    instId=265, err=2.6702880859375e-05, c_err=-316.81756874495409) at handleReal.cpp:880
880	  m_get_error(res_, res);


(gdb) call eftsan_trace(res_)
```

You will get the following output:

```
 258 FDIV  255  54 (real:: -2.541977e+02, computed: 6.261992e+01, error:-316.81756874495409 63)
 255 FSUB  249  252 (real:: -5.338151e+03, computed: 1.315018e+03, error:-6653.1689703469165 63)
 54 CONSTANT (real:: 2.100000e+01, computed: 2.100000e+01, error:0 0)
 249 FSUB  243  246 (real:: -5.336051e+03, computed: 1.317118e+03, error:-6653.1689463243565 63)
 252 FMUL  54  210 (real:: 2.100000e+00, computed: 2.100000e+00, error:5.771668550655506e-08 26)
 243 FSUB  37  240 (real:: -5.336051e+03, computed: 1.317118e+03, error:-6653.1689463243565 63)
 246 FMUL  0  220 (real:: 0.000000e+00, computed: 0.000000e+00, error:0 0)
 54 CONSTANT (real:: 2.100000e+00, computed: 2.100000e+00, error:0 0)
 210 FDIV  201  196 (real:: 9.999999e-01, computed: 9.999999e-01, error:-2.6443879891863774e-08 27)
 37 CONSTANT (real:: 1.531000e+02, computed: 1.531000e+02, error:0 0)
 240 FMUL  54  236 (real:: -7.817187e+03, computed: -1.164018e+03, error:-6653.1689768419346 53)
 0 CONSTANT (real:: 0.000000e+00, computed: 0.000000e+00, error:0 0)
 220 FDIV  217  142 (real:: 5.141541e-08, computed: 0.000000e+00, error:5.1415407887854226e-08 61)
 201 FSUB  175  199 (real:: 2.237443e-01, computed: 2.203739e-01, error:0.003370440988099591 46)
 196 FSUB  170  194 (real:: 2.237443e-01, computed: 2.203739e-01, error:0.0033704488715163893 46)
 54 CONSTANT (real:: 1.300000e+02, computed: 1.300000e+02, error:0 0)
 236 FDIV  233  49 (real:: -6.013221e+01, computed: -8.953986e+00, error:-51.178222796080725 53)
 217 FSUB  152  214 (real:: 2.522979e-04, computed: 0.000000e+00, error:0.00025229789362310288 61)
 142 FSUB  82  140 (real:: 4.907048e+03, computed: 4.832000e+03, error:75.0483729975968 46)
 175 FSUB  120  173 (real:: 2.600000e-08, computed: 2.600000e-08, error:0 0)
 199 FMUL  186  152 (real:: -2.170034e-01, computed: -2.203738e-01, error:0.0033704447904216038 46)
 170 FSUB  115  168 (real:: 9.000000e-09, computed: 9.000000e-09, error:0 0)
 194 FMUL  186  147 (real:: -2.170034e-01, computed: -2.203739e-01, error:0.0033704547726777266 46)
 233 FSUB  227  230 (real:: 2.863473e+01, computed: 4.263855e+00, error:24.370874309726073 53)
 49 FSUB  54  47 (real:: -4.761962e-01, computed: -4.761963e-01, error:9.0826125372023487e-08 30)
 152 FSUB  92  150 (real:: -6.263801e+02, computed: -6.263802e+02, error:0.00012783394883812777 30)
 214 FMUL  147  210 (real:: -6.263801e+02, computed: -6.263802e+02, error:0.00012446394478497514 30)
 82 FSUB  54  80 (real:: 3.981600e+08, computed: 3.981600e+08, error:0 0)
 140 FMUL  132  54 (real:: 3.981552e+08, computed: 3.981552e+08, error:75.0483729975968 30)
 120 FSUB  37  118 (real:: 2.600000e-08, computed: 2.600000e-08, error:0 0)
 173 FMUL  155  64 (real:: 0.000000e+00, computed: -0.000000e+00, error:0 0)
 186 FDIV  165  142 (real:: 3.464404e-04, computed: 3.518212e-04, error:-5.3807643881951617e-06 46)
 152 FSUB  92  150 (real:: -6.263801e+02, computed: -6.263802e+02, error:0.00012783394883812777 30)
 115 FSUB  54  113 (real:: 9.000000e-09, computed: 9.000000e-09, error:0 0)
 168 FMUL  155  59 (real:: 0.000000e+00, computed: -0.000000e+00, error:0 0)
 186 FDIV  165  142 (real:: 3.464404e-04, computed: 3.518212e-04, error:-5.3807643881951617e-06 46)

```


The last call generates the DAG of instructions for x[1]. Each
instruction shows the instruction identifier, computation opcode,
identifier of the left operand and right operand, real value (computed
value + error), computed value (value computed by the actual program),
and error computed using EFTs.  The first instruction in the trace
generated by above call has the identifier 258 and its operands have
identifiers 255 and 54. The second instruction has identifier 255 and
its is a FSUB instruction with operands that have identifiers 249 and
252. We show the topological sorted version of the DAG.  In summary,
this graph shows the instructions responsible for high rounding error
and shows how the error is propagated to debug the numerical error.
