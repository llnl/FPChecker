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
void init_array_long_double(int n,
		long double *alpha,
		long double *beta,
		long double POLYBENCH_2D(A,N,N,n,n),
		long double POLYBENCH_2D(B,N,N,n,n),
		long double POLYBENCH_1D(x,N,n))
{
  int i, j;

  *alpha = 1.5L;
  *beta = 1.2L;
  for (i = 0; i < n; i++)
    {
      x[i] = (long double)( i % n) / n;
      for (j = 0; j < n; j++) {
        A[i][j] = (long double) ((i*j+1) % n) / n;
        B[i][j] = (long double) ((i*j+2) % n) / n;
      }
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(y,N,n),
		 long double POLYBENCH_1D(y_long_double,N,n))

{
  int i;

  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;

  long double max_value_long_double = 0;
  long double sum_long_double = 0;
  long double norm_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("y");
  for (i = 0; i < n; i++) {
    DATA_TYPE value = y[i];
    long double value_long_double = y_long_double[i];

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
      DATA_TYPE scaled = y[i] / max_value;
      sum += scaled * scaled;
    }
    norm = SQRT_FUN(sum);
  }

  if (max_value_long_double != 0) {
    for (i = 0; i < n; i++) {
      long double scaled = y_long_double[i] / max_value_long_double;
      sum_long_double += scaled * scaled;
    }
    norm_long_double = sqrtl(sum_long_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in y: %.17e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of y: %.17e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in long_double: %.21Le\n", max_value_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of long_double: %.21Le\n", norm_long_double);

  long double norm_error = norm_long_double - (long double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.21Le\n", norm_error);

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
void kernel_gesummv_long_double(int n,
		    long double alpha,
		    long double beta,
		    long double POLYBENCH_2D(A,N,N,n,n),
		    long double POLYBENCH_2D(B,N,N,n,n),
		    long double POLYBENCH_1D(tmp,N,n),
		    long double POLYBENCH_1D(x,N,n),
		    long double POLYBENCH_1D(y,N,n))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_N; i++)
    {
      tmp[i] = 0.0L;
      y[i] = 0.0L;
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
  long double alpha_long_double;
  long double beta_long_double;

  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(tmp, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y, DATA_TYPE, N, n);

  POLYBENCH_2D_ARRAY_DECL(A_long_double, long double, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(B_long_double, long double, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(tmp_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(x_long_double, long double, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_long_double, long double, N, n);

  /* Initialize array(s). */
  init_array (n, &alpha, &beta,
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(B),
	      POLYBENCH_ARRAY(x));
  init_array_long_double (n, &alpha_long_double, &beta_long_double,
	      POLYBENCH_ARRAY(A_long_double),
	      POLYBENCH_ARRAY(B_long_double),
	      POLYBENCH_ARRAY(x_long_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gesummv (n, alpha, beta,
		  POLYBENCH_ARRAY(A),
		  POLYBENCH_ARRAY(B),
		  POLYBENCH_ARRAY(tmp),
		  POLYBENCH_ARRAY(x),
		  POLYBENCH_ARRAY(y));
  kernel_gesummv_long_double (n, alpha_long_double, beta_long_double,
		  POLYBENCH_ARRAY(A_long_double),
		  POLYBENCH_ARRAY(B_long_double),
		  POLYBENCH_ARRAY(tmp_long_double),
		  POLYBENCH_ARRAY(x_long_double),
		  POLYBENCH_ARRAY(y_long_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(y), POLYBENCH_ARRAY(y_long_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(tmp);
  POLYBENCH_FREE_ARRAY(x);
  POLYBENCH_FREE_ARRAY(y);
  POLYBENCH_FREE_ARRAY(A_long_double);
  POLYBENCH_FREE_ARRAY(B_long_double);
  POLYBENCH_FREE_ARRAY(tmp_long_double);
  POLYBENCH_FREE_ARRAY(x_long_double);
  POLYBENCH_FREE_ARRAY(y_long_double);

  return 0;
}
