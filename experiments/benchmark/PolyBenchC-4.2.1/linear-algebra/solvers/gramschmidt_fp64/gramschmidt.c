/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* gramschmidt.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "gramschmidt.h"


/* Array initialization. */
static
void init_array(int m, int n,
		DATA_TYPE POLYBENCH_2D(A,M,N,m,n),
		DATA_TYPE POLYBENCH_2D(R,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(Q,M,N,m,n))
{
  int i, j;

  for (i = 0; i < m; i++)
    for (j = 0; j < n; j++) {
      A[i][j] = (((DATA_TYPE) ((i*j) % m) / m )*100) + 10;
      Q[i][j] = 0.0;
    }
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      R[i][j] = 0.0;
}

static
void init_array_long_double(int m, int n,
		long double POLYBENCH_2D(A,M,N,m,n),
		long double POLYBENCH_2D(R,N,N,n,n),
		long double POLYBENCH_2D(Q,M,N,m,n))
{
  int i, j;

  for (i = 0; i < m; i++)
    for (j = 0; j < n; j++) {
      A[i][j] = (((long double) ((i*j) % m) / m )*100.0L) + 10.0L;
      Q[i][j] = 0.0L;
    }
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      R[i][j] = 0.0L;
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int m, int n,
		 DATA_TYPE POLYBENCH_2D(A,M,N,m,n),
		 DATA_TYPE POLYBENCH_2D(R,N,N,n,n),
		 DATA_TYPE POLYBENCH_2D(Q,M,N,m,n),
		 long double POLYBENCH_2D(R_long_double,N,N,n,n),
		 long double POLYBENCH_2D(Q_long_double,M,N,m,n))
{
  int i, j;
  DATA_TYPE max_r = 0;
  DATA_TYPE sum_r = 0;
  DATA_TYPE norm_r = 0;
  DATA_TYPE max_q = 0;
  DATA_TYPE sum_q = 0;
  DATA_TYPE norm_q = 0;
  long double max_r_long_double = 0;
  long double sum_r_long_double = 0;
  long double norm_r_long_double = 0;
  long double max_q_long_double = 0;
  long double sum_q_long_double = 0;
  long double norm_q_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("R");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
	DATA_TYPE value = R[i][j];
	long double value_long_double = R_long_double[i][j];

	if (value < 0)
	  value = -value;
	if (value_long_double < 0.0L)
	  value_long_double = -value_long_double;

	if (value > max_r)
	  max_r = value;
	if (value_long_double > max_r_long_double)
	  max_r_long_double = value_long_double;
    }

  if (max_r != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
        DATA_TYPE scaled = R[i][j] / max_r;
        sum_r += scaled * scaled;
      }
    norm_r = SQRT_FUN(sum_r);
  }

  if (max_r_long_double != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
        long double scaled = R_long_double[i][j] / max_r_long_double;
        sum_r_long_double += scaled * scaled;
      }
    norm_r_long_double = sqrtl(sum_r_long_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in R: %.17e\n", max_r);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of R: %.17e\n", norm_r);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in R_long_double: %.21Le\n", max_r_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of R_long_double: %.21Le\n", norm_r_long_double);

  long double norm_error_r = norm_r_long_double - (long double)norm_r;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error in R: %.21Le\n", norm_error_r);

  POLYBENCH_DUMP_END("R");

  POLYBENCH_DUMP_BEGIN("Q");
  for (i = 0; i < m; i++)
    for (j = 0; j < n; j++) {
      DATA_TYPE value = Q[i][j];
      long double value_long_double = Q_long_double[i][j];

	if (value < 0)
	  value = -value;
	if (value_long_double < 0.0L)
	  value_long_double = -value_long_double;

	if (value > max_q)
	  max_q = value;
	if (value_long_double > max_q_long_double)
	  max_q_long_double = value_long_double;
    }

  if (max_q != 0) {
    for (i = 0; i < m; i++)
      for (j = 0; j < n; j++) {
        DATA_TYPE scaled = Q[i][j] / max_q;
        sum_q += scaled * scaled;
      }
    norm_q = SQRT_FUN(sum_q);
  }

  if (max_q_long_double != 0) {
    for (i = 0; i < m; i++)
      for (j = 0; j < n; j++) {
        long double scaled = Q_long_double[i][j] / max_q_long_double;
        sum_q_long_double += scaled * scaled;
      }
    norm_q_long_double = sqrtl(sum_q_long_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in Q: %.17e\n", max_q);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of Q: %.17e\n", norm_q);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in Q_long_double: %.21Le\n", max_q_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of Q_long_double: %.21Le\n", norm_q_long_double);

  long double norm_error_q = norm_q_long_double - (long double)norm_q;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error in Q: %.21Le\n", norm_error_q);

  POLYBENCH_DUMP_END("Q");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
