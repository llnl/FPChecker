/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* adi.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mpfr.h>

#include <polybench.h>
#include "adi.h"

#define PREC_BITS 500


/* Array initialization. */
static
void init_array (int n,
		 DATA_TYPE POLYBENCH_2D(u,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      u[i][j] =  (DATA_TYPE)(i + n-j) / n;
}


static
void init_array_mpfr (int n,
                 mpfr_t POLYBENCH_2D(u,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      mpfr_set_si (u[i][j], i + n - j, MPFR_RNDN);
      mpfr_div_si (u[i][j], u[i][j], n, MPFR_RNDN);
    }
}


static
void print_array(int n,
                 DATA_TYPE POLYBENCH_2D(u_fp32,N,N,n,n),
                 mpfr_t POLYBENCH_2D(u_mpfr,N,N,n,n))
{
  int i, j;
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
  POLYBENCH_DUMP_BEGIN("u");
  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      DATA_TYPE value = u_fp32[i][j];

      if (value < 0)
        value = -value;

      if (value > max_value)
        max_value = value;

      mpfr_abs (absval, u_mpfr[i][j], MPFR_RNDN);
      if (mpfr_cmp (absval, max_mpfr) > 0)
        mpfr_set (max_mpfr, absval, MPFR_RNDN);
    }

  if (max_value != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
        DATA_TYPE scaled_fp32 = u_fp32[i][j] / max_value;
        sum += scaled_fp32 * scaled_fp32;
      }
    norm_fp32 = SQRT_FUN(sum);
  }

  if (mpfr_cmp_si (max_mpfr, 0) != 0) {
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
        mpfr_div (scaled, u_mpfr[i][j], max_mpfr, MPFR_RNDN);
        mpfr_mul (scaled, scaled, scaled, MPFR_RNDN);
        mpfr_add (sum_mpfr, sum_mpfr, scaled, MPFR_RNDN);
      }
    mpfr_sqrt (norm_mpfr, sum_mpfr, MPFR_RNDN);
  }

  mpfr_set_flt (norm_fp32_m, norm_fp32, MPFR_RNDN);
  mpfr_sub (abs_err, norm_mpfr, norm_fp32_m, MPFR_RNDN);
  mpfr_abs (abs_err, abs_err, MPFR_RNDN);

  fprintf(POLYBENCH_DUMP_TARGET, "Norm (fp32) : %.7f\n", norm_fp32);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm (mpfr) : ");
  mpfr_out_str (POLYBENCH_DUMP_TARGET, 10, 20, norm_mpfr, MPFR_RNDN);
  fprintf(POLYBENCH_DUMP_TARGET, "\n");
  fprintf(POLYBENCH_DUMP_TARGET, "Abs error   : ");
  mpfr_out_str (POLYBENCH_DUMP_TARGET, 10, 20, abs_err, MPFR_RNDN);
  fprintf(POLYBENCH_DUMP_TARGET, "\n");

  mpfr_clear (max_mpfr);
  mpfr_clear (sum_mpfr);
  mpfr_clear (norm_mpfr);
  mpfr_clear (scaled);
  mpfr_clear (absval);
  mpfr_clear (norm_fp32_m);
  mpfr_clear (abs_err);

  POLYBENCH_DUMP_END("u");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
/* Based on a Fortran code fragment from Figure 5 of
 * "Automatic Data and Computation Decomposition on Distributed Memory Parallel Computers"
 * by Peizong Lee and Zvi Meir Kedem, TOPLAS, 2002
 */
