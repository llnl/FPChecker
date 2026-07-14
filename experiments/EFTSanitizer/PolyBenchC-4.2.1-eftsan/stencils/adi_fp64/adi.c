/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* adi.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <polybench.h>
#include "adi.h"

static
void init_array (int n,
                 DATA_TYPE POLYBENCH_2D(u,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      u[i][j] =  (DATA_TYPE)(i + n-j) / n;
}

static
void init_array_long_double (int n,
                 long double POLYBENCH_2D(u,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      u[i][j] =  (long double)(i + n-j) / n;
}

static
void print_array(int n,
                 DATA_TYPE POLYBENCH_2D(u,N,N,n,n),
                 long double POLYBENCH_2D(u_long_double,N,N,n,n))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  long double max_value_long_double = 0;
  long double sum_long_double = 0;
  long double norm_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("u");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      DATA_TYPE value = u[i][j];
      long double value_long_double = u_long_double[i][j];

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
        DATA_TYPE scaled = u[i][j] / max_value;
        sum += scaled * scaled;
      }
    norm = SQRT_FUN(sum);
    norm = 0 + norm;
  }

  if (max_value_long_double != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
        long double scaled = u_long_double[i][j] / max_value_long_double;
        sum_long_double += scaled * scaled;
      }
    norm_long_double = sqrtl(sum_long_double);
  }

  fprintf(POLYBENCH_DUMP_TARGET, "Max value in u: %.17e\n", max_value);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of u: %.17e\n", norm);
  fprintf(POLYBENCH_DUMP_TARGET, "Max value in u_long_double: %.21Le\n", max_value_long_double);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of u_long_double: %.21Le\n", norm_long_double);

  long double norm_error = norm_long_double - (long double)norm;
  fprintf(POLYBENCH_DUMP_TARGET, "Norm error: %.21Le\n", norm_error);

  POLYBENCH_DUMP_END("u");
  POLYBENCH_DUMP_FINISH;
}

static
void kernel_adi(int tsteps, int n,
                DATA_TYPE POLYBENCH_2D(u,N,N,n,n),
                DATA_TYPE POLYBENCH_2D(v,N,N,n,n),
                DATA_TYPE POLYBENCH_2D(p,N,N,n,n),
                DATA_TYPE POLYBENCH_2D(q,N,N,n,n))
{
  int t, i, j;
  DATA_TYPE DX, DY, DT;
  DATA_TYPE B1, B2;
  DATA_TYPE mul1, mul2;
  DATA_TYPE a, b, c, d, e, f;

#pragma scop

  DX = SCALAR_VAL(1.0)/(DATA_TYPE)_PB_N;
  DY = SCALAR_VAL(1.0)/(DATA_TYPE)_PB_N;
  DT = SCALAR_VAL(1.0)/(DATA_TYPE)_PB_TSTEPS;
  B1 = SCALAR_VAL(2.0);
  B2 = SCALAR_VAL(1.0);
  mul1 = B1 * DT / (DX * DX);
  mul2 = B2 * DT / (DY * DY);

  a = -mul1 /  SCALAR_VAL(2.0);
  b = SCALAR_VAL(1.0)+mul1;
  c = a;
  d = -mul2 / SCALAR_VAL(2.0);
  e = SCALAR_VAL(1.0)+mul2;
  f = d;

 for (t=1; t<=_PB_TSTEPS; t++) {
    for (i=1; i<_PB_N-1; i++) {
      v[0][i] = SCALAR_VAL(1.0);
      p[i][0] = SCALAR_VAL(0.0);
      q[i][0] = v[0][i];
      for (j=1; j<_PB_N-1; j++) {
        p[i][j] = -c / (a*p[i][j-1]+b);
        q[i][j] = (-d*u[j][i-1]+(SCALAR_VAL(1.0)+SCALAR_VAL(2.0)*d)*u[j][i] - f*u[j][i+1]-a*q[i][j-1])/(a*p[i][j-1]+b);
      }

      v[_PB_N-1][i] = SCALAR_VAL(1.0);
      for (j=_PB_N-2; j>=1; j--) {
        v[j][i] = p[i][j] * v[j+1][i] + q[i][j];
      }
    }
    for (i=1; i<_PB_N-1; i++) {
      u[i][0] = SCALAR_VAL(1.0);
      p[i][0] = SCALAR_VAL(0.0);
      q[i][0] = u[i][0];
      for (j=1; j<_PB_N-1; j++) {
        p[i][j] = -f / (d*p[i][j-1]+e);
        q[i][j] = (-a*v[i-1][j]+(SCALAR_VAL(1.0)+SCALAR_VAL(2.0)*a)*v[i][j] - c*v[i+1][j]-d*q[i][j-1])/(d*p[i][j-1]+e);
      }
      u[i][_PB_N-1] = SCALAR_VAL(1.0);
      for (j=_PB_N-2; j>=1; j--) {
        u[i][j] = p[i][j] * u[i][j+1] + q[i][j];
      }
    }
  }
#pragma endscop
}

