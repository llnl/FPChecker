/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* gemm.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "gemm.h"


/* Array initialization. */
static
void init_array(int ni, int nj, int nk,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D(C,NI,NJ,ni,nj),
		DATA_TYPE POLYBENCH_2D(A,NI,NK,ni,nk),
		DATA_TYPE POLYBENCH_2D(B,NK,NJ,nk,nj))
{
  int i, j;

  *alpha = 1.5;
  *beta = 1.2;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nj; j++)
      C[i][j] = (DATA_TYPE) ((i*j+1) % ni) / ni;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nk; j++)
      A[i][j] = (DATA_TYPE) (i*(j+1) % nk) / nk;
  for (i = 0; i < nk; i++)
    for (j = 0; j < nj; j++)
      B[i][j] = (DATA_TYPE) (i*(j+2) % nj) / nj;
}

static 
void init_array_long_double(int ni, int nj, int nk,
		long double *alpha,
		long double *beta,
		long double POLYBENCH_2D(C,NI,NJ,ni,nj),
		long double POLYBENCH_2D(A,NI,NK,ni,nk),
		long double POLYBENCH_2D(B,NK,NJ,nk,nj))
{
  int i, j;

  *alpha = 1.5L;
  *beta = 1.2L;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nj; j++)
      C[i][j] = (long double) ((i*j+1) % ni) / ni;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nk; j++)
      A[i][j] = (long double) (i*(j+1) % nk) / nk;
  for (i = 0; i < nk; i++)
    for (j = 0; j < nj; j++)
      B[i][j] = (long double) (i*(j+2) % nj) / nj;
}

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int ni, int nj,
		 DATA_TYPE POLYBENCH_2D(C,NI,NJ,ni,nj), long double POLYBENCH_2D(C_long_double,NI,NJ,ni,nj))
{
  int i, j;

  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0; 

  long double max_value_double = 0;
  long double sum_double = 0;
  long double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("C\n");
  // for (i = 0; i < ni; i++)
  //   for (j = 0; j < nj; j++) {
	// if ((i * ni + j) % 20 == 0) fprintf (POLYBENCH_DUMP_TARGET, "\n");
	// fprintf (POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, C[i][j]);
  //   }

  for (i = 0; i < ni; i++)
    for (j = 0; j < nj; j++) {
      DATA_TYPE value = C[i][j];
      long double value_double = C_long_double[i][j];

    if (value < 0)
      value = -value;
    
    if (value_double < 0.0)
      value_double = -value_double;

    if (value > max_value) {
      max_value = value;
    }
    if (value_double > max_value_double) {
      max_value_double = value_double;
    }
  }

  if (max_value != 0) {
    for (i = 0; i < ni; i++) {
      for (j = 0; j < nj; j++) {
        DATA_TYPE scaled = C[i][j] / max_value;
        sum += scaled * scaled;
      }
    }
    norm = SQRT_FUN(sum);
  }

  if (max_value_double != 0) {
    for (i = 0; i < ni; i++) {
      for (j = 0; j < nj; j++) {
        long double scaled = C_long_double[i][j] / max_value_double;
        sum_double += scaled * scaled;
      }
    }
    norm_double = sqrtl(sum_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in C: %.17e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of C: %.17e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in long_double: %.21Le\n", max_value_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of long_double: %.21Le\n", norm_double);

  double norm_error = norm_double - (long double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.17e\n", norm_error);

  POLYBENCH_DUMP_END("C");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_gemm(int ni, int nj, int nk,
		 DATA_TYPE alpha,
		 DATA_TYPE beta,
		 DATA_TYPE POLYBENCH_2D(C,NI,NJ,ni,nj),
		 DATA_TYPE POLYBENCH_2D(A,NI,NK,ni,nk),
		 DATA_TYPE POLYBENCH_2D(B,NK,NJ,nk,nj))
{
  int i, j, k;

//BLAS PARAMS
//TRANSA = 'N'
//TRANSB = 'N'
// => Form C := alpha*A*B + beta*C,
//A is NIxNK
//B is NKxNJ
//C is NIxNJ
#pragma scop
  for (i = 0; i < _PB_NI; i++) {
    for (j = 0; j < _PB_NJ; j++)
	C[i][j] *= beta;
    for (k = 0; k < _PB_NK; k++) {
       for (j = 0; j < _PB_NJ; j++)
	  C[i][j] += alpha * A[i][k] * B[k][j];
    }
  }
#pragma endscop

}

static
void kernel_gemm_long_double(int ni, int nj, int nk,
		 long double alpha,
		 long double beta,
		 long double POLYBENCH_2D(C,NI,NJ,ni,nj),
		 long double POLYBENCH_2D(A,NI,NK,ni,nk),
		 long double POLYBENCH_2D(B,NK,NJ,nk,nj))
{
  int i, j, k;

#pragma scop
  for (i = 0; i < _PB_NI; i++) {
    for (j = 0; j < _PB_NJ; j++)
	C[i][j] *= beta;
    for (k = 0; k < _PB_NK; k++) {
       for (j = 0; j < _PB_NJ; j++)
	  C[i][j] += alpha * A[i][k] * B[k][j];
    }
  }
#pragma endscop

}

int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int ni = NI;
  int nj = NJ;
  int nk = NK;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  long double alpha_long_double;
  long double beta_long_double;

  POLYBENCH_2D_ARRAY_DECL(C,DATA_TYPE,NI,NJ,ni,nj);
  POLYBENCH_2D_ARRAY_DECL(A,DATA_TYPE,NI,NK,ni,nk);
  POLYBENCH_2D_ARRAY_DECL(B,DATA_TYPE,NK,NJ,nk,nj);

  POLYBENCH_2D_ARRAY_DECL(C_long_double,long double,NI,NJ,ni,nj);
  POLYBENCH_2D_ARRAY_DECL(A_long_double,long double,NI,NK,ni,nk);
  POLYBENCH_2D_ARRAY_DECL(B_long_double,long double,NK,NJ,nk,nj);

  /* Initialize array(s). */
  init_array (ni, nj, nk, &alpha, &beta,
	      POLYBENCH_ARRAY(C),
	      POLYBENCH_ARRAY(A),
	      POLYBENCH_ARRAY(B));
  
  init_array_long_double (ni, nj, nk, &alpha_long_double, &beta_long_double,
	      POLYBENCH_ARRAY(C_long_double),
	      POLYBENCH_ARRAY(A_long_double),
	      POLYBENCH_ARRAY(B_long_double));
  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gemm (ni, nj, nk,
	       alpha, beta,
	       POLYBENCH_ARRAY(C),
	       POLYBENCH_ARRAY(A),
	       POLYBENCH_ARRAY(B));
  
  kernel_gemm_long_double (ni, nj, nk,
	       alpha_long_double, beta_long_double,
	       POLYBENCH_ARRAY(C_long_double),
	       POLYBENCH_ARRAY(A_long_double),
	       POLYBENCH_ARRAY(B_long_double));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(ni, nj,  POLYBENCH_ARRAY(C), POLYBENCH_ARRAY(C_long_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(C);
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(C_long_double);
  POLYBENCH_FREE_ARRAY(A_long_double);
  POLYBENCH_FREE_ARRAY(B_long_double);

  return 0;
}
