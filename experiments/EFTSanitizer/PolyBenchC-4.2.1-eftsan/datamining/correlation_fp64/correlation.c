/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* correlation.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <polybench.h>
#include "correlation.h"

static
void init_array (int m, int n,
		 DATA_TYPE *float_n,
		 DATA_TYPE POLYBENCH_2D(data,N,M,n,m))
{
  int i, j;

  *float_n = (DATA_TYPE)N;

  for (i = 0; i < N; i++)
    for (j = 0; j < M; j++)
      data[i][j] = (DATA_TYPE)(i*j)/M + i;
}

static
void init_array_long_double (int m, int n,
		 long double *float_n,
		 long double POLYBENCH_2D(data,N,M,n,m))
{
  int i, j;

  *float_n = (long double)N;

  for (i = 0; i < N; i++)
    for (j = 0; j < M; j++)
      data[i][j] = (long double)(i*j)/M + i;
}

static
void print_array(int m,
		 DATA_TYPE POLYBENCH_2D(corr,M,M,m,m),
		 long double POLYBENCH_2D(corr_long_double,M,M,m,m))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  long double max_value_long_double = 0;
  long double sum_long_double = 0;
  long double norm_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("corr");
  for (i = 0; i < m; i++)
    for (j = 0; j < m; j++) {
      DATA_TYPE value = corr[i][j];
      long double value_long_double = corr_long_double[i][j];

      if (value < 0)
	        value = -value;
      if (value_long_double < 0.0)
	        value_long_double = -value_long_double;

      if (value > max_value)
	      max_value = value;
      if (value_long_double > max_value_long_double)
	      max_value_long_double = value_long_double;
    }

  if (max_value != 0) {
    for (i = 0; i < m; i++)
      for (j = 0; j < m; j++) {
        DATA_TYPE scaled = corr[i][j] / max_value;
        sum += scaled * scaled;
      }
    norm = SQRT_FUN(sum);
    norm = 0 + norm;
  }

  if (max_value_long_double != 0) {
    for (i = 0; i < m; i++)
      for (j = 0; j < m; j++) {
        long double scaled = corr_long_double[i][j] / max_value_long_double;
        sum_long_double += scaled * scaled;
      }
    norm_long_double = sqrtl(sum_long_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in corr: %.17e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of corr: %.17e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in corr_long_double: %.21Le\n", max_value_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of corr_long_double: %.21Le\n", norm_long_double);

  long double norm_error = norm_long_double - (long double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.21Le\n", norm_error);

  POLYBENCH_DUMP_END("corr");
  POLYBENCH_DUMP_FINISH;
}

static
void kernel_correlation(int m, int n,
			DATA_TYPE float_n,
			DATA_TYPE POLYBENCH_2D(data,N,M,n,m),
			DATA_TYPE POLYBENCH_2D(corr,M,M,m,m),
			DATA_TYPE POLYBENCH_1D(mean,M,m),
			DATA_TYPE POLYBENCH_1D(stddev,M,m))
{
  int i, j, k;
  DATA_TYPE eps = SCALAR_VAL(0.1);

#pragma scop
  for (j = 0; j < _PB_M; j++)
    {
      mean[j] = SCALAR_VAL(0.0);
      for (i = 0; i < _PB_N; i++)
	    mean[j] += data[i][j];
      mean[j] /= float_n;
    }

   for (j = 0; j < _PB_M; j++)
    {
      stddev[j] = SCALAR_VAL(0.0);
      for (i = 0; i < _PB_N; i++)
        stddev[j] += (data[i][j] - mean[j]) * (data[i][j] - mean[j]);
        stddev[j] /= float_n;
        stddev[j] = SQRT_FUN(stddev[j]);
        stddev[j] = stddev[j] <= eps ? SCALAR_VAL(1.0) : stddev[j];
    }

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_M; j++)
      {
        data[i][j] -= mean[j];
        data[i][j] /= SQRT_FUN(float_n) * stddev[j];
      }

  for (i = 0; i < _PB_M-1; i++)
    {
      corr[i][i] = SCALAR_VAL(1.0);
      for (j = i+1; j < _PB_M; j++)
        {
          corr[i][j] = SCALAR_VAL(0.0);
          for (k = 0; k < _PB_N; k++)
            corr[i][j] += (data[k][i] * data[k][j]);
          corr[j][i] = corr[i][j];
        }
    }
  corr[_PB_M-1][_PB_M-1] = SCALAR_VAL(1.0);
#pragma endscop
}