static
void kernel_adi_long_double(int tsteps, int n,
                long double POLYBENCH_2D(u,N,N,n,n),
                long double POLYBENCH_2D(v,N,N,n,n),
                long double POLYBENCH_2D(p,N,N,n,n),
                long double POLYBENCH_2D(q,N,N,n,n))
{
  int t, i, j;
  long double DX, DY, DT;
  long double B1, B2;
  long double mul1, mul2;
  long double a, b, c, d, e, f;

#pragma scop

  DX = 1.0L/(long double)_PB_N;
  DY = 1.0L/(long double)_PB_N;
  DT = 1.0L/(long double)_PB_TSTEPS;
  B1 = 2.0L;
  B2 = 1.0L;
  mul1 = B1 * DT / (DX * DX);
  mul2 = B2 * DT / (DY * DY);

  a = -mul1 /  2.0L;
  b = 1.0L+mul1;
  c = a;
  d = -mul2 / 2.0L;
  e = 1.0L+mul2;
  f = d;

 for (t=1; t<=_PB_TSTEPS; t++) {
    for (i=1; i<_PB_N-1; i++) {
      v[0][i] = 1.0L;
      p[i][0] = 0.0L;
      q[i][0] = v[0][i];
      for (j=1; j<_PB_N-1; j++) {
        p[i][j] = -c / (a*p[i][j-1]+b);
        q[i][j] = (-d*u[j][i-1]+(1.0L+2.0L*d)*u[j][i] - f*u[j][i+1]-a*q[i][j-1])/(a*p[i][j-1]+b);
      }

      v[_PB_N-1][i] = 1.0L;
      for (j=_PB_N-2; j>=1; j--) {
        v[j][i] = p[i][j] * v[j+1][i] + q[i][j];
      }
    }
    for (i=1; i<_PB_N-1; i++) {
      u[i][0] = 1.0L;
      p[i][0] = 0.0L;
      q[i][0] = u[i][0];
      for (j=1; j<_PB_N-1; j++) {
        p[i][j] = -f / (d*p[i][j-1]+e);
        q[i][j] = (-a*v[i-1][j]+(1.0L+2.0L*a)*v[i][j] - c*v[i+1][j]-d*q[i][j-1])/(d*p[i][j-1]+e);
      }
      u[i][_PB_N-1] = 1.0L;
      for (j=_PB_N-2; j>=1; j--) {
        u[i][j] = p[i][j] * u[i][j+1] + q[i][j];
      }
    }
  }
#pragma endscop
}

int main(int argc, char** argv)
{
  int n = N;
  int tsteps = TSTEPS;

  POLYBENCH_2D_ARRAY_DECL(u, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(v, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(p, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(q, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(u_long_double, long double, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(v_long_double, long double, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(p_long_double, long double, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(q_long_double, long double, N, N, n, n);

  init_array (n, POLYBENCH_ARRAY(u));
  init_array_long_double (n, POLYBENCH_ARRAY(u_long_double));

  polybench_start_instruments;

  kernel_adi (tsteps, n, POLYBENCH_ARRAY(u), POLYBENCH_ARRAY(v), POLYBENCH_ARRAY(p), POLYBENCH_ARRAY(q));
  kernel_adi_long_double (tsteps, n, POLYBENCH_ARRAY(u_long_double), POLYBENCH_ARRAY(v_long_double), POLYBENCH_ARRAY(p_long_double), POLYBENCH_ARRAY(q_long_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(u), POLYBENCH_ARRAY(u_long_double)));

  POLYBENCH_FREE_ARRAY(u);
  POLYBENCH_FREE_ARRAY(v);
  POLYBENCH_FREE_ARRAY(p);
  POLYBENCH_FREE_ARRAY(q);
  POLYBENCH_FREE_ARRAY(u_long_double);
  POLYBENCH_FREE_ARRAY(v_long_double);
  POLYBENCH_FREE_ARRAY(p_long_double);
  POLYBENCH_FREE_ARRAY(q_long_double);

  return 0;
}
