/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* gemver.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "gemver.h"


/* Array initialization. */
static
void init_array (int n,
		 DATA_TYPE *alpha,
		 DATA_TYPE *beta,
		 DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
		 DATA_TYPE POLYBENCH_1D(u1,N,n),
		 DATA_TYPE POLYBENCH_1D(v1,N,n),
		 DATA_TYPE POLYBENCH_1D(u2,N,n),
		 DATA_TYPE POLYBENCH_1D(v2,N,n),
		 DATA_TYPE POLYBENCH_1D(w,N,n),
		 DATA_TYPE POLYBENCH_1D(x,N,n),
		 DATA_TYPE POLYBENCH_1D(y,N,n),
		 DATA_TYPE POLYBENCH_1D(z,N,n))
{
  int i, j;

  *alpha = 1.5;
  *beta = 1.2;

  DATA_TYPE fn = (DATA_TYPE)n;

  for (i = 0; i < n; i++)
    {
      u1[i] = i;
      u2[i] = ((i+1)/fn)/2.0;
      v1[i] = ((i+1)/fn)/4.0;
      v2[i] = ((i+1)/fn)/6.0;
      y[i] = ((i+1)/fn)/8.0;
      z[i] = ((i+1)/fn)/9.0;
      x[i] = 0.0;
      w[i] = 0.0;
      for (j = 0; j < n; j++)
        A[i][j] = (DATA_TYPE) (i*j % n) / n;
    }
}

static
void init_array_long_double (int n,
		 long double *alpha,
		 long double *beta,
		 long double POLYBENCH_2D(A,N,N,n,n),
		 long double POLYBENCH_1D(u1,N,n),
		 long double POLYBENCH_1D(v1,N,n),
		 long double POLYBENCH_1D(u2,N,n),
		 long double POLYBENCH_1D(v2,N,n),
		 long double POLYBENCH_1D(w,N,n),
		 long double POLYBENCH_1D(x,N,n),
		 long double POLYBENCH_1D(y,N,n),
		 long double POLYBENCH_1D(z,N,n))
{
  int i, j;

  *alpha = 1.5L;
  *beta = 1.2L;

  long double fn = (long double)n;

  for (i = 0; i < n; i++)
    {
      u1[i] = i;
      u2[i] = ((i+1)/fn)/2.0L;
      v1[i] = ((i+1)/fn)/4.0L;
      v2[i] = ((i+1)/fn)/6.0L;
      y[i] = ((i+1)/fn)/8.0L;
      z[i] = ((i+1)/fn)/9.0L;
      x[i] = 0.0L;
      w[i] = 0.0L;
      for (j = 0; j < n; j++)
        A[i][j] = (long double) (i*j % n) / n;
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(w,N,n),
		 long double POLYBENCH_1D(w_long_double,N,n))
{
  int i;

  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;

  long double max_value_long_double = 0;
  long double sum_long_double = 0;
  long double norm_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("w");
  for (i = 0; i < n; i++) {
    DATA_TYPE value = w[i];
    long double value_long_double = w_long_double[i];

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
      DATA_TYPE scaled = w[i] / max_value;
      sum += scaled * scaled;
    }
    norm = SQRT_FUN(sum);
  }

  if (max_value_long_double != 0) {
    for (i = 0; i < n; i++) {
      long double scaled = w_long_double[i] / max_value_long_double;
      sum_long_double += scaled * scaled;
    }
    norm_long_double = sqrtl(sum_long_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in w: %.17e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of w: %.17e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in long_double: %.21Le\n", max_value_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of long_double: %.21Le\n", norm_long_double);

  long double norm_error = norm_long_double - (long double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.21Le\n", norm_error);

  POLYBENCH_DUMP_END("w");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_gemver(int n,
		   DATA_TYPE alpha,
		   DATA_TYPE beta,
		   DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
		   DATA_TYPE POLYBENCH_1D(u1,N,n),
		   DATA_TYPE POLYBENCH_1D(v1,N,n),
		   DATA_TYPE POLYBENCH_1D(u2,N,n),
		   DATA_TYPE POLYBENCH_1D(v2,N,n),
		   DATA_TYPE POLYBENCH_1D(w,N,n),
		   DATA_TYPE POLYBENCH_1D(x,N,n),
		   DATA_TYPE POLYBENCH_1D(y,N,n),
		   DATA_TYPE POLYBENCH_1D(z,N,n))
{
  int i, j;

#pragma scop

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_N; j++)
      A[i][j] = A[i][j] + u1[i] * v1[j] + u2[i] * v2[j];

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_N; j++)
      x[i] = x[i] + beta * A[j][i] * y[j];

  for (i = 0; i < _PB_N; i++)
    x[i] = x[i] + z[i];

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_N; j++)
      w[i] = w[i] +  alpha * A[i][j] * x[j];

#pragma endscop
}