/* QR Decomposition with Modified Gram Schmidt:
 http://www.inf.ethz.ch/personal/gander/ */
static
void kernel_gramschmidt(int m, int n,
			DATA_TYPE POLYBENCH_2D(A,M,N,m,n),
			DATA_TYPE POLYBENCH_2D(R,N,N,n,n),
			DATA_TYPE POLYBENCH_2D(Q,M,N,m,n))
{
  int i, j, k;

  DATA_TYPE nrm;

#pragma scop
  for (k = 0; k < _PB_N; k++)
    {
      nrm = SCALAR_VAL(0.0);
      for (i = 0; i < _PB_M; i++)
        nrm += A[i][k] * A[i][k];
      R[k][k] = SQRT_FUN(nrm);
      for (i = 0; i < _PB_M; i++)
        Q[i][k] = A[i][k] / R[k][k];
      for (j = k + 1; j < _PB_N; j++)
	{
	  R[k][j] = SCALAR_VAL(0.0);
	  for (i = 0; i < _PB_M; i++)
	    R[k][j] += Q[i][k] * A[i][j];
	  for (i = 0; i < _PB_M; i++)
	    A[i][j] = A[i][j] - Q[i][k] * R[k][j];
	}
    }
#pragma endscop

}

static
void kernel_gramschmidt_long_double(int m, int n,
			long double POLYBENCH_2D(A,M,N,m,n),
			long double POLYBENCH_2D(R,N,N,n,n),
			long double POLYBENCH_2D(Q,M,N,m,n))
{
  int i, j, k;

  long double nrm;

#pragma scop
  for (k = 0; k < _PB_N; k++)
    {
      nrm = 0.0L;
      for (i = 0; i < _PB_M; i++)
        nrm += A[i][k] * A[i][k];
      R[k][k] = sqrtl(nrm);
      for (i = 0; i < _PB_M; i++)
        Q[i][k] = A[i][k] / R[k][k];
      for (j = k + 1; j < _PB_N; j++)
	{
	  R[k][j] = 0.0L;
	  for (i = 0; i < _PB_M; i++)
	    R[k][j] += Q[i][k] * A[i][j];
	  for (i = 0; i < _PB_M; i++)
	    A[i][j] = A[i][j] - Q[i][k] * R[k][j];
	}
    }
#pragma endscop

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int m = M;
  int n = N;

  /* Variable declaration/allocation. */
  POLYBENCH_2D_ARRAY_DECL(A,DATA_TYPE,M,N,m,n);
  POLYBENCH_2D_ARRAY_DECL(R,DATA_TYPE,N,N,n,n);
  POLYBENCH_2D_ARRAY_DECL(Q,DATA_TYPE,M,N,m,n);
  POLYBENCH_2D_ARRAY_DECL(A_long_double,long double,M,N,m,n);
  POLYBENCH_2D_ARRAY_DECL(R_long_double,long double,N,N,n,n);
  POLYBENCH_2D_ARRAY_DECL(Q_long_double,long double,M,N,m,n);

  /* Initialize array(s). */
  init_array (m, n,
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(R),
	      POLYBENCH_ARRAY(Q));
  init_array_long_double (m, n,
	      POLYBENCH_ARRAY(A_long_double),
	      POLYBENCH_ARRAY(R_long_double),
	      POLYBENCH_ARRAY(Q_long_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gramschmidt (m, n,
		      POLYBENCH_ARRAY(A),
		      POLYBENCH_ARRAY(R),
		      POLYBENCH_ARRAY(Q));
  kernel_gramschmidt_long_double (m, n,
		      POLYBENCH_ARRAY(A_long_double),
		      POLYBENCH_ARRAY(R_long_double),
		      POLYBENCH_ARRAY(Q_long_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(m, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(R), POLYBENCH_ARRAY(Q),
				    POLYBENCH_ARRAY(R_long_double), POLYBENCH_ARRAY(Q_long_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(R);
  POLYBENCH_FREE_ARRAY(Q);
  POLYBENCH_FREE_ARRAY(A_long_double);
  POLYBENCH_FREE_ARRAY(R_long_double);
  POLYBENCH_FREE_ARRAY(Q_long_double);

  return 0;
}
