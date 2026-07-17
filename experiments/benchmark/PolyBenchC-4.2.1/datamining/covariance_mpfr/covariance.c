/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* covariance.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mpfr.h>

#include <polybench.h>
#include "covariance.h"

#define PREC_BITS 500

/* --------------------- FP32 Array initialization. ----------------*/

static
void init_array (int m, int n,
		 DATA_TYPE *float_n,
		 DATA_TYPE POLYBENCH_2D(data,N,M,n,m))
{
  int i, j;

  *float_n = (DATA_TYPE)n;

  for (i = 0; i < N; i++)
    for (j = 0; j < M; j++)
      data[i][j] = ((DATA_TYPE) i*j) / M;
}

/* --------------------- MPFR initialization ---------------------- */
static
void init_array_mpfr (int m, int n,
		 mpfr_t float_n,
		 mpfr_t POLYBENCH_2D(data,N,M,n,m))
{
  int i, j;
 
  mpfr_set_si (float_n, n, MPFR_RNDN);

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; j++){
      mpfr_set_si (data[i][j], i*j, MPFR_RNDN);
      mpfr_div_si (data[i][j], data[i][j], M, MPFR_RNDN);
    }
  }
}

/* --------------- FP32 and MPFR array printing --------------- */
static
void print_array(int m,
		 DATA_TYPE POLYBENCH_2D(cov_fp32,M,M,m,m),
		 mpfr_t POLYBENCH_2D(cov_mpfr,M,M,m,m))
{
  int i, j;
  
  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("cov");

  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm_fp32 = 0;
 
  for (i = 0; i < m; i++)
    for (j = 0; j < m; j++) {
      DATA_TYPE value = cov_fp32[i][j];
      if (value < 0) 
        value = -value;

      if (value > max_value) 
        max_value = value;
    }
 
  if (max_value != 0) {
    for (i = 0; i < m; i++)
      for (j = 0; j < m; j++) {
        DATA_TYPE scaled = cov_fp32[i][j] / max_value;
        sum += scaled * scaled;
      }
    norm_fp32 = SQRT_FUN(sum);
  }

  /* -------------------- MPFR norm in 500-bit precision -------- */
  mpfr_t max_mpfr, sum_mpfr, norm_mpfr, scaled, absval;
  mpfr_init2 (max_mpfr,  PREC_BITS);
  mpfr_init2 (sum_mpfr,  PREC_BITS);
  mpfr_init2 (norm_mpfr, PREC_BITS);
  mpfr_init2 (scaled,    PREC_BITS);
  mpfr_init2 (absval,    PREC_BITS);
  mpfr_set_si (max_mpfr,  0, MPFR_RNDN);
  mpfr_set_si (sum_mpfr,  0, MPFR_RNDN);
  mpfr_set_si (norm_mpfr, 0, MPFR_RNDN);
 
  for (i = 0; i < m; i++){
    for (j = 0; j < m; j++) {
      mpfr_abs (absval, cov_mpfr[i][j], MPFR_RNDN);
      if (mpfr_cmp (absval, max_mpfr) > 0)
        mpfr_set (max_mpfr, absval, MPFR_RNDN);
    }
  }
  if (mpfr_cmp_si (max_mpfr, 0) != 0) {
    for (i = 0; i < m; i++)
      for (j = 0; j < m; j++) {
        mpfr_div (scaled, cov_mpfr[i][j], max_mpfr, MPFR_RNDN);
        mpfr_mul (scaled, scaled, scaled, MPFR_RNDN);
        mpfr_add (sum_mpfr, sum_mpfr, scaled, MPFR_RNDN);
      }
    mpfr_sqrt (norm_mpfr, sum_mpfr, MPFR_RNDN);
  }
 

  // fprintf (POLYBENCH_DUMP_TARGET, "Max value in cov: %.7e\n", max_value);
  // fprintf (POLYBENCH_DUMP_TARGET, "Norm of cov: %.7e\n", norm);
  // fprintf (POLYBENCH_DUMP_TARGET, "Max value in cov_double: %.17e\n", max_value_double);
  // fprintf (POLYBENCH_DUMP_TARGET, "Norm of cov_double: %.17e\n", norm_double);

  // double norm_error = norm_double - (double)norm;
  // fprintf (POLYBENCH_DUMP_TARGET, "Norm error: %.17e\n", norm_error);

  /* ----------- FP32 norm, MPFR norm, and their difference --- */
 
  fprintf (POLYBENCH_DUMP_TARGET, "Norm (fp32) : %.7f\n", norm_fp32);
 
  fprintf (POLYBENCH_DUMP_TARGET, "Norm (mpfr) : ");
  mpfr_out_str (POLYBENCH_DUMP_TARGET, 10, 20 , norm_mpfr, MPFR_RNDN);
  fprintf (POLYBENCH_DUMP_TARGET, "\n");
 
  /* ------------ Absolute error = |norm_mpfr - norm_fp32|, computed in MPFR --- */
  mpfr_t norm_fp32_m, abs_err;
  mpfr_init2 (norm_fp32_m, PREC_BITS);
  mpfr_init2 (abs_err,     PREC_BITS);
 
  // mpfr_set_d (norm_fp32_m, (double)norm_fp32, MPFR_RNDN);  // for double 
  mpfr_set_flt (norm_fp32_m, norm_fp32, MPFR_RNDN);  // for float
  mpfr_sub (abs_err, norm_mpfr, norm_fp32_m, MPFR_RNDN);   
  mpfr_abs (abs_err, abs_err, MPFR_RNDN);
 
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

	  POLYBENCH_DUMP_END("cov");
	  POLYBENCH_DUMP_FINISH;
	}