static
void kernel_gemver_long_double(int n,
		   long double alpha,
		   long double beta,
		   long double POLYBENCH_2D(A,N,N,n,n),
		   long double POLYBENCH_1D(u1,N,n),
		   long double POLYBENCH_1D(v1,N,n),
		   long double POLYBENCH_1D(u2,N,n),
		   long double POLYBENCH_1D(v2,N,n),
		   long double POLYBENCH_1D(w,N,n),
		   long double POLYBENCH_1D(x,N,n),
		   long double POLYBENCH_1D(y,N,n),
		   long double POLYBENCH_1D(z,N,n))
{
  int i, j;

#pragma scop

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_N; j++)
      A[i][j] = A[i][j] + u1[i] * v1[j] + u2[i] * v2[j];

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_N; j++)
      x[i] = x[i] + beta * A[j][i] * y[j];

  for (i = 0; i < _PB_N; i++)
    x[i] = x[i] + z[i];

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_N; j++)
      w[i] = w[i] +  alpha * A[i][j] * x[j];

#pragma endscop
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  long double alpha_long_double;
  long double beta_long_double;

  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(u1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(v1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(u2, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(v2, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(w, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(z, DATA_TYPE, N, n);

  POLYBENCH_2D_ARRAY_DECL(A_long_double, long double, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(u1_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(v1_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(u2_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(v2_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(w_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(x_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(z_long_double, long double, N, n);

  /* Initialize array(s). */
  init_array (n, &alpha, &beta,
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(u1),
	      POLYBENCH_ARRAY(v1),
	      POLYBENCH_ARRAY(u2),
	      POLYBENCH_ARRAY(v2),
	      POLYBENCH_ARRAY(w),
	      POLYBENCH_ARRAY(x),
	      POLYBENCH_ARRAY(y),
	      POLYBENCH_ARRAY(z));
  init_array_long_double (n, &alpha_long_double, &beta_long_double,
	      POLYBENCH_ARRAY(A_long_double),
	      POLYBENCH_ARRAY(u1_long_double),
	      POLYBENCH_ARRAY(v1_long_double),
	      POLYBENCH_ARRAY(u2_long_double),
	      POLYBENCH_ARRAY(v2_long_double),
	      POLYBENCH_ARRAY(w_long_double),
	      POLYBENCH_ARRAY(x_long_double),
	      POLYBENCH_ARRAY(y_long_double),
	      POLYBENCH_ARRAY(z_long_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gemver (n, alpha, beta,
		 POLYBENCH_ARRAY(A),
		 POLYBENCH_ARRAY(u1),
		 POLYBENCH_ARRAY(v1),
		 POLYBENCH_ARRAY(u2),
		 POLYBENCH_ARRAY(v2),
		 POLYBENCH_ARRAY(w),
		 POLYBENCH_ARRAY(x),
		 POLYBENCH_ARRAY(y),
		 POLYBENCH_ARRAY(z));
  kernel_gemver_long_double (n, alpha_long_double, beta_long_double,
		 POLYBENCH_ARRAY(A_long_double),
		 POLYBENCH_ARRAY(u1_long_double),
		 POLYBENCH_ARRAY(v1_long_double),
		 POLYBENCH_ARRAY(u2_long_double),
		 POLYBENCH_ARRAY(v2_long_double),
		 POLYBENCH_ARRAY(w_long_double),
		 POLYBENCH_ARRAY(x_long_double),
		 POLYBENCH_ARRAY(y_long_double),
		 POLYBENCH_ARRAY(z_long_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(w), POLYBENCH_ARRAY(w_long_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(u1);
  POLYBENCH_FREE_ARRAY(v1);
  POLYBENCH_FREE_ARRAY(u2);
  POLYBENCH_FREE_ARRAY(v2);
  POLYBENCH_FREE_ARRAY(w);
  POLYBENCH_FREE_ARRAY(x);
  POLYBENCH_FREE_ARRAY(y);
  POLYBENCH_FREE_ARRAY(z);
  POLYBENCH_FREE_ARRAY(A_long_double);
  POLYBENCH_FREE_ARRAY(u1_long_double);
  POLYBENCH_FREE_ARRAY(v1_long_double);
  POLYBENCH_FREE_ARRAY(u2_long_double);
  POLYBENCH_FREE_ARRAY(v2_long_double);
  POLYBENCH_FREE_ARRAY(w_long_double);
  POLYBENCH_FREE_ARRAY(x_long_double);
  POLYBENCH_FREE_ARRAY(y_long_double);
  POLYBENCH_FREE_ARRAY(z_long_double);

  return 0;
}