static
void kernel_correlation_long_double(int m, int n,
			long double float_n,
			long double POLYBENCH_2D(data,N,M,n,m),
			long double POLYBENCH_2D(corr,M,M,m,m),
			long double POLYBENCH_1D(mean,M,m),
			long double POLYBENCH_1D(stddev,M,m))
{
  int i, j, k;
  long double eps = 0.1L;

#pragma scop
  for (j = 0; j < _PB_M; j++)
    {
      mean[j] = 0.0L;
      for (i = 0; i < _PB_N; i++)
	mean[j] += data[i][j];
      mean[j] /= float_n;
    }

   for (j = 0; j < _PB_M; j++)
    {
      stddev[j] = 0.0L;
      for (i = 0; i < _PB_N; i++)
        stddev[j] += (data[i][j] - mean[j]) * (data[i][j] - mean[j]);
      stddev[j] /= float_n;
      stddev[j] = sqrtl(stddev[j]);
      stddev[j] = stddev[j] <= eps ? 1.0L : stddev[j];
    }

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_M; j++)
      {
        data[i][j] -= mean[j];
        data[i][j] /= sqrtl(float_n) * stddev[j];
      }

  for (i = 0; i < _PB_M-1; i++)
    {
      corr[i][i] = 1.0L;
      for (j = i+1; j < _PB_M; j++)
        {
          corr[i][j] = 0.0L;
          for (k = 0; k < _PB_N; k++)
            corr[i][j] += (data[k][i] * data[k][j]);
          corr[j][i] = corr[i][j];
        }
    }
  corr[_PB_M-1][_PB_M-1] = 1.0L;
#pragma endscop
}

int main(int argc, char** argv)
{
  int n = N;
  int m = M;

  DATA_TYPE float_n;
  long double float_n_long_double;

  POLYBENCH_2D_ARRAY_DECL(data,DATA_TYPE,N,M,n,m);
  POLYBENCH_2D_ARRAY_DECL(corr,DATA_TYPE,M,M,m,m);
  POLYBENCH_1D_ARRAY_DECL(mean,DATA_TYPE,M,m);
  POLYBENCH_1D_ARRAY_DECL(stddev,DATA_TYPE,M,m);

  POLYBENCH_2D_ARRAY_DECL(data_long_double,long double,N,M,n,m);
  POLYBENCH_2D_ARRAY_DECL(corr_long_double,long double,M,M,m,m);
  POLYBENCH_1D_ARRAY_DECL(mean_long_double,long double,M,m);
  POLYBENCH_1D_ARRAY_DECL(stddev_long_double,long double,M,m);

  init_array (m, n, &float_n, POLYBENCH_ARRAY(data));
  init_array_long_double (m, n, &float_n_long_double, POLYBENCH_ARRAY(data_long_double));

  polybench_start_instruments;

  kernel_correlation (m, n, float_n,
		      POLYBENCH_ARRAY(data),
		      POLYBENCH_ARRAY(corr),
		      POLYBENCH_ARRAY(mean),
		      POLYBENCH_ARRAY(stddev));
  kernel_correlation_long_double (m, n, float_n_long_double,
		      POLYBENCH_ARRAY(data_long_double),
		      POLYBENCH_ARRAY(corr_long_double),
		      POLYBENCH_ARRAY(mean_long_double),
		      POLYBENCH_ARRAY(stddev_long_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(m, POLYBENCH_ARRAY(corr), POLYBENCH_ARRAY(corr_long_double)));

  POLYBENCH_FREE_ARRAY(data);
  POLYBENCH_FREE_ARRAY(corr);
  POLYBENCH_FREE_ARRAY(mean);
  POLYBENCH_FREE_ARRAY(stddev);
  POLYBENCH_FREE_ARRAY(data_long_double);
  POLYBENCH_FREE_ARRAY(corr_long_double);
  POLYBENCH_FREE_ARRAY(mean_long_double);
  POLYBENCH_FREE_ARRAY(stddev_long_double);

  return 0;
}
