/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* gesummv.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "gesummv.h"


/* Array initialization. */
static
void init_array(int n,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(B,N,N,n,n),
		DATA_TYPE POLYBENCH_1D(x,N,n))
{
  int i, j;

  *alpha = 1.5;
  *beta = 1.2;
  for (i = 0; i < n; i++)
    {
      x[i] = (DATA_TYPE)( i % n) / n;
      for (j = 0; j < n; j++) {
	A[i][j] = (DATA_TYPE) ((i*j+1) % n) / n;
	B[i][j] = (DATA_TYPE) ((i*j+2) % n) / n;
      }
    }
}

static
void init_array_double(int n,
		double *alpha,
		double *beta,
		double POLYBENCH_2D(A,N,N,n,n),
		double POLYBENCH_2D(B,N,N,n,n),
		double POLYBENCH_1D(x,N,n))
{
  int i, j;

  *alpha = 1.5;
  *beta = 1.2;
  for (i = 0; i < n; i++)
    {
      x[i] = (double)( i % n) / n;
      for (j = 0; j < n; j++) {
	A[i][j] = (double) ((i*j+1) % n) / n;
	B[i][j] = (double) ((i*j+2) % n) / n;
      }
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(y,N,n),
		 double POLYBENCH_1D(y_double,N,n))

{
  int i;

  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;

  double max_value_double = 0;
  double sum_double = 0;
  double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("y");
  for (i = 0; i < n; i++) {
    DATA_TYPE value = y[i];
    double value_double = y_double[i];

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
      DATA_TYPE scaled = y[i] / max_value;
      sum += scaled * scaled;
    }
    norm = SQRT_FUN(sum);
  }

  if (max_value_double != 0) {
    for (i = 0; i < n; i++) {
      double scaled = y_double[i] / max_value_double;
      sum_double += scaled * scaled;
    }
    norm_double = sqrt(sum_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in y: %.7e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of y: %.7e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in y_double: %.17e\n", max_value_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of y_double: %.17e\n", norm_double);

  double norm_error = norm_double - (double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.17e\n", norm_error);

  POLYBENCH_DUMP_END("y");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_gesummv(int n,
		    DATA_TYPE alpha,
		    DATA_TYPE beta,
		    DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
		    DATA_TYPE POLYBENCH_2D(B,N,N,n,n),
		    DATA_TYPE POLYBENCH_1D(tmp,N,n),
		    DATA_TYPE POLYBENCH_1D(x,N,n),
		    DATA_TYPE POLYBENCH_1D(y,N,n))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_N; i++)
    {
      tmp[i] = SCALAR_VAL(0.0);
      y[i] = SCALAR_VAL(0.0);
      for (j = 0; j < _PB_N; j++)
	  {
	  tmp[i] = A[i][j] * x[j] + tmp[i];
	  y[i] = B[i][j] * x[j] + y[i];
	  }
      y[i] = alpha * tmp[i] + beta * y[i];
    }
#pragma endscop

}

static
void kernel_gesummv_double(int n,
		    double alpha,
		    double beta,
		    double POLYBENCH_2D(A,N,N,n,n),
		    double POLYBENCH_2D(B,N,N,n,n),
		    double POLYBENCH_1D(tmp,N,n),
		    double POLYBENCH_1D(x,N,n),
		    double POLYBENCH_1D(y,N,n))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_N; i++)
    {
      tmp[i] = 0.0;
      y[i] = 0.0;
      for (j = 0; j < _PB_N; j++)
	{
	  tmp[i] = A[i][j] * x[j] + tmp[i];
	  y[i] = B[i][j] * x[j] + y[i];
	}
      y[i] = alpha * tmp[i] + beta * y[i];
    }
#pragma endscop

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  double alpha_double;
  double beta_double;

  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(tmp, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y, DATA_TYPE, N, n);

  POLYBENCH_2D_ARRAY_DECL(A_double, double, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(B_double, double, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(tmp_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(x_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_double, double, N, n);

  /* Initialize array(s). */
  init_array (n, &alpha, &beta,
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(B),
	      POLYBENCH_ARRAY(x));

  init_array_double (n, &alpha_double, &beta_double,
	      POLYBENCH_ARRAY(A_double),
	      POLYBENCH_ARRAY(B_double),
	      POLYBENCH_ARRAY(x_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gesummv (n, alpha, beta,
		  POLYBENCH_ARRAY(A),
		  POLYBENCH_ARRAY(B),
		  POLYBENCH_ARRAY(tmp),
		  POLYBENCH_ARRAY(x),
		  POLYBENCH_ARRAY(y));

  kernel_gesummv_double (n, alpha_double, beta_double,
		  POLYBENCH_ARRAY(A_double),
		  POLYBENCH_ARRAY(B_double),
		  POLYBENCH_ARRAY(tmp_double),
		  POLYBENCH_ARRAY(x_double),
		  POLYBENCH_ARRAY(y_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(y), POLYBENCH_ARRAY(y_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(tmp);
  POLYBENCH_FREE_ARRAY(x);
  POLYBENCH_FREE_ARRAY(y);
  POLYBENCH_FREE_ARRAY(A_double);
  POLYBENCH_FREE_ARRAY(B_double);
  POLYBENCH_FREE_ARRAY(tmp_double);
  POLYBENCH_FREE_ARRAY(x_double);
  POLYBENCH_FREE_ARRAY(y_double);

  return 0;
}
