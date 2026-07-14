/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* trisolv.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "trisolv.h"


/* Array initialization. */
static
void init_array(int n,
		DATA_TYPE POLYBENCH_2D(L,N,N,n,n),
		DATA_TYPE POLYBENCH_1D(x,N,n),
		DATA_TYPE POLYBENCH_1D(b,N,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    {
      x[i] = - 999;
      b[i] =  i ;
      for (j = 0; j <= i; j++)
	L[i][j] = (DATA_TYPE) (i+n-j+1)*2/n;
    }
}

static
void init_array_double(int n,
		double POLYBENCH_2D(L,N,N,n,n),
		double POLYBENCH_1D(x,N,n),
		double POLYBENCH_1D(b,N,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    {
      x[i] = - 999;
      b[i] =  i ;
      for (j = 0; j <= i; j++)
	L[i][j] = (double) (i+n-j+1)*2/n;
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(x,N,n),
		 double POLYBENCH_1D(x_double,N,n))

{
  int i;

  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;

  double max_value_double = 0;
  double sum_double = 0;
  double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("x");
  for (i = 0; i < n; i++) {
    DATA_TYPE value = x[i];
    double value_double = x_double[i];

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
      DATA_TYPE scaled = x[i] / max_value;
      sum += scaled * scaled;
    }
    norm = SQRT_FUN(sum);
    norm = 0 + norm;
  }

  if (max_value_double != 0) {
    for (i = 0; i < n; i++) {
      double scaled = x_double[i] / max_value_double;
      sum_double += scaled * scaled;
    }
    norm_double = sqrt(sum_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in x: %.7e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of x: %.7e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in x_double: %.17e\n", max_value_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of x_double: %.17e\n", norm_double);

  double norm_error = norm_double - (double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.17e\n", norm_error);

  POLYBENCH_DUMP_END("x");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_trisolv(int n,
		    DATA_TYPE POLYBENCH_2D(L,N,N,n,n),
		    DATA_TYPE POLYBENCH_1D(x,N,n),
		    DATA_TYPE POLYBENCH_1D(b,N,n))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_N; i++)
    {
      x[i] = b[i];
      for (j = 0; j <i; j++)
        x[i] -= L[i][j] * x[j];
      x[i] = x[i] / L[i][i];
    }
#pragma endscop

}

static
void kernel_trisolv_double(int n,
		    double POLYBENCH_2D(L,N,N,n,n),
		    double POLYBENCH_1D(x,N,n),
		    double POLYBENCH_1D(b,N,n))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_N; i++)
    {
      x[i] = b[i];
      for (j = 0; j <i; j++)
        x[i] -= L[i][j] * x[j];
      x[i] = x[i] / L[i][i];
    }
#pragma endscop

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  POLYBENCH_2D_ARRAY_DECL(L, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(b, DATA_TYPE, N, n);

  POLYBENCH_2D_ARRAY_DECL(L_double, double, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL(x_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(b_double, double, N, n);

  /* Initialize array(s). */
  init_array (n, POLYBENCH_ARRAY(L), POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(b));
  init_array_double (n, POLYBENCH_ARRAY(L_double), POLYBENCH_ARRAY(x_double), POLYBENCH_ARRAY(b_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_trisolv (n, POLYBENCH_ARRAY(L), POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(b));
  kernel_trisolv_double (n, POLYBENCH_ARRAY(L_double), POLYBENCH_ARRAY(x_double), POLYBENCH_ARRAY(b_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(x), POLYBENCH_ARRAY(x_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(L);
  POLYBENCH_FREE_ARRAY(x);
  POLYBENCH_FREE_ARRAY(b);
  POLYBENCH_FREE_ARRAY(L_double);
  POLYBENCH_FREE_ARRAY(x_double);
  POLYBENCH_FREE_ARRAY(b_double);

  return 0;
}
