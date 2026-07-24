/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* durbin.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mpfr.h>

#include <polybench.h>
#include "durbin.h"

#define PREC_BITS 500


/* Array initialization. */
static
void init_array (int n,
		 DATA_TYPE POLYBENCH_1D(r,N,n))
{
  int i;

  for (i = 0; i < n; i++)
    {
      r[i] = (n+1-i);
    }
}


static
void init_array_mpfr (int n,
		 mpfr_t POLYBENCH_1D(r,N,n))
{
  int i;

  for (i = 0; i < n; i++)
    {
      mpfr_set_si (r[i], n+1-i, MPFR_RNDN);
    }
}


/* DCE code. Must scan the entire live-out data. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D(y_fp32,N,n),
		 mpfr_t POLYBENCH_1D(y_mpfr,N,n))

{
  int i;

  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm_fp32 = 0;

  mpfr_t max_mpfr, sum_mpfr, norm_mpfr, scaled, absval;
  mpfr_t norm_fp32_m, abs_err;
  mpfr_init2 (max_mpfr, PREC_BITS);
  mpfr_init2 (sum_mpfr, PREC_BITS);
  mpfr_init2 (norm_mpfr, PREC_BITS);
  mpfr_init2 (scaled, PREC_BITS);
  mpfr_init2 (absval, PREC_BITS);
  mpfr_init2 (norm_fp32_m, PREC_BITS);
  mpfr_init2 (abs_err, PREC_BITS);
  mpfr_set_si (max_mpfr, 0, MPFR_RNDN);
  mpfr_set_si (sum_mpfr, 0, MPFR_RNDN);
  mpfr_set_si (norm_mpfr, 0, MPFR_RNDN);

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("y");
  for (i = 0; i < n; i++) {
    DATA_TYPE value = y_fp32[i];

    if (value < 0)
      value = -value;

    if (value > max_value)
      max_value = value;

    mpfr_abs (absval, y_mpfr[i], MPFR_RNDN);
    if (mpfr_cmp (absval, max_mpfr) > 0)
      mpfr_set (max_mpfr, absval, MPFR_RNDN);
  }

  if (max_value != 0) {
    for (i = 0; i < n; i++) {
      DATA_TYPE scaled_fp32 = y_fp32[i] / max_value;
      sum += scaled_fp32 * scaled_fp32;
    }
    norm_fp32 = SQRT_FUN(sum);
  }

  if (mpfr_cmp_si (max_mpfr, 0) != 0) {
    for (i = 0; i < n; i++) {
      mpfr_div (scaled, y_mpfr[i], max_mpfr, MPFR_RNDN);
      mpfr_mul (scaled, scaled, scaled, MPFR_RNDN);
      mpfr_add (sum_mpfr, sum_mpfr, scaled, MPFR_RNDN);
    }
    mpfr_sqrt (norm_mpfr, sum_mpfr, MPFR_RNDN);
  }

  mpfr_set_flt (norm_fp32_m, norm_fp32, MPFR_RNDN);
  mpfr_sub (abs_err, norm_mpfr, norm_fp32_m, MPFR_RNDN);
  mpfr_abs (abs_err, abs_err, MPFR_RNDN);

  fprintf (POLYBENCH_DUMP_TARGET, "Norm (fp32) : %.7f\n", norm_fp32);
  fprintf (POLYBENCH_DUMP_TARGET, "Norm (mpfr) : ");
  mpfr_out_str (POLYBENCH_DUMP_TARGET, 10, 20, norm_mpfr, MPFR_RNDN);
  fprintf (POLYBENCH_DUMP_TARGET, "\n");
  fprintf (POLYBENCH_DUMP_TARGET, "Abs error   : ");
  mpfr_out_str (POLYBENCH_DUMP_TARGET, 10, 20, abs_err, MPFR_RNDN);
  fprintf (POLYBENCH_DUMP_TARGET, "\n");

  mpfr_clear (max_mpfr);
  mpfr_clear (sum_mpfr);
  mpfr_clear (norm_mpfr);
  mpfr_clear (scaled);
  mpfr_clear (absval);
  mpfr_clear (norm_fp32_m);
  mpfr_clear (abs_err);

  POLYBENCH_DUMP_END("y");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_durbin(int n,
		   DATA_TYPE POLYBENCH_1D(r,N,n),
		   DATA_TYPE POLYBENCH_1D(y,N,n))
{
 DATA_TYPE z[N];
 DATA_TYPE alpha;
 DATA_TYPE beta;
 DATA_TYPE sum;

 int i,k;

#pragma scop
 y[0] = -r[0];
 beta = SCALAR_VAL(1.0);
 alpha = -r[0];

 for (k = 1; k < _PB_N; k++) {
   beta = (1-alpha*alpha)*beta;
   sum = SCALAR_VAL(0.0);
   for (i=0; i<k; i++) {
      sum += r[k-i-1]*y[i];
   }
   alpha = - (r[k] + sum)/beta;

   for (i=0; i<k; i++) {
      z[i] = y[i] + alpha*y[k-i-1];
   }
   for (i=0; i<k; i++) {
     y[i] = z[i];
   }
   y[k] = alpha;
 }
#pragma endscop

}


static
void kernel_durbin_mpfr(int n,
		   mpfr_t POLYBENCH_1D(r,N,n),
		   mpfr_t POLYBENCH_1D(y,N,n))
{
 mpfr_t z[N];
 mpfr_t alpha;
 mpfr_t beta;
 mpfr_t sum;
 mpfr_t tmp;

 int i,k;

 for (i = 0; i < N; i++)
   mpfr_init2 (z[i], PREC_BITS);
 mpfr_init2 (alpha, PREC_BITS);
 mpfr_init2 (beta, PREC_BITS);
 mpfr_init2 (sum, PREC_BITS);
 mpfr_init2 (tmp, PREC_BITS);

#pragma scop
 mpfr_neg (y[0], r[0], MPFR_RNDN);
 mpfr_set_si (beta, 1, MPFR_RNDN);
 mpfr_neg (alpha, r[0], MPFR_RNDN);

 for (k = 1; k < _PB_N; k++) {
   mpfr_mul (tmp, alpha, alpha, MPFR_RNDN);
   mpfr_si_sub (tmp, 1, tmp, MPFR_RNDN);
   mpfr_mul (beta, tmp, beta, MPFR_RNDN);
   mpfr_set_si (sum, 0, MPFR_RNDN);
   for (i=0; i<k; i++) {
      mpfr_mul (tmp, r[k-i-1], y[i], MPFR_RNDN);
      mpfr_add (sum, sum, tmp, MPFR_RNDN);
   }
   mpfr_add (tmp, r[k], sum, MPFR_RNDN);
   mpfr_neg (tmp, tmp, MPFR_RNDN);
   mpfr_div (alpha, tmp, beta, MPFR_RNDN);

   for (i=0; i<k; i++) {
      mpfr_mul (tmp, alpha, y[k-i-1], MPFR_RNDN);
      mpfr_add (z[i], y[i], tmp, MPFR_RNDN);
   }
   for (i=0; i<k; i++) {
     mpfr_set (y[i], z[i], MPFR_RNDN);
   }
   mpfr_set (y[k], alpha, MPFR_RNDN);
 }
#pragma endscop

 for (i = 0; i < N; i++)
   mpfr_clear (z[i]);
 mpfr_clear (alpha);
 mpfr_clear (beta);
 mpfr_clear (sum);
 mpfr_clear (tmp);
}


int main(int argc, char** argv)
{
  int n = N;

  POLYBENCH_1D_ARRAY_DECL(r, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL(y, DATA_TYPE, N, n);

  POLYBENCH_1D_ARRAY_DECL(r_mpfr, mpfr_t, N, n);
  POLYBENCH_1D_ARRAY_DECL(y_mpfr, mpfr_t, N, n);

  mpfr_set_default_prec (PREC_BITS);
  {
    int i;
    for (i = 0; i < N; i++) {
      mpfr_init2 ((*r_mpfr)[i], PREC_BITS);
      mpfr_init2 ((*y_mpfr)[i], PREC_BITS);
    }
  }

  init_array (n, POLYBENCH_ARRAY(r));
  init_array_mpfr (n, POLYBENCH_ARRAY(r_mpfr));

  polybench_start_instruments;

  kernel_durbin (n,
		 POLYBENCH_ARRAY(r),
		 POLYBENCH_ARRAY(y));
  kernel_durbin_mpfr (n,
		 POLYBENCH_ARRAY(r_mpfr),
		 POLYBENCH_ARRAY(y_mpfr));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(y), POLYBENCH_ARRAY(y_mpfr)));

  {
    int i;
    for (i = 0; i < N; i++) {
      mpfr_clear ((*r_mpfr)[i]);
      mpfr_clear ((*y_mpfr)[i]);
    }
    mpfr_free_cache ();
  }

  POLYBENCH_FREE_ARRAY(r);
  POLYBENCH_FREE_ARRAY(y);
  POLYBENCH_FREE_ARRAY(r_mpfr);
  POLYBENCH_FREE_ARRAY(y_mpfr);

  return 0;
}
