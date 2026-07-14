/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* bicg.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "bicg.h"


/* Array initialization. */
static
void init_array (int m, int n,
		 DATA_TYPE POLYBENCH_2D(A,N,M,n,m),
		 DATA_TYPE POLYBENCH_1D(r,N,n),
		 DATA_TYPE POLYBENCH_1D(p,M,m))
{
  int i, j;

  for (i = 0; i < m; i++)
    p[i] = (DATA_TYPE)(i % m) / m;
  for (i = 0; i < n; i++) {
    r[i] = (DATA_TYPE)(i % n) / n;
    for (j = 0; j < m; j++)
      A[i][j] = (DATA_TYPE) (i*(j+1) % n)/n;
  }
}

static
void init_array_double (int m, int n,
		 double POLYBENCH_2D(A,N,M,n,m),
		 double POLYBENCH_1D(r,N,n),
		 double POLYBENCH_1D(p,M,m))
{
  int i, j;

  for (i = 0; i < m; i++)
    p[i] = (double)(i % m) / m;
  for (i = 0; i < n; i++) {
    r[i] = (double)(i % n) / n;
    for (j = 0; j < m; j++)
      A[i][j] = (double) (i*(j+1) % n)/n;
  }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int m, int n,
		 DATA_TYPE POLYBENCH_1D(s,M,m),
		 DATA_TYPE POLYBENCH_1D(q,N,n),
		 double POLYBENCH_1D(s_double,M,m),
		 double POLYBENCH_1D(q_double,N,n))

{
  int i;
  DATA_TYPE max_s = 0;
  DATA_TYPE sum_s = 0;
  DATA_TYPE norm_s = 0;
  DATA_TYPE max_q = 0;
  DATA_TYPE sum_q = 0;
  DATA_TYPE norm_q = 0;

  double max_s_double = 0;
  double sum_s_double = 0;
  double norm_s_double = 0;
  double max_q_double = 0;
  double sum_q_double = 0;
  double norm_q_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("s");
  for (i = 0; i < m; i++) {
    DATA_TYPE value = s[i];
    double value_double = s_double[i];

    if (value < 0)
      value = -value;
    if (value_double < 0.0)
      value_double = -value_double;

    if (value > max_s)
      max_s = value;
    if (value_double > max_s_double)
      max_s_double = value_double;
  }

  if (max_s != 0) {
    for (i = 0; i < m; i++) {
      DATA_TYPE scaled = s[i] / max_s;
      sum_s += scaled * scaled;
    }
    norm_s = SQRT_FUN(sum_s);
    norm_s = 0 + norm_s;
  }

  if (max_s_double != 0) {
    for (i = 0; i < m; i++) {
      double scaled = s_double[i] / max_s_double;
      sum_s_double += scaled * scaled;
    }
    norm_s_double = sqrt(sum_s_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in s: %.7e\n", max_s);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of s: %.7e\n", norm_s);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in s_double: %.17e\n", max_s_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of s_double: %.17e\n", norm_s_double);

  double norm_error_s = norm_s_double - (double)norm_s;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error in s: %.17e\n", norm_error_s);

  POLYBENCH_DUMP_END("s");
  POLYBENCH_DUMP_BEGIN("q");
  for (i = 0; i < n; i++) {
    DATA_TYPE value = q[i];
    double value_double = q_double[i];

    if (value < 0)
      value = -value;
    if (value_double < 0.0)
      value_double = -value_double;

    if (value > max_q)
      max_q = value;
    if (value_double > max_q_double)
      max_q_double = value_double;
  }

  if (max_q != 0) {
    for (i = 0; i < n; i++) {
      DATA_TYPE scaled = q[i] / max_q;
      sum_q += scaled * scaled;
    }
    norm_q = SQRT_FUN(sum_q);
    norm_q = 0 + norm_q;
  }

  if (max_q_double != 0) {
    for (i = 0; i < n; i++) {
      double scaled = q_double[i] / max_q_double;
      sum_q_double += scaled * scaled;
    }
    norm_q_double = sqrt(sum_q_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in q: %.7e\n", max_q);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of q: %.7e\n", norm_q);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in q_double: %.17e\n", max_q_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of q_double: %.17e\n", norm_q_double);

  double norm_error_q = norm_q_double - (double)norm_q;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error in q: %.17e\n", norm_error_q);

  POLYBENCH_DUMP_END("q");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_bicg(int m, int n,
		 DATA_TYPE POLYBENCH_2D(A,N,M,n,m),
		 DATA_TYPE POLYBENCH_1D(s,M,m),
		 DATA_TYPE POLYBENCH_1D(q,N,n),
		 DATA_TYPE POLYBENCH_1D(p,M,m),
		 DATA_TYPE POLYBENCH_1D(r,N,n))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_M; i++)
    s[i] = 0;
  for (i = 0; i < _PB_N; i++)
    {
      q[i] = SCALAR_VAL(0.0);
      for (j = 0; j < _PB_M; j++)
	{
	  s[j] = s[j] + r[i] * A[i][j];
	  q[i] = q[i] + A[i][j] * p[j];
	}
    }
