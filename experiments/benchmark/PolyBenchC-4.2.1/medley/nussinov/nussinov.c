/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* nussinov.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "nussinov.h"

/* RNA bases represented as chars, range is [0,3] */
typedef char base;

#define match(b1, b2) (((b1)+(b2)) == 3 ? 1 : 0)
#define max_score(s1, s2) ((s1 >= s2) ? s1 : s2)

/* Array initialization. */
static
void init_array (int n,
                 base POLYBENCH_1D(seq,N,n),
		 DATA_TYPE POLYBENCH_2D(table,N,N,n,n))
{
  int i, j;

  //base is AGCT/0..3
  for (i=0; i <n; i++) {
     seq[i] = (base)((i+1)%4);
  }

  for (i=0; i <n; i++)
     for (j=0; j <n; j++)
       table[i][j] = 0;
}

static
void init_array_double (int n,
                 base POLYBENCH_1D(seq,N,n),
		 double POLYBENCH_2D(table,N,N,n,n))
{
  int i, j;

  for (i=0; i <n; i++)
     seq[i] = (base)((i+1)%4);

  for (i=0; i <n; i++)
     for (j=0; j <n; j++)
       table[i][j] = 0.0;
}

static
void print_array(int n,
		 DATA_TYPE POLYBENCH_2D(table,N,N,n,n),
		 double POLYBENCH_2D(table_double,N,N,n,n))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  double max_value_double = 0;
  double sum_double = 0;
  double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("table");
  for (i = 0; i < n; i++)
    for (j = i; j < n; j++) {
      DATA_TYPE value = table[i][j];
      double value_double = table_double[i][j];

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
      for (j = i; j < n; j++) {
        DATA_TYPE scaled = table[i][j] / max_value;
        sum += scaled * scaled;
      }
    norm = SQRT_FUN(sum);
  }

  if (max_value_double != 0) {
    for (i = 0; i < n; i++)
      for (j = i; j < n; j++) {
        double scaled = table_double[i][j] / max_value_double;
        sum_double += scaled * scaled;
      }
    norm_double = sqrt(sum_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in table: %.7e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of table: %.7e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in table_double: %.17e\n", max_value_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of table_double: %.17e\n", norm_double);

  double norm_error = norm_double - (double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.17e\n", norm_error);

  POLYBENCH_DUMP_END("table");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
/*
  Original version by Dave Wonnacott at Haverford College <davew@cs.haverford.edu>,
  with help from Allison Lake, Ting Zhou, and Tian Jin,
  based on algorithm by Nussinov, described in Allison Lake's senior thesis.
*/
static
void kernel_nussinov(int n, base POLYBENCH_1D(seq,N,n),
			   DATA_TYPE POLYBENCH_2D(table,N,N,n,n))
{
  int i, j, k;

#pragma scop
 for (i = _PB_N-1; i >= 0; i--) {
  for (j=i+1; j<_PB_N; j++) {

   if (j-1>=0)
      table[i][j] = max_score(table[i][j], table[i][j-1]);
   if (i+1<_PB_N)
      table[i][j] = max_score(table[i][j], table[i+1][j]);

   if (j-1>=0 && i+1<_PB_N) {
     /* don't allow adjacent elements to bond */
     if (i<j-1)
        table[i][j] = max_score(table[i][j], table[i+1][j-1]+match(seq[i], seq[j]));
     else
        table[i][j] = max_score(table[i][j], table[i+1][j-1]);
   }

   for (k=i+1; k<j; k++) {
      table[i][j] = max_score(table[i][j], table[i][k] + table[k+1][j]);
   }
  }
 }
#pragma endscop
}

static
void kernel_nussinov_double(int n, base POLYBENCH_1D(seq,N,n),
			   double POLYBENCH_2D(table,N,N,n,n))
{
  int i, j, k;

#pragma scop
 for (i = _PB_N-1; i >= 0; i--) {
  for (j=i+1; j<_PB_N; j++) {

   if (j-1>=0)
      table[i][j] = max_score(table[i][j], table[i][j-1]);
   if (i+1<_PB_N)
      table[i][j] = max_score(table[i][j], table[i+1][j]);

   if (j-1>=0 && i+1<_PB_N) {
     if (i<j-1)
        table[i][j] = max_score(table[i][j], table[i+1][j-1]+match(seq[i], seq[j]));
     else
        table[i][j] = max_score(table[i][j], table[i+1][j-1]);
   }

   for (k=i+1; k<j; k++) {
      table[i][j] = max_score(table[i][j], table[i][k] + table[k+1][j]);
   }
  }
 }
#pragma endscop
}

int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  POLYBENCH_1D_ARRAY_DECL(seq, base, N, n);
  POLYBENCH_1D_ARRAY_DECL(seq_double, base, N, n);
  POLYBENCH_2D_ARRAY_DECL(table, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(table_double, double, N, N, n, n);

  /* Array initialization. */
  init_array (n, POLYBENCH_ARRAY(seq), POLYBENCH_ARRAY(table));
  init_array_double (n, POLYBENCH_ARRAY(seq_double), POLYBENCH_ARRAY(table_double));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_nussinov (n, POLYBENCH_ARRAY(seq), POLYBENCH_ARRAY(table));
  kernel_nussinov_double (n, POLYBENCH_ARRAY(seq_double), POLYBENCH_ARRAY(table_double));

  /* Stop timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(table), POLYBENCH_ARRAY(table_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(seq);
  POLYBENCH_FREE_ARRAY(seq_double);
  POLYBENCH_FREE_ARRAY(table);
  POLYBENCH_FREE_ARRAY(table_double);

  return 0;
}
