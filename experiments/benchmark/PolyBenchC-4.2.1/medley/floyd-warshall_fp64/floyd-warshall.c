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

#include <polybench.h>
#include "floyd-warshall.h"

static
void init_array (int n,
		 DATA_TYPE POLYBENCH_2D(path,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      path[i][j] = (DATA_TYPE)(i*j%7+1);
      if ((i+j)%13 == 0 || (i+j)%7==0 || (i+j)%11 == 0)
         path[i][j] = SCALAR_VAL(999.0);
    }
}

static
void init_array_long_double (int n,
		 long double POLYBENCH_2D(path,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      path[i][j] = (long double)(i*j%7+1);
      if ((i+j)%13 == 0 || (i+j)%7==0 || (i+j)%11 == 0)
         path[i][j] = 999.0L;
    }
}

static
void print_array(int n,
		 DATA_TYPE POLYBENCH_2D(path,N,N,n,n),
		 long double POLYBENCH_2D(path_long_double,N,N,n,n))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  long double max_value_long_double = 0;
  long double sum_long_double = 0;
  long double norm_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("path");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      DATA_TYPE value = path[i][j];
      long double value_long_double = path_long_double[i][j];

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
      for (j = 0; j < n; j++) {
	DATA_TYPE scaled = path[i][j] / max_value;
	sum += scaled * scaled;
      }
    norm = SQRT_FUN(sum);
  }

  if (max_value_long_double != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
	long double scaled = path_long_double[i][j] / max_value_long_double;
	sum_long_double += scaled * scaled;
      }
    norm_long_double = sqrtl(sum_long_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in path: %.17e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of path: %.17e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in path_long_double: %.21Le\n", max_value_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of path_long_double: %.21Le\n", norm_long_double);

  long double norm_error = norm_long_double - (long double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.21Le\n", norm_error);

  POLYBENCH_DUMP_END("path");
  POLYBENCH_DUMP_FINISH;
}

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
void kernel_floyd_warshall_long_double(int n,
			   long double POLYBENCH_2D(path,N,N,n,n))
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
  int n = N;

  POLYBENCH_2D_ARRAY_DECL(path, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(path_long_double, long double, N, N, n, n);

  init_array (n, POLYBENCH_ARRAY(path));
  init_array_long_double (n, POLYBENCH_ARRAY(path_long_double));

  polybench_start_instruments;

  kernel_floyd_warshall (n, POLYBENCH_ARRAY(path));
  kernel_floyd_warshall_long_double (n, POLYBENCH_ARRAY(path_long_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(path), POLYBENCH_ARRAY(path_long_double)));

  POLYBENCH_FREE_ARRAY(path);
  POLYBENCH_FREE_ARRAY(path_long_double);

  return 0;
}