#pragma endscop

}

static
void kernel_bicg_double(int m, int n,
		 double POLYBENCH_2D(A,N,M,n,m),
		 double POLYBENCH_1D(s,M,m),
		 double POLYBENCH_1D(q,N,n),
		 double POLYBENCH_1D(p,M,m),
		 double POLYBENCH_1D(r,N,n))
{
  int i, j;

#pragma scop
  for (i = 0; i < _PB_M; i++)
    s[i] = 0.0;
  for (i = 0; i < _PB_N; i++)
    {
      q[i] = 0.0;
      for (j = 0; j < _PB_M; j++)
	{
	  s[j] = s[j] + r[i] * A[i][j];
	  q[i] = q[i] + A[i][j] * p[j];
	}
    }
#pragma endscop

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;
  int m = M;

  /* Variable declaration/allocation. */
  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, M, n, m);
  POLYBENCH_1D_ARRAY_DECL(s, DATA_TYPE, M, m);
  POLYBENCH_1D_ARRAY_DECL(q, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(p, DATA_TYPE, M, m);
  POLYBENCH_1D_ARRAY_DECL(r, DATA_TYPE, N, n);

  POLYBENCH_2D_ARRAY_DECL(A_double, double, N, M, n, m);
  POLYBENCH_1D_ARRAY_DECL(s_double, double, M, m);
  POLYBENCH_1D_ARRAY_DECL(q_double, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(p_double, double, M, m);
  POLYBENCH_1D_ARRAY_DECL(r_double, double, N, n);

  /* Initialize array(s). */
  init_array (m, n,
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(r),
	      POLYBENCH_ARRAY(p));

  init_array_double (m, n,
	      POLYBENCH_ARRAY(A_double),
	      POLYBENCH_ARRAY(r_double),
	      POLYBENCH_ARRAY(p_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_bicg (m, n,
	       POLYBENCH_ARRAY(A),
	       POLYBENCH_ARRAY(s),
	       POLYBENCH_ARRAY(q),
	       POLYBENCH_ARRAY(p),
	       POLYBENCH_ARRAY(r));

  kernel_bicg_double (m, n,
	       POLYBENCH_ARRAY(A_double),
	       POLYBENCH_ARRAY(s_double),
	       POLYBENCH_ARRAY(q_double),
	       POLYBENCH_ARRAY(p_double),
	       POLYBENCH_ARRAY(r_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(m, n, POLYBENCH_ARRAY(s), POLYBENCH_ARRAY(q),
				    POLYBENCH_ARRAY(s_double), POLYBENCH_ARRAY(q_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(s);
  POLYBENCH_FREE_ARRAY(q);
  POLYBENCH_FREE_ARRAY(p);
  POLYBENCH_FREE_ARRAY(r);
  POLYBENCH_FREE_ARRAY(A_double);
  POLYBENCH_FREE_ARRAY(s_double);
  POLYBENCH_FREE_ARRAY(q_double);
  POLYBENCH_FREE_ARRAY(p_double);
  POLYBENCH_FREE_ARRAY(r_double);

  return 0;
}
