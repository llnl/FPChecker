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

#include <polybench.h>
#include "nussinov.h"

/* RNA bases represented as chars, range is [0,3] */
typedef char base;

#define match(b1, b2) (((b1)+(b2)) == 3 ? 1 : 0)
#define max_score(s1, s2) ((s1 >= s2) ? s1 : s2)

static
void init_array (int n,
                 base POLYBENCH_1D(seq,N,n),
		 DATA_TYPE POLYBENCH_2D(table,N,N,n,n))
{
  int i, j;

  for (i=0; i <n; i++)
     seq[i] = (base)((i+1)%4);

  for (i=0; i <n; i++)
     for (j=0; j <n; j++)
       table[i][j] = SCALAR_VAL(0.0);
}

static
void init_array_long_double (int n,
                 base POLYBENCH_1D(seq,N,n),
		 long double POLYBENCH_2D(table,N,N,n,n))
{
  int i, j;

  for (i=0; i <n; i++)
     seq[i] = (base)((i+1)%4);

  for (i=0; i <n; i++)
     for (j=0; j <n; j++)
       table[i][j] = 0.0L;
}

static
void print_array(int n,
		 DATA_TYPE POLYBENCH_2D(table,N,N,n,n),
		 long double POLYBENCH_2D(table_long_double,N,N,n,n))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  long double max_value_long_double = 0;
  long double sum_long_double = 0;
  long double norm_long_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("table");
  for (i = 0; i < n; i++)
    for (j = i; j < n; j++) {
      DATA_TYPE value = table[i][j];
      long double value_long_double = table_long_double[i][j];

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
      for (j = i; j < n; j++) {
        DATA_TYPE scaled = table[i][j] / max_value;
        sum += scaled * scaled;
      }
    norm = SQRT_FUN(sum);
    norm = 0 + norm;
  }

  if (max_value_long_double != 0) {
    for (i = 0; i < n; i++)
      for (j = i; j < n; j++) {
        long double scaled = table_long_double[i][j] / max_value_long_double;
        sum_long_double += scaled * scaled;
      }
    norm_long_double = sqrtl(sum_long_double);
  }

  fprintf (POLYBENCH_DUMP_TARGET, "Max value in table: %.17e\n", max_value);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of table: %.17e\n", norm);
  fprintf (POLYBENCH_DUMP_TARGET, "Max value in table_long_double: %.21Le\n", max_value_long_double);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm of table_long_double: %.21Le\n", norm_long_double);

  long double norm_error = norm_long_double - (long double)norm;
  fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.21Le\n", norm_error);

  POLYBENCH_DUMP_END("table");
  POLYBENCH_DUMP_FINISH;
}

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
void kernel_nussinov_long_double(int n, base POLYBENCH_1D(seq,N,n),
			   long double POLYBENCH_2D(table,N,N,n,n))
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
  int n = N;

  POLYBENCH_1D_ARRAY_DECL(seq, base, N, n);
  POLYBENCH_1D_ARRAY_DECL(seq_long_double, base, N, n);
  POLYBENCH_2D_ARRAY_DECL(table, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(table_long_double, long double, N, N, n, n);

  init_array (n, POLYBENCH_ARRAY(seq), POLYBENCH_ARRAY(table));
  init_array_long_double (n, POLYBENCH_ARRAY(seq_long_double), POLYBENCH_ARRAY(table_long_double));

  polybench_start_instruments;

  kernel_nussinov (n, POLYBENCH_ARRAY(seq), POLYBENCH_ARRAY(table));
  kernel_nussinov_long_double (n, POLYBENCH_ARRAY(seq_long_double), POLYBENCH_ARRAY(table_long_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(table), POLYBENCH_ARRAY(table_long_double)));

  POLYBENCH_FREE_ARRAY(seq);
  POLYBENCH_FREE_ARRAY(seq_long_double);
  POLYBENCH_FREE_ARRAY(table);
  POLYBENCH_FREE_ARRAY(table_long_double);

  return 0;
}