/* ---------------------- fp32 kernel ------------------------------ */
static
void kernel_covariance(int m, int n,
		       DATA_TYPE float_n,
		       DATA_TYPE POLYBENCH_2D(data,N,M,n,m),
		       DATA_TYPE POLYBENCH_2D(cov,M,M,m,m),
		       DATA_TYPE POLYBENCH_1D(mean,M,m))
{
  int i, j, k;

#pragma scop
  for (j = 0; j < _PB_M; j++)
    {
      mean[j] = SCALAR_VAL(0.0);
      for (i = 0; i < _PB_N; i++)
        mean[j] += data[i][j];
      mean[j] /= float_n;
    }

  for (i = 0; i < _PB_N; i++)
    for (j = 0; j < _PB_M; j++)
      data[i][j] -= mean[j];

  for (i = 0; i < _PB_M; i++)
    for (j = i; j < _PB_M; j++)
      {
        cov[i][j] = SCALAR_VAL(0.0);
        for (k = 0; k < _PB_N; k++)
	  cov[i][j] += data[k][i] * data[k][j];
        cov[i][j] /= (float_n - SCALAR_VAL(1.0));
        cov[j][i] = cov[i][j];
      }
#pragma endscop
}

/* -------------------------- MPFR kernel  ---------------------- */
static
void kernel_covariance_mpfr(int m, int n,
		       mpfr_t float_n,
		       mpfr_t POLYBENCH_2D(data,N,M,n,m),
		       mpfr_t POLYBENCH_2D(cov,M,M,m,m),
		       mpfr_t POLYBENCH_1D(mean,M,m))
{
  int i, j, k;

  mpfr_t tmp, denom;
  mpfr_init2 (tmp,   PREC_BITS);
  mpfr_init2 (denom, PREC_BITS);

#pragma scop
  for (j = 0; j < _PB_M; j++)
    {
      // mean[j] = 0.0;
      mpfr_set_si (mean[j], 0, MPFR_RNDN);
      for (i = 0; i < _PB_N; i++)
        // mean[j] += data[i][j];
      mpfr_add (mean[j], mean[j], data[i][j], MPFR_RNDN);
      // mean[j] /= float_n;
      mpfr_div (mean[j], mean[j], float_n, MPFR_RNDN);
    }

  for (i = 0; i < _PB_N; i++) {
    for (j = 0; j < _PB_M; j++) {
      // data[i][j] -= mean[j];
      mpfr_sub (data[i][j], data[i][j], mean[j], MPFR_RNDN);
    }
  }

  mpfr_sub_si (denom, float_n, 1, MPFR_RNDN);  

  for (i = 0; i < _PB_M; i++) {
    for (j = i; j < _PB_M; j++) {
      {
        // cov[i][j] = 0.0;
        mpfr_set_si (cov[i][j], 0, MPFR_RNDN);
        for (k = 0; k < _PB_N; k++) {
          // cov[i][j] += data[k][i] * data[k][j];
          mpfr_mul (tmp, data[k][i], data[k][j], MPFR_RNDN);
          mpfr_add (cov[i][j], cov[i][j], tmp, MPFR_RNDN);
        }

        // cov[i][j] /= (float_n - 1.0);
        mpfr_div (cov[i][j], cov[i][j], denom, MPFR_RNDN);
        // cov[j][i] = cov[i][j];
        mpfr_set (cov[j][i], cov[i][j], MPFR_RNDN);
      }
    }
  }
#pragma endscop
  mpfr_clear (tmp);
  mpfr_clear (denom);
}