static
void kernel_adi(int tsteps, int n,
		DATA_TYPE POLYBENCH_2D(u,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(v,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(p,N,N,n,n),
		DATA_TYPE POLYBENCH_2D(q,N,N,n,n))
{
  int t, i, j;
  DATA_TYPE DX, DY, DT;
  DATA_TYPE B1, B2;
  DATA_TYPE mul1, mul2;
  DATA_TYPE a, b, c, d, e, f;

#pragma scop

  DX = SCALAR_VAL(1.0)/(DATA_TYPE)_PB_N;
  DY = SCALAR_VAL(1.0)/(DATA_TYPE)_PB_N;
  DT = SCALAR_VAL(1.0)/(DATA_TYPE)_PB_TSTEPS;
  B1 = SCALAR_VAL(2.0);
  B2 = SCALAR_VAL(1.0);
  mul1 = B1 * DT / (DX * DX);
  mul2 = B2 * DT / (DY * DY);

  a = -mul1 /  SCALAR_VAL(2.0);
  b = SCALAR_VAL(1.0)+mul1;
  c = a;
  d = -mul2 / SCALAR_VAL(2.0);
  e = SCALAR_VAL(1.0)+mul2;
  f = d;

 for (t=1; t<=_PB_TSTEPS; t++) {
    for (i=1; i<_PB_N-1; i++) {
      v[0][i] = SCALAR_VAL(1.0);
      p[i][0] = SCALAR_VAL(0.0);
      q[i][0] = v[0][i];
      for (j=1; j<_PB_N-1; j++) {
        p[i][j] = -c / (a*p[i][j-1]+b);
        q[i][j] = (-d*u[j][i-1]+(SCALAR_VAL(1.0)+SCALAR_VAL(2.0)*d)*u[j][i] - f*u[j][i+1]-a*q[i][j-1])/(a*p[i][j-1]+b);
      }

      v[_PB_N-1][i] = SCALAR_VAL(1.0);
      for (j=_PB_N-2; j>=1; j--) {
        v[j][i] = p[i][j] * v[j+1][i] + q[i][j];
      }
    }
    for (i=1; i<_PB_N-1; i++) {
      u[i][0] = SCALAR_VAL(1.0);
      p[i][0] = SCALAR_VAL(0.0);
      q[i][0] = u[i][0];
      for (j=1; j<_PB_N-1; j++) {
        p[i][j] = -f / (d*p[i][j-1]+e);
        q[i][j] = (-a*v[i-1][j]+(SCALAR_VAL(1.0)+SCALAR_VAL(2.0)*a)*v[i][j] - c*v[i+1][j]-d*q[i][j-1])/(d*p[i][j-1]+e);
      }
      u[i][_PB_N-1] = SCALAR_VAL(1.0);
      for (j=_PB_N-2; j>=1; j--) {
        u[i][j] = p[i][j] * u[i][j+1] + q[i][j];
      }
    }
  }
#pragma endscop
}


static
void kernel_adi_mpfr(int tsteps, int n,
                mpfr_t POLYBENCH_2D(u,N,N,n,n),
                mpfr_t POLYBENCH_2D(v,N,N,n,n),
                mpfr_t POLYBENCH_2D(p,N,N,n,n),
                mpfr_t POLYBENCH_2D(q,N,N,n,n))
{
  int t, i, j;
  mpfr_t DX, DY, DT;
  mpfr_t B1, B2;
  mpfr_t mul1, mul2;
  mpfr_t a, b, c, d, e, f;
  mpfr_t denom, tmp1, tmp2, tmp3, tmp4;

  mpfr_inits2(PREC_BITS, DX, DY, DT, B1, B2, mul1, mul2,
              a, b, c, d, e, f, denom, tmp1, tmp2, tmp3, tmp4,
              (mpfr_ptr) 0);

#pragma scop

  mpfr_set_si(DX, _PB_N, MPFR_RNDN);
  mpfr_ui_div(DX, 1, DX, MPFR_RNDN);
  mpfr_set_si(DY, _PB_N, MPFR_RNDN);
  mpfr_ui_div(DY, 1, DY, MPFR_RNDN);
  mpfr_set_si(DT, _PB_TSTEPS, MPFR_RNDN);
  mpfr_ui_div(DT, 1, DT, MPFR_RNDN);
  mpfr_set_si(B1, 2, MPFR_RNDN);
  mpfr_set_si(B2, 1, MPFR_RNDN);
  mpfr_mul(tmp1, DX, DX, MPFR_RNDN);
  mpfr_mul(mul1, B1, DT, MPFR_RNDN);
  mpfr_div(mul1, mul1, tmp1, MPFR_RNDN);
  mpfr_mul(tmp1, DY, DY, MPFR_RNDN);
  mpfr_mul(mul2, B2, DT, MPFR_RNDN);
  mpfr_div(mul2, mul2, tmp1, MPFR_RNDN);

  mpfr_div_si(a, mul1, -2, MPFR_RNDN);
  mpfr_add_si(b, mul1, 1, MPFR_RNDN);
  mpfr_set(c, a, MPFR_RNDN);
  mpfr_div_si(d, mul2, -2, MPFR_RNDN);
  mpfr_add_si(e, mul2, 1, MPFR_RNDN);
  mpfr_set(f, d, MPFR_RNDN);

 for (t=1; t<=_PB_TSTEPS; t++) {
    for (i=1; i<_PB_N-1; i++) {
      mpfr_set_si(v[0][i], 1, MPFR_RNDN);
      mpfr_set_si(p[i][0], 0, MPFR_RNDN);
      mpfr_set(q[i][0], v[0][i], MPFR_RNDN);
      for (j=1; j<_PB_N-1; j++) {
        mpfr_mul(denom, a, p[i][j-1], MPFR_RNDN);
        mpfr_add(denom, denom, b, MPFR_RNDN);
        mpfr_neg(tmp1, c, MPFR_RNDN);
        mpfr_div(p[i][j], tmp1, denom, MPFR_RNDN);

        mpfr_mul(tmp1, d, u[j][i-1], MPFR_RNDN);
        mpfr_neg(tmp1, tmp1, MPFR_RNDN);
        mpfr_mul_si(tmp2, d, 2, MPFR_RNDN);
        mpfr_add_si(tmp2, tmp2, 1, MPFR_RNDN);
        mpfr_mul(tmp2, tmp2, u[j][i], MPFR_RNDN);
        mpfr_add(tmp1, tmp1, tmp2, MPFR_RNDN);
        mpfr_mul(tmp3, f, u[j][i+1], MPFR_RNDN);
        mpfr_sub(tmp1, tmp1, tmp3, MPFR_RNDN);
        mpfr_mul(tmp4, a, q[i][j-1], MPFR_RNDN);
        mpfr_sub(tmp1, tmp1, tmp4, MPFR_RNDN);
        mpfr_div(q[i][j], tmp1, denom, MPFR_RNDN);
      }

      mpfr_set_si(v[_PB_N-1][i], 1, MPFR_RNDN);
      for (j=_PB_N-2; j>=1; j--) {
        mpfr_mul(tmp1, p[i][j], v[j+1][i], MPFR_RNDN);
        mpfr_add(v[j][i], tmp1, q[i][j], MPFR_RNDN);
      }
    }
    for (i=1; i<_PB_N-1; i++) {
      mpfr_set_si(u[i][0], 1, MPFR_RNDN);
      mpfr_set_si(p[i][0], 0, MPFR_RNDN);
      mpfr_set(q[i][0], u[i][0], MPFR_RNDN);
      for (j=1; j<_PB_N-1; j++) {
        mpfr_mul(denom, d, p[i][j-1], MPFR_RNDN);
        mpfr_add(denom, denom, e, MPFR_RNDN);
        mpfr_neg(tmp1, f, MPFR_RNDN);
        mpfr_div(p[i][j], tmp1, denom, MPFR_RNDN);

        mpfr_mul(tmp1, a, v[i-1][j], MPFR_RNDN);
        mpfr_neg(tmp1, tmp1, MPFR_RNDN);
        mpfr_mul_si(tmp2, a, 2, MPFR_RNDN);
        mpfr_add_si(tmp2, tmp2, 1, MPFR_RNDN);
        mpfr_mul(tmp2, tmp2, v[i][j], MPFR_RNDN);
        mpfr_add(tmp1, tmp1, tmp2, MPFR_RNDN);
        mpfr_mul(tmp3, c, v[i+1][j], MPFR_RNDN);
        mpfr_sub(tmp1, tmp1, tmp3, MPFR_RNDN);
        mpfr_mul(tmp4, d, q[i][j-1], MPFR_RNDN);
        mpfr_sub(tmp1, tmp1, tmp4, MPFR_RNDN);
        mpfr_div(q[i][j], tmp1, denom, MPFR_RNDN);
      }
      mpfr_set_si(u[i][_PB_N-1], 1, MPFR_RNDN);
      for (j=_PB_N-2; j>=1; j--) {
        mpfr_mul(tmp1, p[i][j], u[i][j+1], MPFR_RNDN);
        mpfr_add(u[i][j], tmp1, q[i][j], MPFR_RNDN);
      }
    }
  }
#pragma endscop

  mpfr_clears(DX, DY, DT, B1, B2, mul1, mul2,
              a, b, c, d, e, f, denom, tmp1, tmp2, tmp3, tmp4,
              (mpfr_ptr) 0);
}


int main(int argc, char** argv)
{
  int n = N;
  int tsteps = TSTEPS;

  POLYBENCH_2D_ARRAY_DECL(u, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(v, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(p, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(q, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(u_mpfr, mpfr_t, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(v_mpfr, mpfr_t, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(p_mpfr, mpfr_t, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL(q_mpfr, mpfr_t, N, N, n, n);

  mpfr_set_default_prec(PREC_BITS);
  {
    int i, j;
    for (i = 0; i < N; i++)
      for (j = 0; j < N; j++) {
        mpfr_init2((*u_mpfr)[i][j], PREC_BITS);
        mpfr_init2((*v_mpfr)[i][j], PREC_BITS);
        mpfr_init2((*p_mpfr)[i][j], PREC_BITS);
        mpfr_init2((*q_mpfr)[i][j], PREC_BITS);
      }
  }

  init_array (n, POLYBENCH_ARRAY(u));
  init_array_mpfr (n, POLYBENCH_ARRAY(u_mpfr));

  polybench_start_instruments;

  kernel_adi (tsteps, n, POLYBENCH_ARRAY(u), POLYBENCH_ARRAY(v), POLYBENCH_ARRAY(p), POLYBENCH_ARRAY(q));
  kernel_adi_mpfr (tsteps, n, POLYBENCH_ARRAY(u_mpfr), POLYBENCH_ARRAY(v_mpfr), POLYBENCH_ARRAY(p_mpfr), POLYBENCH_ARRAY(q_mpfr));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY(u), POLYBENCH_ARRAY(u_mpfr)));

  {
    int i, j;
    for (i = 0; i < N; i++)
      for (j = 0; j < N; j++) {
        mpfr_clear((*u_mpfr)[i][j]);
        mpfr_clear((*v_mpfr)[i][j]);
        mpfr_clear((*p_mpfr)[i][j]);
        mpfr_clear((*q_mpfr)[i][j]);
      }
    mpfr_free_cache();
  }

  POLYBENCH_FREE_ARRAY(u);
  POLYBENCH_FREE_ARRAY(v);
  POLYBENCH_FREE_ARRAY(p);
  POLYBENCH_FREE_ARRAY(q);
  POLYBENCH_FREE_ARRAY(u_mpfr);
  POLYBENCH_FREE_ARRAY(v_mpfr);
  POLYBENCH_FREE_ARRAY(p_mpfr);
  POLYBENCH_FREE_ARRAY(q_mpfr);

  return 0;
}
