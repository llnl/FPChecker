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


/* Double baseline initialization. */
static
void init_array_double (int n,
		 double *alpha,
		 double *beta,
		 double POLYBENCH_2D(A,N,N,n,n),
		 double POLYBENCH_1D(u1,N,n),
		 double POLYBENCH_1D(v1,N,n),
		 double POLYBENCH_1D(u2,N,n),
		 double POLYBENCH_1D(v2,N,n),
		 double POLYBENCH_1D(w,N,n),
		 double POLYBENCH_1D(x,N,n),
		 double POLYBENCH_1D(y,N,n),
		 double POLYBENCH_1D(z,N,n))
{
  int i, j;

  *alpha = 1.5;
  *beta = 1.2;

  double fn = (double)n;

  for (i = 0; i < n; i++)
    {
      u1[i] = (double)i;
      u2[i] = ((double)(i+1)/fn)/2.0;
      v1[i] = ((double)(i+1)/fn)/4.0;
      v2[i] = ((double)(i+1)/fn)/6.0;
      y[i] = ((double)(i+1)/fn)/8.0;
      z[i] = ((double)(i+1)/fn)/9.0;
      x[i] = 0.0;
      w[i] = 0.0;

      for (j = 0; j < n; j++)
        A[i][j] = (double) (i*j % n) / n;
    }
}


/* DCE code. Must scan the entire live-out data.
   Here we print a scalar normalized vector norm instead of the full w vector. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(w,N,n),
		 double POLYBENCH_1D(w_double,N,n))
{
  int i;

  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;

  double max_value_double = 0;
  double sum_double = 0;
  double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("w");

  for (i = 0; i < n; i++) {
    DATA_TYPE value = w[i];
    double value_double = w_double[i];

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
    for (i = 0; i < n; i++) {
      DATA_TYPE scaled = w[i] / max_value;
      sum += scaled * scaled;
    }
    norm = SQRT_FUN(sum);
  }

  if (max_value_double != 0) {
    for (i = 0; i < n; i++) {
      double scaled = w_double[i] / max_value_double;
      sum_double += scaled * scaled;
    }
    norm_double = sqrt(sum_double);
  }

  fprintf(POLYBENCH_DUMP_TARGET, "Max value in w: %.7e\n", max_value);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of w: %.7e\n", norm);
  fprintf(POLYBENCH_DUMP_TARGET, "Max value in w_double: %.17e\n", max_value_double);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of w_double: %.17e\n", norm_double);

  double norm_error = norm_double - (double)norm;
  fprintf(POLYBENCH_DUMP_TARGET, "Norm error: %.17e\n", norm_error);

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
      w[i] = w[i] + alpha * A[i][j] * x[j];

#pragma endscop
}


/* Double baseline computational kernel. */
static
void kernel_gemver_double(int n,
		   double alpha,
		   double beta,
		   double POLYBENCH_2D(A,N,N,n,n),
		   double POLYBENCH_1D(u1,N,n),
		   double POLYBENCH_1D(v1,N,n),
		   double POLYBENCH_1D(u2,N,n),
		   double POLYBENCH_1D(v2,N,n),
		   double POLYBENCH_1D(w,N,n),
		   double POLYBENCH_1D(x,N,n),
		   double POLYBENCH_1D(y,N,n),
		   double POLYBENCH_1D(z,N,n))
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
      w[i] = w[i] + alpha * A[i][j] * x[j];

#pragma endscop
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;

  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(u1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(v1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(u2, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(v2, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(w, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(z, DATA_TYPE, N, n);

  double alpha_double;
  double beta_double;

  POLYBENCH_2D_ARRAY_DECL(A_double, double, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(u1_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(v1_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(u2_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(v2_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(w_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(x_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(z_double, double, N, n);

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

  init_array_double (n, &alpha_double, &beta_double,
	      POLYBENCH_ARRAY(A_double),
	      POLYBENCH_ARRAY(u1_double),
	      POLYBENCH_ARRAY(v1_double),
	      POLYBENCH_ARRAY(u2_double),
	      POLYBENCH_ARRAY(v2_double),
	      POLYBENCH_ARRAY(w_double),
	      POLYBENCH_ARRAY(x_double),
	      POLYBENCH_ARRAY(y_double),
	      POLYBENCH_ARRAY(z_double));

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

  kernel_gemver_double (n, alpha_double, beta_double,
		 POLYBENCH_ARRAY(A_double),
		 POLYBENCH_ARRAY(u1_double),
		 POLYBENCH_ARRAY(v1_double),
		 POLYBENCH_ARRAY(u2_double),
		 POLYBENCH_ARRAY(v2_double),
		 POLYBENCH_ARRAY(w_double),
		 POLYBENCH_ARRAY(x_double),
		 POLYBENCH_ARRAY(y_double),
		 POLYBENCH_ARRAY(z_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n,
				  POLYBENCH_ARRAY(w),
				  POLYBENCH_ARRAY(w_double)));

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

  POLYBENCH_FREE_ARRAY(A_double);
  POLYBENCH_FREE_ARRAY(u1_double);
  POLYBENCH_FREE_ARRAY(v1_double);
  POLYBENCH_FREE_ARRAY(u2_double);
  POLYBENCH_FREE_ARRAY(v2_double);
  POLYBENCH_FREE_ARRAY(w_double);
  POLYBENCH_FREE_ARRAY(x_double);
  POLYBENCH_FREE_ARRAY(y_double);
  POLYBENCH_FREE_ARRAY(z_double);

  return 0;
}
