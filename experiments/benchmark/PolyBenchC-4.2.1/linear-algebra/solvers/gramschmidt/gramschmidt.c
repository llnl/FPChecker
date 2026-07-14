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
      A[i][j] = (((DATA_TYPE) ((i*j) % m) / m )*100) + 10.00f;
      Q[i][j] = 0.0f;
    }
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      R[i][j] = 0.0;
}

static
void copy_array_to_double(int m, int n,
		DATA_TYPE POLYBENCH_2D(A,M,N,m,n),
		DATA_TYPE POLYBENCH_2D(R,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(Q,M,N,m,n),
		double POLYBENCH_2D(A_double,M,N,m,n),
		double POLYBENCH_2D(R_double,N,N,n,n),
		double POLYBENCH_2D(Q_double,M,N,m,n))
{
  int i, j;

  for (i = 0; i < m; i++)
    for (j = 0; j < n; j++) {
      A_double[i][j] = (double)A[i][j];
      Q_double[i][j] = (double)Q[i][j];
    }
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      R_double[i][j] = (double)R[i][j];
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int m, int n,
		 DATA_TYPE POLYBENCH_2D(A,M,N,m,n),
		 DATA_TYPE POLYBENCH_2D(R,N,N,n,n),
		 DATA_TYPE POLYBENCH_2D(Q,M,N,m,n),
		 double POLYBENCH_2D(R_double,N,N,n,n),
		 double POLYBENCH_2D(Q_double,M,N,m,n))
{
  int i, j;

  DATA_TYPE max_value_R = 0;
  DATA_TYPE sum_R = 0;
  DATA_TYPE norm_R = 0;
  DATA_TYPE max_value_Q = 0;
  DATA_TYPE sum_Q = 0;
  DATA_TYPE norm_Q = 0;

  double max_value_R_double = 0;
  double sum_R_double = 0;
  double norm_R_double = 0;
  double sum_R_same_output = 0;
  double norm_R_same_output = 0;
  double max_value_Q_double = 0;
  double sum_Q_double = 0;
  double norm_Q_double = 0;
  double sum_Q_same_output = 0;
  double norm_Q_same_output = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("R");
  for (i = 0; i < n; i++){
    for (j = 0; j < n; j++) {
      DATA_TYPE value = R[i][j];
      double value_double = R_double[i][j];

	if (value < 0)
	  value = -value;
	if (value_double < 0.0)
	  value_double = -value_double;

	if (value > max_value_R)
	  max_value_R = value;
	if (value_double > max_value_R_double)
	  max_value_R_double = value_double;
    }
  }
  if (max_value_R != 0) {
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
          DATA_TYPE scaled = R[i][j] / max_value_R;
          sum_R += scaled * scaled;
      }
    }
    norm_R = SQRT_FUN(sum_R);
  }

  if (max_value_R_double != 0) {
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
        double scaled = R_double[i][j] / max_value_R_double;
        sum_R_double += scaled * scaled;
      }
    }
    norm_R_double = sqrt(sum_R_double);
  }

  if (max_value_R != 0) {
    for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++) {
        double scaled = (double)R[i][j] / (double)max_value_R;
        sum_R_same_output += scaled * scaled;
      }
    }
    norm_R_same_output = sqrt(sum_R_same_output);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in R: %.7e\n", max_value_R);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of R: %.7e\n", norm_R);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in R_double: %.17e\n", max_value_R_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of R_double: %.17e\n", norm_R_double);

  double norm_error_R = norm_R_double - (double)norm_R;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error R: %.17e\n", norm_error_R);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error R (same FP32 outputs): %.17e\n",
           norm_R_same_output - (double)norm_R);

  POLYBENCH_DUMP_END("R");

  POLYBENCH_DUMP_BEGIN("Q");
  for (i = 0; i < m; i++){
    for (j = 0; j < n; j++) {
	DATA_TYPE value = Q[i][j];
	double value_double = Q_double[i][j];

	if (value < 0)
	  value = -value;
	if (value_double < 0.0)
	  value_double = -value_double;

	if (value > max_value_Q)
	  max_value_Q = value;
	if (value_double > max_value_Q_double)
	  max_value_Q_double = value_double;
    } 
  }
  if (max_value_Q != 0) {
    for (i = 0; i < m; i++) {
      for (j = 0; j < n; j++) {
        DATA_TYPE scaled = Q[i][j] / max_value_Q;
        sum_Q += scaled * scaled;
      }
    }
    norm_Q = SQRT_FUN(sum_Q);
  }

  if (max_value_Q_double != 0) {
    for (i = 0; i < m; i++) {
      for (j = 0; j < n; j++) {
        double scaled = Q_double[i][j] / max_value_Q_double;
        sum_Q_double += scaled * scaled;
      }
    }
    norm_Q_double = sqrt(sum_Q_double);
  }

  if (max_value_Q != 0) {
    for (i = 0; i < m; i++) {
      for (j = 0; j < n; j++) {
        double scaled = (double)Q[i][j] / (double)max_value_Q;
        sum_Q_same_output += scaled * scaled;
      }
    }
    norm_Q_same_output = sqrt(sum_Q_same_output);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in Q: %.7e\n", max_value_Q);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of Q: %.7e\n", norm_Q);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in Q_double: %.17e\n", max_value_Q_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of Q_double: %.17e\n", norm_Q_double);

  double norm_error_Q = norm_Q_double - (double)norm_Q;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error Q: %.17e\n", norm_error_Q);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error Q (same FP32 outputs): %.17e\n",
           norm_Q_same_output - (double)norm_Q);

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
void kernel_gramschmidt_double(int m, int n,
			double POLYBENCH_2D(A,M,N,m,n),
			double POLYBENCH_2D(R,N,N,n,n),
			double POLYBENCH_2D(Q,M,N,m,n))
{
  int i, j, k;

  double nrm;

#pragma scop
  for (k = 0; k < _PB_N; k++)
    {
      nrm = 0.0;
      for (i = 0; i < _PB_M; i++)
        nrm += A[i][k] * A[i][k];
      R[k][k] = sqrt(nrm);
      for (i = 0; i < _PB_M; i++)
        Q[i][k] = A[i][k] / R[k][k];
      for (j = k + 1; j < _PB_N; j++)
	{
	  R[k][j] = 0.0;
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

  POLYBENCH_2D_ARRAY_DECL(A_double,double,M,N,m,n);
  POLYBENCH_2D_ARRAY_DECL(R_double,double,N,N,n,n);
  POLYBENCH_2D_ARRAY_DECL(Q_double,double,M,N,m,n);

  /* Initialize array(s). */
  init_array (m, n,
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(R),
	      POLYBENCH_ARRAY(Q));
  copy_array_to_double (m, n,
        POLYBENCH_ARRAY(A),
        POLYBENCH_ARRAY(R),
        POLYBENCH_ARRAY(Q),
        POLYBENCH_ARRAY(A_double),
        POLYBENCH_ARRAY(R_double),
        POLYBENCH_ARRAY(Q_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gramschmidt (m, n,
		      POLYBENCH_ARRAY(A),
		      POLYBENCH_ARRAY(R),
		      POLYBENCH_ARRAY(Q));
  kernel_gramschmidt_double (m, n,
		      POLYBENCH_ARRAY(A_double),
		      POLYBENCH_ARRAY(R_double),
		      POLYBENCH_ARRAY(Q_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(m, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(R), POLYBENCH_ARRAY(Q),
				    POLYBENCH_ARRAY(R_double), POLYBENCH_ARRAY(Q_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(R);
  POLYBENCH_FREE_ARRAY(Q);
  POLYBENCH_FREE_ARRAY(A_double);
  POLYBENCH_FREE_ARRAY(R_double);
  POLYBENCH_FREE_ARRAY(Q_double);

  return 0;
}
