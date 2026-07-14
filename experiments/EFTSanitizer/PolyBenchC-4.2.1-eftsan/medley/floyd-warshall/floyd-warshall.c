/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* floyd-warshall.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "floyd-warshall.h"


/* Array initialization. */
static
void init_array (int n,
		 DATA_TYPE POLYBENCH_2D(path,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      path[i][j] = (DATA_TYPE)(i*j%7+1);
      if ((i+j)%13 == 0 || (i+j)%7==0 || (i+j)%11 == 0)
         path[i][j] = 999.0f;
    }
}

static
void init_array_double (int n,
		 double POLYBENCH_2D(path,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      path[i][j] = (double)(i*j%7+1);
      if ((i+j)%13 == 0 || (i+j)%7==0 || (i+j)%11 == 0)
         path[i][j] = 999.0;
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_2D(path,N,N,n,n),
		 double POLYBENCH_2D(path_double,N,N,n,n))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  double max_value_double = 0;
  double sum_double = 0;
  double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("path");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      DATA_TYPE value = path[i][j];
      double value_double = path_double[i][j];

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
        DATA_TYPE scaled = path[i][j] / max_value;
        sum += scaled * scaled;
      }
    norm = SQRT_FUN(sum);
    norm = norm + 0;
  }

  if (max_value_double != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
        double scaled = path_double[i][j] / max_value_double;
        sum_double += scaled * scaled;
      }
    norm_double = sqrt(sum_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in path: %.7e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of path: %.7e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in path_double: %.17e\n", max_value_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of path_double: %.17e\n", norm_double);

  double norm_error = norm_double - (double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.17e\n", norm_error);

  POLYBENCH_DUMP_END("path");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_floyd_warshall(int n,
			   DATA_TYPE POLYBENCH_2D(path,N,N,n,n))
{
  int i, j, k;

#pragma scop
  for (k = 0; k < _PB_N; k++)
    {
      for(i = 0; i < _PB_N; i++)
	for (j = 0; j < _PB_N; j++)
	  path[i][j] = path[i][j] < path[i][k] + path[k][j] ?
	    path[i][j] : path[i][k] + path[k][j];
    }
#pragma endscop
}

static
void kernel_floyd_warshall_double(int n,
			   double POLYBENCH_2D(path,N,N,n,n))
{
  int i, j, k;

#pragma scop
  for (k = 0; k < _PB_N; k++)
    {
      for(i = 0; i < _PB_N; i++)
	for (j = 0; j < _PB_N; j++)
	  path[i][j] = path[i][j] < path[i][k] + path[k][j] ?
	    path[i][j] : path[i][k] + path[k][j];
    }
#pragma endscop
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  POLYBENCH_2D_ARRAY_DECL(path, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(path_double, double, N, N, n, n);

  /* Initialize array(s). */
  init_array (n, POLYBENCH_ARRAY(path));
  init_array_double (n, POLYBENCH_ARRAY(path_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_floyd_warshall (n, POLYBENCH_ARRAY(path));
  kernel_floyd_warshall_double (n, POLYBENCH_ARRAY(path_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(path), POLYBENCH_ARRAY(path_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(path);
  POLYBENCH_FREE_ARRAY(path_double);

  return 0;
}
