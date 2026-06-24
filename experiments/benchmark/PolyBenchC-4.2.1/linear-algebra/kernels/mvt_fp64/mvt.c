/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* mvt.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "mvt.h"


/* Array initialization. */
static
void init_array(int n,
		DATA_TYPE POLYBENCH_1D(x1,N,n),
		DATA_TYPE POLYBENCH_1D(x2,N,n),
		DATA_TYPE POLYBENCH_1D(y_1,N,n),
		DATA_TYPE POLYBENCH_1D(y_2,N,n),
		DATA_TYPE POLYBENCH_2D(A,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    {
      x1[i] = (DATA_TYPE) (i % n) / n;
      x2[i] = (DATA_TYPE) ((i + 1) % n) / n;
      y_1[i] = (DATA_TYPE) ((i + 3) % n) / n;
      y_2[i] = (DATA_TYPE) ((i + 4) % n) / n;
      for (j = 0; j < n; j++)
	        A[i][j] = (DATA_TYPE) (i*j % n) / n;
    }
}

static
void init_array_long_double(int n,
		long double POLYBENCH_1D(x1,N,n),
		long double POLYBENCH_1D(x2,N,n),
		long double POLYBENCH_1D(y_1,N,n),
		long double POLYBENCH_1D(y_2,N,n),
		long double POLYBENCH_2D(A,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    {
      x1[i] = (long double) (i % n) / n;
      x2[i] = (long double) ((i + 1) % n) / n;
      y_1[i] = (long double) ((i + 3) % n) / n;
      y_2[i] = (long double) ((i + 4) % n) / n;
      for (j = 0; j < n; j++)
	      A[i][j] = (long double) (i*j % n) / n;
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(x1,N,n),
		 DATA_TYPE POLYBENCH_1D(x2,N,n),
		 long double POLYBENCH_1D(x1_long_double,N,n),
		 long double POLYBENCH_1D(x2_long_double,N,n))

{
  int i;

  DATA_TYPE max_value_x1 = 0;
  DATA_TYPE sum_x1 = 0;
  DATA_TYPE norm_x1 = 0;
  DATA_TYPE max_value_x2 = 0;
  DATA_TYPE sum_x2 = 0;
  DATA_TYPE norm_x2 = 0;

  long double max_value_x1_long_double = 0;
  long double sum_x1_long_double = 0;
  long double norm_x1_long_double = 0;
  long double max_value_x2_long_double = 0;
  long double sum_x2_long_double = 0;
  long double norm_x2_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("x1");
  for (i = 0; i < n; i++) {
    DATA_TYPE value_x1 = x1[i];
    DATA_TYPE value_x2 = x2[i];
    long double value_x1_long_double = x1_long_double[i];
    long double value_x2_long_double = x2_long_double[i];

    if (value_x1 < 0)
      value_x1 = -value_x1;
    if (value_x2 < 0)
      value_x2 = -value_x2;
    if (value_x1_long_double < 0.0)
      value_x1_long_double = -value_x1_long_double;
    if (value_x2_long_double < 0.0)
      value_x2_long_double = -value_x2_long_double;

    if (value_x1 > max_value_x1)
      max_value_x1 = value_x1;
    if (value_x2 > max_value_x2)
      max_value_x2 = value_x2;
    if (value_x1_long_double > max_value_x1_long_double)
      max_value_x1_long_double = value_x1_long_double;
    if (value_x2_long_double > max_value_x2_long_double)
      max_value_x2_long_double = value_x2_long_double;
  }

  if (max_value_x1 != 0) {
    for (i = 0; i < n; i++) {
      DATA_TYPE scaled = x1[i] / max_value_x1;
      sum_x1 += scaled * scaled;
    }
    norm_x1 = SQRT_FUN(sum_x1);
  }

  if (max_value_x2 != 0) {
    for (i = 0; i < n; i++) {
      DATA_TYPE scaled = x2[i] / max_value_x2;
      sum_x2 += scaled * scaled;
    }
    norm_x2 = SQRT_FUN(sum_x2);
  }

  if (max_value_x1_long_double != 0) {
    for (i = 0; i < n; i++) {
      long double scaled = x1_long_double[i] / max_value_x1_long_double;
      sum_x1_long_double += scaled * scaled;
    }
    norm_x1_long_double = sqrtl(sum_x1_long_double);
  }

  if (max_value_x2_long_double != 0) {
    for (i = 0; i < n; i++) {
      long double scaled = x2_long_double[i] / max_value_x2_long_double;
      sum_x2_long_double += scaled * scaled;
    }
    norm_x2_long_double = sqrtl(sum_x2_long_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in x1: %.17e\n", max_value_x1);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of x1: %.17e\n", norm_x1);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in x1_long_double: %.21Le\n", max_value_x1_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of x1_long_double: %.21Le\n", norm_x1_long_double);

  long double norm_error_x1 = norm_x1_long_double - (long double)norm_x1;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error x1: %.21Le\n", norm_error_x1);

  POLYBENCH_DUMP_END("x1");

  POLYBENCH_DUMP_BEGIN("x2");
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in x2: %.17e\n", max_value_x2);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of x2: %.17e\n", norm_x2);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in x2_long_double: %.21Le\n", max_value_x2_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of x2_long_double: %.21Le\n", norm_x2_long_double);

  long double norm_error_x2 = norm_x2_long_double - (long double)norm_x2;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error x2: %.21Le\n", norm_error_x2);

  POLYBENCH_DUMP_END("x2");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_mvt(int n,
		DATA_TYPE POLYBENCH_1D(x1,N,n),
		DATA_TYPE POLYBENCH_1D(x2,N,n),
		DATA_TYPE POLYBENCH_1D(y_1,N,n),
		DATA_TYPE POLYBENCH_1D(y_2,N,n),
		DATA_TYPE POLYBENCH_2D(A,N,N,n,n))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_N; j++)
      x1[i] = x1[i] + A[i][j] * y_1[j];
  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_N; j++)
      x2[i] = x2[i] + A[j][i] * y_2[j];
