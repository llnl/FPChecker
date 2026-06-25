/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* jacobi-1d.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <polybench.h>
#include "jacobi-1d.h"

static
void init_array (int n,
                 DATA_TYPE POLYBENCH_1D(A,N,n),
                 DATA_TYPE POLYBENCH_1D(B,N,n))
{
  int i;

  for (i = 0; i < n; i++) {
    A[i] = ((DATA_TYPE) i + 2) / n;
    B[i] = ((DATA_TYPE) i + 3) / n;
  }
}

static
void init_array_long_double (int n,
                 long double POLYBENCH_1D(A,N,n),
                 long double POLYBENCH_1D(B,N,n))
{
  int i;

  for (i = 0; i < n; i++) {
    A[i] = ((long double) i + 2.0L) / n;
    B[i] = ((long double) i + 3.0L) / n;
  }
}

static
void print_array(int n,
                 DATA_TYPE POLYBENCH_1D(A,N,n),
                 long double POLYBENCH_1D(A_long_double,N,n))
{
  int i;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  long double max_value_long_double = 0;
  long double sum_long_double = 0;
  long double norm_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("A");
  for (i = 0; i < n; i++) {
    DATA_TYPE value = A[i];
    long double value_long_double = A_long_double[i];

    if (value < 0)
      value = -value;
    if (value_long_double < 0.0L)
      value_long_double = -value_long_double;

    if (value > max_value)
      max_value = value;
    if (value_long_double > max_value_long_double)
      max_value_long_double = value_long_double;
  }

  if (max_value != 0) {
    for (i = 0; i < n; i++) {
      DATA_TYPE scaled = A[i] / max_value;
      sum += scaled * scaled;
    }
    norm = SQRT_FUN(sum);
  }

  if (max_value_long_double != 0) {
    for (i = 0; i < n; i++) {
      long double scaled = A_long_double[i] / max_value_long_double;
      sum_long_double += scaled * scaled;
    }
    norm_long_double = sqrtl(sum_long_double);
  }

  fprintf(POLYBENCH_DUMP_TARGET, "Max value in A: %.17e\n", max_value);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of A: %.17e\n", norm);
  fprintf(POLYBENCH_DUMP_TARGET, "Max value in A_long_double: %.21Le\n", max_value_long_double);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of A_long_double: %.21Le\n", norm_long_double);

  long double norm_error = norm_long_double - (long double)norm;
  fprintf(POLYBENCH_DUMP_TARGET, "Norm error: %.21Le\n", norm_error);

  POLYBENCH_DUMP_END("A");
  POLYBENCH_DUMP_FINISH;
}

static
void kernel_jacobi_1d(int tsteps, int n,
                      DATA_TYPE POLYBENCH_1D(A,N,n),
                      DATA_TYPE POLYBENCH_1D(B,N,n))
{
  int t, i;

#pragma scop
  for (t = 0; t < _PB_TSTEPS; t++) {
    for (i = 1; i < _PB_N - 1; i++)
      B[i] = SCALAR_VAL(0.33333) * (A[i-1] + A[i] + A[i + 1]);
    for (i = 1; i < _PB_N - 1; i++)
      A[i] = SCALAR_VAL(0.33333) * (B[i-1] + B[i] + B[i + 1]);
  }
#pragma endscop
}

static
void kernel_jacobi_1d_long_double(int tsteps, int n,
                      long double POLYBENCH_1D(A,N,n),
                      long double POLYBENCH_1D(B,N,n))
{
  int t, i;

#pragma scop
  for (t = 0; t < _PB_TSTEPS; t++) {
    for (i = 1; i < _PB_N - 1; i++)
      B[i] = 0.33333L * (A[i-1] + A[i] + A[i + 1]);
    for (i = 1; i < _PB_N - 1; i++)
      A[i] = 0.33333L * (B[i-1] + B[i] + B[i + 1]);
  }
#pragma endscop
}

int main(int argc, char** argv)
{
  int n = N;
  int tsteps = TSTEPS;

  POLYBENCH_1D_ARRAY_DECL(A, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(B, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(A_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(B_long_double, long double, N, n);

  init_array (n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));
  init_array_long_double (n, POLYBENCH_ARRAY(A_long_double), POLYBENCH_ARRAY(B_long_double));

  polybench_start_instruments;

  kernel_jacobi_1d(tsteps, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));
  kernel_jacobi_1d_long_double(tsteps, n, POLYBENCH_ARRAY(A_long_double), POLYBENCH_ARRAY(B_long_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_long_double)));

  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(A_long_double);
  POLYBENCH_FREE_ARRAY(B_long_double);

  return 0;
}
