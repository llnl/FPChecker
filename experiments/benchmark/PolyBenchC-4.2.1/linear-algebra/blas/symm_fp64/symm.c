/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* symm.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "symm.h"


/* Array initialization. */
static
void init_array(int m, int n,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D(C,M,N,m,n),
		DATA_TYPE POLYBENCH_2D(A,M,M,m,m),
		DATA_TYPE POLYBENCH_2D(B,M,N,m,n))
{
  int i, j;

  *alpha = 1.5;
  *beta = 1.2;
  for (i = 0; i < m; i++)
    for (j = 0; j < n; j++) {
      C[i][j] = (DATA_TYPE) ((i+j) % 100) / m;
      B[i][j] = (DATA_TYPE) ((n+i-j) % 100) / m;
    }
  for (i = 0; i < m; i++) {
    for (j = 0; j <=i; j++)
      A[i][j] = (DATA_TYPE) ((i+j) % 100) / m;
    for (j = i+1; j < m; j++)
      A[i][j] = -999; //regions of arrays that should not be used
  }
}

static
void init_array_long_double(int m, int n,
		long double *alpha,
		long double *beta,
		long double POLYBENCH_2D(C,M,N,m,n),
		long double POLYBENCH_2D(A,M,M,m,m),
		long double POLYBENCH_2D(B,M,N,m,n))
{
  int i, j;

  *alpha = 1.5L;
  *beta = 1.2L;
  for (i = 0; i < m; i++)
    for (j = 0; j < n; j++) {
      C[i][j] = (long double) ((i+j) % 100) / m;
      B[i][j] = (long double) ((n+i-j) % 100) / m;
    }
  for (i = 0; i < m; i++) {
    for (j = 0; j <=i; j++)
      A[i][j] = (long double) ((i+j) % 100) / m;
    for (j = i+1; j < m; j++)
      A[i][j] = -999;
  }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int m, int n,
		 DATA_TYPE POLYBENCH_2D(C,M,N,m,n),
		 long double POLYBENCH_2D(C_long_double,M,N,m,n))
{
  int i, j;

  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;

  long double max_value_long_double = 0;
  long double sum_long_double = 0;
  long double norm_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("C");
  for (i = 0; i < m; i++){
    for (j = 0; j < n; j++) {
      DATA_TYPE value = C[i][j];
      long double value_long_double = C_long_double[i][j];

      if (value < 0)
	        value = -value;

      if (value_long_double < 0.0L)
	        value_long_double = -value_long_double;

      if (value > max_value)
	        max_value = value;
      if (value_long_double > max_value_long_double)
	        max_value_long_double = value_long_double;
      }
    }
  if (max_value != 0) {
    for (i = 0; i < m; i++) {
      for (j = 0; j < n; j++) {
        DATA_TYPE scaled = C[i][j] / max_value;
        sum += scaled * scaled;
      }
    }
    norm = SQRT_FUN(sum);
  }

  if (max_value_long_double != 0) {
    for (i = 0; i < m; i++) {
      for (j = 0; j < n; j++) {
	        long double scaled = C_long_double[i][j] / max_value_long_double;
	          sum_long_double += scaled * scaled;
      }
    }
    norm_long_double = sqrtl(sum_long_double);
  }

  long double norm_error = norm_long_double - (long double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in C: %.17e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of C: %.17e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in long_double: %.21Le\n", max_value_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of long_double: %.21Le\n", norm_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.21Le\n", norm_error);

  POLYBENCH_DUMP_END("C");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_symm(int m, int n,
		 DATA_TYPE alpha,
		 DATA_TYPE beta,
		 DATA_TYPE POLYBENCH_2D(C,M,N,m,n),
		 DATA_TYPE POLYBENCH_2D(A,M,M,m,m),
		 DATA_TYPE POLYBENCH_2D(B,M,N,m,n))
{
  int i, j, k;
  DATA_TYPE temp2;

//BLAS PARAMS
//SIDE = 'L'
//UPLO = 'L'
// =>  Form  C := alpha*A*B + beta*C
// A is MxM
// B is MxN
// C is MxN
//note that due to Fortran array layout, the code below more closely resembles upper triangular case in BLAS
#pragma scop
   for (i = 0; i < _PB_M; i++)
      for (j = 0; j < _PB_N; j++ )
      {
        temp2 = 0;
        for (k = 0; k < i; k++) {
           C[k][j] += alpha*B[i][j] * A[i][k];
           temp2 += B[k][j] * A[i][k];
        }
        C[i][j] = beta * C[i][j] + alpha*B[i][j] * A[i][i] + alpha * temp2;
     }
#pragma endscop

}

static
void kernel_symm_long_double(int m, int n,
		 long double alpha,
		 long double beta,
		 long double POLYBENCH_2D(C,M,N,m,n),
		 long double POLYBENCH_2D(A,M,M,m,m),
		 long double POLYBENCH_2D(B,M,N,m,n))
{
  int i, j, k;
  long double temp2;

#pragma scop
   for (i = 0; i < _PB_M; i++)
      for (j = 0; j < _PB_N; j++ )
      {
        temp2 = 0;
        for (k = 0; k < i; k++) {
           C[k][j] += alpha*B[i][j] * A[i][k];
           temp2 += B[k][j] * A[i][k];
        }
        C[i][j] = beta * C[i][j] + alpha*B[i][j] * A[i][i] + alpha * temp2;
     }
#pragma endscop

}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int m = M;
  int n = N;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  long double alpha_long_double;
  long double beta_long_double;

  POLYBENCH_2D_ARRAY_DECL(C,DATA_TYPE,M,N,m,n);
  POLYBENCH_2D_ARRAY_DECL(A,DATA_TYPE,M,M,m,m);
  POLYBENCH_2D_ARRAY_DECL(B,DATA_TYPE,M,N,m,n);

  POLYBENCH_2D_ARRAY_DECL(C_long_double,long double,M,N,m,n);
  POLYBENCH_2D_ARRAY_DECL(A_long_double,long double,M,M,m,m);
  POLYBENCH_2D_ARRAY_DECL(B_long_double,long double,M,N,m,n);

  /* Initialize array(s). */
  init_array (m, n, &alpha, &beta,
	      POLYBENCH_ARRAY(C),
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(B));

  init_array_long_double (m, n, &alpha_long_double, &beta_long_double,
	      POLYBENCH_ARRAY(C_long_double),
	      POLYBENCH_ARRAY(A_long_double),
	      POLYBENCH_ARRAY(B_long_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_symm (m, n,
	       alpha, beta,
	       POLYBENCH_ARRAY(C),
	       POLYBENCH_ARRAY(A),
	       POLYBENCH_ARRAY(B));

  kernel_symm_long_double (m, n,
	       alpha_long_double, beta_long_double,
	       POLYBENCH_ARRAY(C_long_double),
	       POLYBENCH_ARRAY(A_long_double),
	       POLYBENCH_ARRAY(B_long_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(m, n, POLYBENCH_ARRAY(C), POLYBENCH_ARRAY(C_long_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(C);
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(C_long_double);
  POLYBENCH_FREE_ARRAY(A_long_double);
  POLYBENCH_FREE_ARRAY(B_long_double);

  return 0;
}
