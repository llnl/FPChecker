/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* covariance.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <polybench.h>
#include "covariance.h"

static
void init_array (int m, int n,
		 DATA_TYPE *float_n,
		 DATA_TYPE POLYBENCH_2D(data,N,M,n,m))
{
  int i, j;

  *float_n = (DATA_TYPE)n;

  for (i = 0; i < N; i++)
    for (j = 0; j < M; j++)
      data[i][j] = ((DATA_TYPE) i*j) / M;
}

static
void init_array_double (int m, int n,
		 double *float_n,
		 double POLYBENCH_2D(data,N,M,n,m))
{
  int i, j;

  *float_n = (double)n;

  for (i = 0; i < N; i++)
    for (j = 0; j < M; j++)
      data[i][j] = ((double) i*j) / M;
}

static
void print_array(int m,
		 DATA_TYPE POLYBENCH_2D(cov,M,M,m,m),
		 double POLYBENCH_2D(cov_double,M,M,m,m))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  double max_value_double = 0;
  double sum_double = 0;
  double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("cov");
  for (i = 0; i < m; i++)
    for (j = 0; j < m; j++) {
      DATA_TYPE value = cov[i][j];
      double value_double = cov_double[i][j];

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
    for (i = 0; i < m; i++)
      for (j = 0; j < m; j++) {
        DATA_TYPE scaled = cov[i][j] / max_value;
        sum += scaled * scaled;
      }
    norm = SQRT_FUN(sum);
  }

  if (max_value_double != 0) {
    for (i = 0; i < m; i++)
      for (j = 0; j < m; j++) {
        double scaled = cov_double[i][j] / max_value_double;
        sum_double += scaled * scaled;
      }
    norm_double = sqrt(sum_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in cov: %.7e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of cov: %.7e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in cov_double: %.17e\n", max_value_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of cov_double: %.17e\n", norm_double);

  double norm_error = norm_double - (double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.17e\n", norm_error);

  POLYBENCH_DUMP_END("cov");
  POLYBENCH_DUMP_FINISH;
}

static
void kernel_covariance(int m, int n,
		       DATA_TYPE float_n,
		       DATA_TYPE POLYBENCH_2D(data,N,M,n,m),
		       DATA_TYPE POLYBENCH_2D(cov,M,M,m,m),
		       DATA_TYPE POLYBENCH_1D(mean,M,m))
{
  int i, j, k;

#pragma scop
  for (j = 0; j < _PB_M; j++)
    {
      mean[j] = SCALAR_VAL(0.0);
      for (i = 0; i < _PB_N; i++)
        mean[j] += data[i][j];
      mean[j] /= float_n;
    }

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_M; j++)
      data[i][j] -= mean[j];

  for (i = 0; i < _PB_M; i++)
    for (j = i; j < _PB_M; j++)
      {
        cov[i][j] = SCALAR_VAL(0.0);
        for (k = 0; k < _PB_N; k++)
	  cov[i][j] += data[k][i] * data[k][j];
        cov[i][j] /= (float_n - SCALAR_VAL(1.0));
        cov[j][i] = cov[i][j];
      }
#pragma endscop
}

static
void kernel_covariance_double(int m, int n,
		       double float_n,
		       double POLYBENCH_2D(data,N,M,n,m),
		       double POLYBENCH_2D(cov,M,M,m,m),
		       double POLYBENCH_1D(mean,M,m))
{
  int i, j, k;

#pragma scop
  for (j = 0; j < _PB_M; j++)
    {
      mean[j] = 0.0;
      for (i = 0; i < _PB_N; i++)
        mean[j] += data[i][j];
      mean[j] /= float_n;
    }

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_M; j++)
      data[i][j] -= mean[j];

  for (i = 0; i < _PB_M; i++)
    for (j = i; j < _PB_M; j++)
      {
        cov[i][j] = 0.0;
        for (k = 0; k < _PB_N; k++)
	  cov[i][j] += data[k][i] * data[k][j];
        cov[i][j] /= (float_n - 1.0);
        cov[j][i] = cov[i][j];
      }
#pragma endscop
}

int main(int argc, char** argv)
{
  int n = N;
  int m = M;

  DATA_TYPE float_n;
  double float_n_double;

  POLYBENCH_2D_ARRAY_DECL(data,DATA_TYPE,N,M,n,m);
  POLYBENCH_2D_ARRAY_DECL(cov,DATA_TYPE,M,M,m,m);
  POLYBENCH_1D_ARRAY_DECL(mean,DATA_TYPE,M,m);

  POLYBENCH_2D_ARRAY_DECL(data_double,double,N,M,n,m);
  POLYBENCH_2D_ARRAY_DECL(cov_double,double,M,M,m,m);
  POLYBENCH_1D_ARRAY_DECL(mean_double,double,M,m);

  init_array (m, n, &float_n, POLYBENCH_ARRAY(data));
  init_array_double (m, n, &float_n_double, POLYBENCH_ARRAY(data_double));

  polybench_start_instruments;

  kernel_covariance (m, n, float_n,
		     POLYBENCH_ARRAY(data),
		     POLYBENCH_ARRAY(cov),
		     POLYBENCH_ARRAY(mean));
  kernel_covariance_double (m, n, float_n_double,
		     POLYBENCH_ARRAY(data_double),
		     POLYBENCH_ARRAY(cov_double),
		     POLYBENCH_ARRAY(mean_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(m, POLYBENCH_ARRAY(cov), POLYBENCH_ARRAY(cov_double)));

  POLYBENCH_FREE_ARRAY(data);
  POLYBENCH_FREE_ARRAY(cov);
  POLYBENCH_FREE_ARRAY(mean);
  POLYBENCH_FREE_ARRAY(data_double);
  POLYBENCH_FREE_ARRAY(cov_double);
  POLYBENCH_FREE_ARRAY(mean_double);

  return 0;
}
