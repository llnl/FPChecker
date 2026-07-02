#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <polybench.h>
#include "jacobi-1d.h"

static
void init_array_base(int n,
                     DATA_TYPE POLYBENCH_1D(A,N,n),
                     DATA_TYPE POLYBENCH_1D(B,N,n),
                     double POLYBENCH_1D(A_shadow,N,n),
                     double POLYBENCH_1D(B_shadow,N,n))
{
  int i;
  double n_shadow = (double)(DATA_TYPE)n;

  for (i = 0; i < n; i++) {
    A[i] = ((DATA_TYPE) i + SCALAR_VAL(2.0)) / n;
    B[i] = ((DATA_TYPE) i + SCALAR_VAL(3.0)) / n;

    A_shadow[i] = ((double)(DATA_TYPE) i + (double) SCALAR_VAL(2.0)) / n_shadow;
    B_shadow[i] = ((double)(DATA_TYPE) i + (double) SCALAR_VAL(3.0)) / n_shadow;
  }
}

static
void kernel_jacobi_1d(int tsteps, int n,
                           DATA_TYPE POLYBENCH_1D(A,N,n),
                           DATA_TYPE POLYBENCH_1D(B,N,n),
                           double POLYBENCH_1D(A_shadow,N,n),
                           double POLYBENCH_1D(B_shadow,N,n))
{
  int t, i;
  double c_shadow = (double) SCALAR_VAL(0.33333);

#pragma scop
  for (t = 0; t < _PB_TSTEPS; t++) {
    for (i = 1; i < _PB_N - 1; i++) {
      B[i] = SCALAR_VAL(0.33333) * (A[i-1] + A[i] + A[i + 1]);
      B_shadow[i] = c_shadow * (A_shadow[i-1] + A_shadow[i] + A_shadow[i + 1]);
    }
    for (i = 1; i < _PB_N - 1; i++) {
      A[i] = SCALAR_VAL(0.33333) * (B[i-1] + B[i] + B[i + 1]);
      A_shadow[i] = c_shadow * (B_shadow[i-1] + B_shadow[i] + B_shadow[i + 1]);
    }
  }
#pragma endscop
}

static
void print_array(int n,
                           DATA_TYPE POLYBENCH_1D(A,N,n),
                           double POLYBENCH_1D(A_shadow,N,n))
{
  int i;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  double max_shadow = 0.0;
  double sum_shadow = 0.0;
  double norm_shadow = 0.0;

  for (i = 0; i < n; i++) {
    DATA_TYPE value = A[i];
    double value_shadow = A_shadow[i];

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
    for (i = 0; i < n; i++) {
      DATA_TYPE scaled = A[i] / max_value;
      double scaled_shadow = A_shadow[i] / max_shadow;

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
  // (void) argc;
  // (void) argv;

  POLYBENCH_1D_ARRAY_DECL(A, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(B, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(A_shadow, double, N, n);
  POLYBENCH_1D_ARRAY_DECL(B_shadow, double, N, n);

  init_array_base(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B),
                  POLYBENCH_ARRAY(A_shadow), POLYBENCH_ARRAY(B_shadow));
  kernel_jacobi_1d(tsteps, n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B),
                        POLYBENCH_ARRAY(A_shadow), POLYBENCH_ARRAY(B_shadow));
  print_array(n, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(A_shadow));

  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(A_shadow);
  POLYBENCH_FREE_ARRAY(B_shadow);

  return 0;
}
