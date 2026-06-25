/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* seidel-2d.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <polybench.h>
#include "seidel-2d.h"

static
void init_array (int n,
                 DATA_TYPE POLYBENCH_2D(A,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      A[i][j] = ((DATA_TYPE) i*(j+2) + 2) / n;
}

static
void init_array_double (int n,
                 double POLYBENCH_2D(A,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      A[i][j] = ((double) i*(j+2) + 2.0) / n;
}

static
void print_array(int n,
                 DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
                 double POLYBENCH_2D(A_double,N,N,n,n))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  double max_value_double = 0;
  double sum_double = 0;
  double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("A");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      DATA_TYPE value = A[i][j];
      double value_double = A_double[i][j];

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
      for (j = 0; j < n; j++) {
        DATA_TYPE scaled = A[i][j] / max_value;
        sum += scaled * scaled;
      }
    norm = SQRT_FUN(sum);
  }

  if (max_value_double != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
        double scaled = A_double[i][j] / max_value_double;
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

static
void kernel_seidel_2d(int tsteps,
                      int n,
                      DATA_TYPE POLYBENCH_2D(A,N,N,n,n))
{
  int t, i, j;

#pragma scop
  for (t = 0; t <= _PB_TSTEPS - 1; t++)
    for (i = 1; i<= _PB_N - 2; i++)
      for (j = 1; j <= _PB_N - 2; j++)
        A[i][j] = (A[i-1][j-1] + A[i-1][j] + A[i-1][j+1]
                   + A[i][j-1] + A[i][j] + A[i][j+1]
                   + A[i+1][j-1] + A[i+1][j] + A[i+1][j+1])/SCALAR_VAL(9.0);
#pragma endscop
}

static
void kernel_seidel_2d_double(int tsteps,
                      int n,
                      double POLYBENCH_2D(A,N,N,n,n))
{
  int t, i, j;

#pragma scop
  for (t = 0; t <= _PB_TSTEPS - 1; t++)
    for (i = 1; i<= _PB_N - 2; i++)
      for (j = 1; j <= _PB_N - 2; j++)
        A[i][j] = (A[i-1][j-1] + A[i-1][j] + A[i-1][j+1]
                   + A[i][j-1] + A[i][j] + A[i][j+1]
                   + A[i+1][j-1] + A[i+1][j] + A[i+1][j+1])/9.0;
#pragma endscop
}

int main(int argc, char** argv)
{
  int n = N;
  int tsteps = TSTEPS;

  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(A_double, double, N, N, n, n);

  init_array (n, POLYBENCH_ARRAY(A));
  init_array_double (n, POLYBENCH_ARRAY(A_double));

  polybench_start_instruments;

  kernel_seidel_2d (tsteps, n, POLYBENCH_ARRAY(A));
  kernel_seidel_2d_double (tsteps, n, POLYBENCH_ARRAY(A_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_double)));

  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(A_double);

  return 0;
}
