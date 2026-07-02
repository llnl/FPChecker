#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <polybench.h>
#include "jacobi-2d.h"

static
void init_array_base(int n,
                     DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
                     DATA_TYPE POLYBENCH_2D(B,N,N,n,n),
                     double POLYBENCH_2D(A_shadow,N,N,n,n),
                     double POLYBENCH_2D(B_shadow,N,N,n,n))
{
  int i, j;
  double n_shadow = (double)(DATA_TYPE)n;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      A[i][j] = ((DATA_TYPE) i*(j+2) + SCALAR_VAL(2.0)) / n;
      B[i][j] = ((DATA_TYPE) i*(j+3) + SCALAR_VAL(3.0)) / n;

      A_shadow[i][j] = ((double)(DATA_TYPE) i * (double)(DATA_TYPE)(j+2) +
                        (double) SCALAR_VAL(2.0)) / n_shadow;
      B_shadow[i][j] = ((double)(DATA_TYPE) i * (double)(DATA_TYPE)(j+3) +
                        (double) SCALAR_VAL(3.0)) / n_shadow;
    }
}

static
void kernel_jacobi_2d_base(int tsteps, int n,
                           DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
                           DATA_TYPE POLYBENCH_2D(B,N,N,n,n),
                           double POLYBENCH_2D(A_shadow,N,N,n,n),
                           double POLYBENCH_2D(B_shadow,N,N,n,n))
{
  int t, i, j;
  double c_shadow = (double) SCALAR_VAL(0.2);

#pragma scop
  for (t = 0; t < _PB_TSTEPS; t++) {
    for (i = 1; i < _PB_N - 1; i++)
      for (j = 1; j < _PB_N - 1; j++) {
        B[i][j] = SCALAR_VAL(0.2) * (A[i][j] + A[i][j-1] + A[i][1+j] + A[1+i][j] + A[i-1][j]);
        B_shadow[i][j] = c_shadow * (A_shadow[i][j] + A_shadow[i][j-1] + A_shadow[i][1+j] + A_shadow[1+i][j] + A_shadow[i-1][j]);
      }
    for (i = 1; i < _PB_N - 1; i++)
      for (j = 1; j < _PB_N - 1; j++) {
        A[i][j] = SCALAR_VAL(0.2) * (B[i][j] + B[i][j-1] + B[i][1+j] + B[1+i][j] + B[i-1][j]);
        A_shadow[i][j] = c_shadow * (B_shadow[i][j] + B_shadow[i][j-1] + B_shadow[i][1+j] + B_shadow[1+i][j] + B_shadow[i-1][j]);
      }
  }
#pragma endscop
}

static
void print_norm_error_base(int n,
                           DATA_TYPE POLYBENCH_2D(A,N,N,n,n),
                           double POLYBENCH_2D(A_shadow,N,N,n,n))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  double max_shadow = 0.0;
  double sum_shadow = 0.0;
  double norm_shadow = 0.0;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      DATA_TYPE value = A[i][j];
      double value_shadow = A_shadow[i][j];

      if (value < 0)
        value = -value;
      if (value_shadow < 0.0)
        value_shadow = -value_shadow;

      if (value > max_value) {
        max_value = value;
        max_shadow = value_shadow;
      }
    }

  if (max_value != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
        DATA_TYPE scaled = A[i][j] / max_value;
        double scaled_shadow = A_shadow[i][j] / max_shadow;

        sum += scaled * scaled;
        sum_shadow += scaled_shadow * scaled_shadow;
      }
    norm = SQRT_FUN(sum);
    norm_shadow = sqrt(sum_shadow);
  }

  printf("Max value in A: %.7e\n", max_value);
  printf("Norm of A: %.7e\n", norm);
  printf("FPChecker-style shadow norm: %.17e\n", norm_shadow);
  printf("FPChecker-style norm error: %.17e\n", norm_shadow - (double) norm);
}

int main(int argc, char** argv)
{
  int n = N;
  int tsteps = TSTEPS;
  (void) argc;
  (void) argv;

  POLYBENCH_2D_ARRAY_DECL(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(B, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(A_shadow, double, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(B_shadow, double, N, N, n, n);

  init_array_base(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B),
                  POLYBENCH_ARRAY(A_shadow), POLYBENCH_ARRAY(B_shadow));
  kernel_jacobi_2d_base(tsteps, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B),
                        POLYBENCH_ARRAY(A_shadow), POLYBENCH_ARRAY(B_shadow));
  print_norm_error_base(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_shadow));

  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(A_shadow);
  POLYBENCH_FREE_ARRAY(B_shadow);

  return 0;
}
