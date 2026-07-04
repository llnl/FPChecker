/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* jacobi-2d.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <polybench.h>
#include "jacobi-2d.h"

static
void init_array (int n,
                 DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
                 DATA_TYPE POLYBENCH_2D(B,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      A[i][j] = ((DATA_TYPE) i*(j+2) + 2) / n;
      B[i][j] = ((DATA_TYPE) i*(j+3) + 3) / n;
    }
}

static
void init_array_long_double (int n,
                 long double POLYBENCH_2D(A,N,N,n,n),
                 long double POLYBENCH_2D(B,N,N,n,n))
{
  int i, j;
  long double n_shadow = (long double)(DATA_TYPE)n;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      A[i][j] = ((long double)(DATA_TYPE) i * (long double)(DATA_TYPE)(j+2) +
                 (long double) SCALAR_VAL(2.0)) / n_shadow;
      B[i][j] = ((long double)(DATA_TYPE) i * (long double)(DATA_TYPE)(j+3) +
                 (long double) SCALAR_VAL(3.0)) / n_shadow;
    }
}

static
void print_array(int n,
                 DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
                 long double POLYBENCH_2D(A_long_double,N,N,n,n))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  long double max_shadow = 0;
  long double sum_long_double = 0;
  long double norm_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("A");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      DATA_TYPE value = A[i][j];
      long double value_long_double = A_long_double[i][j];

      if (value < 0)
        value = -value;
      if (value_long_double < 0.0L)
        value_long_double = -value_long_double;

      if (value > max_value) {
        max_value = value;
        max_shadow = value_long_double;
      }
    }

  if (max_value != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
        DATA_TYPE scaled = A[i][j] / max_value;
        sum += scaled * scaled;
      }
    norm = SQRT_FUN(sum);
  }

  if (max_shadow != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
        long double scaled = A_long_double[i][j] / max_shadow;
        sum_long_double += scaled * scaled;
      }
    norm_long_double = sqrtl(sum_long_double);
  }

  fprintf(POLYBENCH_DUMP_TARGET, "Max value in A: %.17e\n", max_value);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of A: %.17e\n", norm);
  fprintf(POLYBENCH_DUMP_TARGET, "Max value in A_long_double: %.21Le\n", max_shadow);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of A_long_double: %.21Le\n", norm_long_double);

  long double norm_error = norm_long_double - (long double)norm;
  fprintf(POLYBENCH_DUMP_TARGET, "Norm error: %.21Le\n", norm_error);

  POLYBENCH_DUMP_END("A");
  POLYBENCH_DUMP_FINISH;
}

static
void kernel_jacobi_2d(int tsteps, int n,
                      DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
                      DATA_TYPE POLYBENCH_2D(B,N,N,n,n))
{
  int t, i, j;

#pragma scop
  for (t = 0; t < _PB_TSTEPS; t++) {
    for (i = 1; i < _PB_N - 1; i++)
      for (j = 1; j < _PB_N - 1; j++)
        B[i][j] = SCALAR_VAL(0.2) * (A[i][j] + A[i][j-1] + A[i][1+j] + A[1+i][j] + A[i-1][j]);
    for (i = 1; i < _PB_N - 1; i++)
      for (j = 1; j < _PB_N - 1; j++)
        A[i][j] = SCALAR_VAL(0.2) * (B[i][j] + B[i][j-1] + B[i][1+j] + B[1+i][j] + B[i-1][j]);
  }
#pragma endscop
}

static
void kernel_jacobi_2d_long_double(int tsteps, int n,
                      long double POLYBENCH_2D(A,N,N,n,n),
                      long double POLYBENCH_2D(B,N,N,n,n))
{
  int t, i, j;
  long double c_shadow = (long double) SCALAR_VAL(0.2);

#pragma scop
  for (t = 0; t < _PB_TSTEPS; t++) {
    for (i = 1; i < _PB_N - 1; i++)
      for (j = 1; j < _PB_N - 1; j++)
        B[i][j] = c_shadow * (A[i][j] + A[i][j-1] + A[i][1+j] + A[1+i][j] + A[i-1][j]);
    for (i = 1; i < _PB_N - 1; i++)
      for (j = 1; j < _PB_N - 1; j++)
        A[i][j] = c_shadow * (B[i][j] + B[i][j-1] + B[i][1+j] + B[1+i][j] + B[i-1][j]);
  }
#pragma endscop
}

int main(int argc, char** argv)
{
  int n = N;
  int tsteps = TSTEPS;

  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(A_long_double, long double, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(B_long_double, long double, N, N, n, n);

  init_array (n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));
  init_array_long_double (n, POLYBENCH_ARRAY(A_long_double), POLYBENCH_ARRAY(B_long_double));

  polybench_start_instruments;

  kernel_jacobi_2d(tsteps, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));
  kernel_jacobi_2d_long_double(tsteps, n, POLYBENCH_ARRAY(A_long_double), POLYBENCH_ARRAY(B_long_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_long_double)));

  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(A_long_double);
  POLYBENCH_FREE_ARRAY(B_long_double);

  return 0;
}
