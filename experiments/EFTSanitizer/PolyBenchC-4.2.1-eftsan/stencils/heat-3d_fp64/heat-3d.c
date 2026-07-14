/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* heat-3d.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <polybench.h>
#include "heat-3d.h"

static
void init_array (int n,
                 DATA_TYPE POLYBENCH_3D(A,N,N,N,n,n,n),
                 DATA_TYPE POLYBENCH_3D(B,N,N,N,n,n,n))
{
  int i, j, k;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      for (k = 0; k < n; k++)
        A[i][j][k] = B[i][j][k] = (DATA_TYPE) (i + j + (n-k))* 10 / n;
}

static
void init_array_long_double (int n,
                 long double POLYBENCH_3D(A,N,N,N,n,n,n),
                 long double POLYBENCH_3D(B,N,N,N,n,n,n))
{
  int i, j, k;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      for (k = 0; k < n; k++)
        A[i][j][k] = B[i][j][k] = (long double) (i + j + (n-k))* 10.0L / n;
}

static
void print_array(int n,
                 DATA_TYPE POLYBENCH_3D(A,N,N,N,n,n,n),
                 long double POLYBENCH_3D(A_long_double,N,N,N,n,n,n))
{
  int i, j, k;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  long double max_value_long_double = 0;
  long double sum_long_double = 0;
  long double norm_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("A");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      for (k = 0; k < n; k++) {
        DATA_TYPE value = A[i][j][k];
        long double value_long_double = A_long_double[i][j][k];

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
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++)
        for (k = 0; k < n; k++) {
          DATA_TYPE scaled = A[i][j][k] / max_value;
          sum += scaled * scaled;
        }
    norm = SQRT_FUN(sum);
    norm = 0 + norm;
  }

  if (max_value_long_double != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++)
        for (k = 0; k < n; k++) {
          long double scaled = A_long_double[i][j][k] / max_value_long_double;
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
void kernel_heat_3d(int tsteps,
                    int n,
                    DATA_TYPE POLYBENCH_3D(A,N,N,N,n,n,n),
                    DATA_TYPE POLYBENCH_3D(B,N,N,N,n,n,n))
{
  int t, i, j, k;

#pragma scop
    for (t = 1; t <= TSTEPS; t++) {
        for (i = 1; i < _PB_N-1; i++)
            for (j = 1; j < _PB_N-1; j++)
                for (k = 1; k < _PB_N-1; k++)
                    B[i][j][k] =   SCALAR_VAL(0.125) * (A[i+1][j][k] - SCALAR_VAL(2.0) * A[i][j][k] + A[i-1][j][k])
                                 + SCALAR_VAL(0.125) * (A[i][j+1][k] - SCALAR_VAL(2.0) * A[i][j][k] + A[i][j-1][k])
                                 + SCALAR_VAL(0.125) * (A[i][j][k+1] - SCALAR_VAL(2.0) * A[i][j][k] + A[i][j][k-1])
                                 + A[i][j][k];
        for (i = 1; i < _PB_N-1; i++)
           for (j = 1; j < _PB_N-1; j++)
               for (k = 1; k < _PB_N-1; k++)
                   A[i][j][k] =   SCALAR_VAL(0.125) * (B[i+1][j][k] - SCALAR_VAL(2.0) * B[i][j][k] + B[i-1][j][k])
                                + SCALAR_VAL(0.125) * (B[i][j+1][k] - SCALAR_VAL(2.0) * B[i][j][k] + B[i][j-1][k])
                                + SCALAR_VAL(0.125) * (B[i][j][k+1] - SCALAR_VAL(2.0) * B[i][j][k] + B[i][j][k-1])
                                + B[i][j][k];
    }
#pragma endscop
}

static
void kernel_heat_3d_long_double(int tsteps,
                    int n,
                    long double POLYBENCH_3D(A,N,N,N,n,n,n),
                    long double POLYBENCH_3D(B,N,N,N,n,n,n))
{
  int t, i, j, k;

#pragma scop
    for (t = 1; t <= TSTEPS; t++) {
        for (i = 1; i < _PB_N-1; i++)
            for (j = 1; j < _PB_N-1; j++)
                for (k = 1; k < _PB_N-1; k++)
                    B[i][j][k] =   0.125L * (A[i+1][j][k] - 2.0L * A[i][j][k] + A[i-1][j][k])
                                 + 0.125L * (A[i][j+1][k] - 2.0L * A[i][j][k] + A[i][j-1][k])
                                 + 0.125L * (A[i][j][k+1] - 2.0L * A[i][j][k] + A[i][j][k-1])
                                 + A[i][j][k];
        for (i = 1; i < _PB_N-1; i++)
           for (j = 1; j < _PB_N-1; j++)
               for (k = 1; k < _PB_N-1; k++)
                   A[i][j][k] =   0.125L * (B[i+1][j][k] - 2.0L * B[i][j][k] + B[i-1][j][k])
                                + 0.125L * (B[i][j+1][k] - 2.0L * B[i][j][k] + B[i][j-1][k])
                                + 0.125L * (B[i][j][k+1] - 2.0L * B[i][j][k] + B[i][j][k-1])
                                + B[i][j][k];
    }
#pragma endscop
}

int main(int argc, char** argv)
{
  int n = N;
  int tsteps = TSTEPS;

  POLYBENCH_3D_ARRAY_DECL(A, DATA_TYPE, N, N, N, n, n, n);
  POLYBENCH_3D_ARRAY_DECL(B, DATA_TYPE, N, N, N, n, n, n);
  POLYBENCH_3D_ARRAY_DECL(A_long_double, long double, N, N, N, n, n, n);
  POLYBENCH_3D_ARRAY_DECL(B_long_double, long double, N, N, N, n, n, n);

  init_array (n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));
  init_array_long_double (n, POLYBENCH_ARRAY(A_long_double), POLYBENCH_ARRAY(B_long_double));

  polybench_start_instruments;

  kernel_heat_3d (tsteps, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));
  kernel_heat_3d_long_double (tsteps, n, POLYBENCH_ARRAY(A_long_double), POLYBENCH_ARRAY(B_long_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_long_double)));

  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(A_long_double);
  POLYBENCH_FREE_ARRAY(B_long_double);

  return 0;
}