int main(int argc, char** argv)
{
 
  int n = N;
  int m = M;
 
  /* fp32 side. */
  DATA_TYPE float_n;
  POLYBENCH_2D_ARRAY_DECL(data,DATA_TYPE,N,M,n,m);
  POLYBENCH_2D_ARRAY_DECL(cov,DATA_TYPE,M,M,m,m);
  POLYBENCH_1D_ARRAY_DECL(mean,DATA_TYPE,M,m);
 
  /* MPFR side. */
  mpfr_t float_n_mpfr;
  POLYBENCH_2D_ARRAY_DECL(data_mpfr,mpfr_t,N,M,n,m);
  POLYBENCH_2D_ARRAY_DECL(cov_mpfr,mpfr_t,M,M,m,m);
  POLYBENCH_1D_ARRAY_DECL(mean_mpfr,mpfr_t,M,m);
 
  /* Initialize every MPFR cell to 500 bits. */
  mpfr_set_default_prec (PREC_BITS);
  mpfr_init2 (float_n_mpfr, PREC_BITS);
  {
    int i, j;
    for (i = 0; i < N; i++) for (j = 0; j < M; j++) mpfr_init2 ((*data_mpfr)[i][j], PREC_BITS);
    for (i = 0; i < M; i++) for (j = 0; j < M; j++) mpfr_init2 ((*cov_mpfr)[i][j],  PREC_BITS);
    for (i = 0; i < M; i++)                          mpfr_init2 ((*mean_mpfr)[i],    PREC_BITS);
  }
 
  init_array      (m, n, &float_n,      POLYBENCH_ARRAY(data));
  init_array_mpfr (m, n, float_n_mpfr,  POLYBENCH_ARRAY(data_mpfr));
 
  polybench_start_instruments;
 
  kernel_covariance      (m, n, float_n,
			  POLYBENCH_ARRAY(data),
			  POLYBENCH_ARRAY(cov),
			  POLYBENCH_ARRAY(mean));
  kernel_covariance_mpfr (m, n, float_n_mpfr,
			  POLYBENCH_ARRAY(data_mpfr),
			  POLYBENCH_ARRAY(cov_mpfr),
			  POLYBENCH_ARRAY(mean_mpfr));
 
  polybench_stop_instruments;
  polybench_print_instruments;
 
  polybench_prevent_dce(print_array(m, POLYBENCH_ARRAY(cov), POLYBENCH_ARRAY(cov_mpfr)));
 
  /* Clear MPFR cells before freeing backing storage. */
  {
    int i, j;
    for (i = 0; i < N; i++) for (j = 0; j < M; j++) mpfr_clear ((*data_mpfr)[i][j]);
    for (i = 0; i < M; i++) for (j = 0; j < M; j++) mpfr_clear ((*cov_mpfr)[i][j]);
    for (i = 0; i < M; i++)                          mpfr_clear ((*mean_mpfr)[i]);
    mpfr_clear (float_n_mpfr);
    mpfr_free_cache ();
  }
 
  POLYBENCH_FREE_ARRAY(data);
  POLYBENCH_FREE_ARRAY(cov);
  POLYBENCH_FREE_ARRAY(mean);
  POLYBENCH_FREE_ARRAY(data_mpfr);
  POLYBENCH_FREE_ARRAY(cov_mpfr);
  POLYBENCH_FREE_ARRAY(mean_mpfr);
 
  return 0;
}