#pragma endscop

}

static
void kernel_mvt_long_double(int n,
		long double POLYBENCH_1D(x1,N,n),
		long double POLYBENCH_1D(x2,N,n),
		long double POLYBENCH_1D(y_1,N,n),
		long double POLYBENCH_1D(y_2,N,n),
		long double POLYBENCH_2D(A,N,N,n,n))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_N; j++)
      x1[i] = x1[i] + A[i][j] * y_1[j];
  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_N; j++)
      x2[i] = x2[i] + A[j][i] * y_2[j];
#pragma endscop

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(x1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(x2, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_2, DATA_TYPE, N, n);

  POLYBENCH_2D_ARRAY_DECL(A_long_double, long double, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(x1_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(x2_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_1_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_2_long_double, long double, N, n);

  /* Initialize array(s). */
  init_array (n,
	      POLYBENCH_ARRAY(x1),
	      POLYBENCH_ARRAY(x2),
	      POLYBENCH_ARRAY(y_1),
	      POLYBENCH_ARRAY(y_2),
	      POLYBENCH_ARRAY(A));

  init_array_long_double (n,
	      POLYBENCH_ARRAY(x1_long_double),
	      POLYBENCH_ARRAY(x2_long_double),
	      POLYBENCH_ARRAY(y_1_long_double),
	      POLYBENCH_ARRAY(y_2_long_double),
	      POLYBENCH_ARRAY(A_long_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_mvt (n,
	      POLYBENCH_ARRAY(x1),
	      POLYBENCH_ARRAY(x2),
	      POLYBENCH_ARRAY(y_1),
	      POLYBENCH_ARRAY(y_2),
	      POLYBENCH_ARRAY(A));

  kernel_mvt_long_double (n,
	      POLYBENCH_ARRAY(x1_long_double),
	      POLYBENCH_ARRAY(x2_long_double),
	      POLYBENCH_ARRAY(y_1_long_double),
	      POLYBENCH_ARRAY(y_2_long_double),
	      POLYBENCH_ARRAY(A_long_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(x1), POLYBENCH_ARRAY(x2),
				    POLYBENCH_ARRAY(x1_long_double), POLYBENCH_ARRAY(x2_long_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(x1);
  POLYBENCH_FREE_ARRAY(x2);
  POLYBENCH_FREE_ARRAY(y_1);
  POLYBENCH_FREE_ARRAY(y_2);
  POLYBENCH_FREE_ARRAY(A_long_double);
  POLYBENCH_FREE_ARRAY(x1_long_double);
  POLYBENCH_FREE_ARRAY(x2_long_double);
  POLYBENCH_FREE_ARRAY(y_1_long_double);
  POLYBENCH_FREE_ARRAY(y_2_long_double);

  return 0;
}
