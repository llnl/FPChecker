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

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "heat-3d.h"


/* Array initialization. */
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
void init_array_double (int n,
                 double POLYBENCH_3D(A,N,N,N,n,n,n),
                 double POLYBENCH_3D(B,N,N,N,n,n,n))
{
  int i, j, k;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      for (k = 0; k < n; k++)
        A[i][j][k] = B[i][j][k] = (double) (i + j + (n-k))* 10.0 / n;
}

static
void print_array(int n,
                 DATA_TYPE POLYBENCH_3D(A,N,N,N,n,n,n),
                 double POLYBENCH_3D(A_double,N,N,N,n,n,n))
{
  int i, j, k;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  double max_value_double = 0;
  double sum_double = 0;
  double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("A");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      for (k = 0; k < n; k++) {
        DATA_TYPE value = A[i][j][k];
        double value_double = A_double[i][j][k];

        if (value < 0)
          value = -value;
        if (value_double < 0.0)
          value_double = -value_double;

        if (value > max_value)
          max_value = value;
        if (value_double > max_value_double)
          max_value_double = value_double;
      }

  if (max_value != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++)
        for (k = 0; k < n; k++) {
          DATA_TYPE scaled = A[i][j][k] / max_value;
          sum += scaled * scaled;
        }
    norm = SQRT_FUN(sum);
  }

  if (max_value_double != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++)
        for (k = 0; k < n; k++) {
          double scaled = A_double[i][j][k] / max_value_double;
          sum_double += scaled * scaled;
        }
    norm_double = sqrt(sum_double);
  }

  fprintf(POLYBENCH_DUMP_TARGET, "Max value in A: %.7e\n", max_value);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of A: %.7e\n", norm);
  fprintf(POLYBENCH_DUMP_TARGET, "Max value in A_double: %.17e\n", max_value_double);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of A_double: %.17e\n", norm_double);

  double norm_error = norm_double - (double)norm;
  fprintf(POLYBENCH_DUMP_TARGET, "Norm error: %.17e\n", norm_error);

  POLYBENCH_DUMP_END("A");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
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
void kernel_heat_3d_double(int tsteps,
                    int n,
                    double POLYBENCH_3D(A,N,N,N,n,n,n),
                    double POLYBENCH_3D(B,N,N,N,n,n,n))
{
  int t, i, j, k;

#pragma scop
    for (t = 1; t <= TSTEPS; t++) {
        for (i = 1; i < _PB_N-1; i++)
            for (j = 1; j < _PB_N-1; j++)
                for (k = 1; k < _PB_N-1; k++)
                    B[i][j][k] =   0.125 * (A[i+1][j][k] - 2.0 * A[i][j][k] + A[i-1][j][k])
                                 + 0.125 * (A[i][j+1][k] - 2.0 * A[i][j][k] + A[i][j-1][k])
                                 + 0.125 * (A[i][j][k+1] - 2.0 * A[i][j][k] + A[i][j][k-1])
                                 + A[i][j][k];
        for (i = 1; i < _PB_N-1; i++)
           for (j = 1; j < _PB_N-1; j++)
               for (k = 1; k < _PB_N-1; k++)
                   A[i][j][k] =   0.125 * (B[i+1][j][k] - 2.0 * B[i][j][k] + B[i-1][j][k])
                                + 0.125 * (B[i][j+1][k] - 2.0 * B[i][j][k] + B[i][j-1][k])
                                + 0.125 * (B[i][j][k+1] - 2.0 * B[i][j][k] + B[i][j][k-1])
                                + B[i][j][k];
    }
#pragma endscop

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;
  int tsteps = TSTEPS;

  /* Variable declaration/allocation. */
  POLYBENCH_3D_ARRAY_DECL(A, DATA_TYPE, N, N, N, n, n, n);
  POLYBENCH_3D_ARRAY_DECL(B, DATA_TYPE, N, N, N, n, n, n);
  POLYBENCH_3D_ARRAY_DECL(A_double, double, N, N, N, n, n, n);
  POLYBENCH_3D_ARRAY_DECL(B_double, double, N, N, N, n, n, n);

  /* Initialize array(s). */
  init_array (n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));
  init_array_double (n, POLYBENCH_ARRAY(A_double), POLYBENCH_ARRAY(B_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_heat_3d (tsteps, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B));
  kernel_heat_3d_double (tsteps, n, POLYBENCH_ARRAY(A_double), POLYBENCH_ARRAY(B_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(A_double);
  POLYBENCH_FREE_ARRAY(B_double);

  return 0;
}
