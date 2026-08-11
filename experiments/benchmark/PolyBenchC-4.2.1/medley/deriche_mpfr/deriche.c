/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* deriche.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mpfr.h>

#include <polybench.h>
#include "deriche.h"

#define PREC_BITS 500


static
void init_array (int w, int h, DATA_TYPE* alpha,
		 DATA_TYPE POLYBENCH_2D(imgIn,W,H,w,h),
		 DATA_TYPE POLYBENCH_2D(imgOut,W,H,w,h))
{
  int i, j;

  *alpha=0.25;

  for (i = 0; i < w; i++)
     for (j = 0; j < h; j++)
	imgIn[i][j] = (DATA_TYPE) ((313*i+991*j)%65536) / 65535.0f;
}


static
void init_array_mpfr (int w, int h,
		 mpfr_t alpha,
		 mpfr_t POLYBENCH_2D(imgIn,W,H,w,h))
{
  int i, j;

  mpfr_set_si (alpha, 1, MPFR_RNDN);
  mpfr_div_si (alpha, alpha, 4, MPFR_RNDN);

  for (i = 0; i < w; i++)
     for (j = 0; j < h; j++) {
	mpfr_set_si (imgIn[i][j], (313*i+991*j)%65536, MPFR_RNDN);
	mpfr_div_si (imgIn[i][j], imgIn[i][j], 65535, MPFR_RNDN);
     }
}


static
void print_array(int w, int h,
		 DATA_TYPE POLYBENCH_2D(imgOut_fp32,W,H,w,h),
		 mpfr_t POLYBENCH_2D(imgOut_mpfr,W,H,w,h))
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
  POLYBENCH_DUMP_BEGIN("imgOut");

  for (i = 0; i < w; i++) {
    for (j = 0; j < h; j++) {
      DATA_TYPE value = imgOut_fp32[i][j];

      if (value < 0)
        value = -value;

      if (value > max_value)
        max_value = value;

      mpfr_abs (absval, imgOut_mpfr[i][j], MPFR_RNDN);
      if (mpfr_cmp (absval, max_mpfr) > 0)
        mpfr_set (max_mpfr, absval, MPFR_RNDN);
    }
  }

  if (max_value != 0) {
    for (i = 0; i < w; i++) {
      for (j = 0; j < h; j++) {
        DATA_TYPE scaled_fp32 = imgOut_fp32[i][j] / max_value;
        sum += scaled_fp32 * scaled_fp32;
      }
    }
    norm_fp32 = SQRT_FUN(sum);
  }

  if (mpfr_cmp_si (max_mpfr, 0) != 0) {
    for (i = 0; i < w; i++) {
      for (j = 0; j < h; j++) {
        mpfr_div (scaled, imgOut_mpfr[i][j], max_mpfr, MPFR_RNDN);
        mpfr_mul (scaled, scaled, scaled, MPFR_RNDN);
        mpfr_add (sum_mpfr, sum_mpfr, scaled, MPFR_RNDN);
      }
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

  POLYBENCH_DUMP_END("imgOut");
  POLYBENCH_DUMP_FINISH;
}


/* Original code provided by Gael Deest */
static
void kernel_deriche(int w, int h, DATA_TYPE alpha,
       DATA_TYPE POLYBENCH_2D(imgIn, W, H, w, h),
       DATA_TYPE POLYBENCH_2D(imgOut, W, H, w, h),
       DATA_TYPE POLYBENCH_2D(y1, W, H, w, h),
       DATA_TYPE POLYBENCH_2D(y2, W, H, w, h)) {
    int i,j;
    DATA_TYPE xm1, tm1, ym1, ym2;
    DATA_TYPE xp1, xp2;
    DATA_TYPE tp1, tp2;
    DATA_TYPE yp1, yp2;

    DATA_TYPE k;
    DATA_TYPE a1, a2, a3, a4, a5, a6, a7, a8;
    DATA_TYPE b1, b2, c1, c2;

#pragma scop
   k = (SCALAR_VAL(1.0)-EXP_FUN(-alpha))*(SCALAR_VAL(1.0)-EXP_FUN(-alpha))/(SCALAR_VAL(1.0)+SCALAR_VAL(2.0)*alpha*EXP_FUN(-alpha)-EXP_FUN(SCALAR_VAL(2.0)*alpha));
   a1 = a5 = k;
   a2 = a6 = k*EXP_FUN(-alpha)*(alpha-SCALAR_VAL(1.0));
   a3 = a7 = k*EXP_FUN(-alpha)*(alpha+SCALAR_VAL(1.0));
   a4 = a8 = -k*EXP_FUN(SCALAR_VAL(-2.0)*alpha);
   b1 =  POW_FUN(SCALAR_VAL(2.0),-alpha);
   b2 = -EXP_FUN(SCALAR_VAL(-2.0)*alpha);
   c1 = c2 = SCALAR_VAL(1.0);

   for (i=0; i<_PB_W; i++) {
        ym1 = SCALAR_VAL(0.0);
        ym2 = SCALAR_VAL(0.0);
        xm1 = SCALAR_VAL(0.0);
        for (j=0; j<_PB_H; j++) {
            y1[i][j] = a1*imgIn[i][j] + a2*xm1 + b1*ym1 + b2*ym2;
            xm1 = imgIn[i][j];
            ym2 = ym1;
            ym1 = y1[i][j];
        }
    }

    for (i=0; i<_PB_W; i++) {
        yp1 = SCALAR_VAL(0.0);
        yp2 = SCALAR_VAL(0.0);
        xp1 = SCALAR_VAL(0.0);
        xp2 = SCALAR_VAL(0.0);
        for (j=_PB_H-1; j>=0; j--) {
            y2[i][j] = a3*xp1 + a4*xp2 + b1*yp1 + b2*yp2;
            xp2 = xp1;
            xp1 = imgIn[i][j];
            yp2 = yp1;
            yp1 = y2[i][j];
        }
    }

    for (i=0; i<_PB_W; i++)
        for (j=0; j<_PB_H; j++)
            imgOut[i][j] = c1 * (y1[i][j] + y2[i][j]);

    for (j=0; j<_PB_H; j++) {
        tm1 = SCALAR_VAL(0.0);
        ym1 = SCALAR_VAL(0.0);
        ym2 = SCALAR_VAL(0.0);
        for (i=0; i<_PB_W; i++) {
            y1[i][j] = a5*imgOut[i][j] + a6*tm1 + b1*ym1 + b2*ym2;
            tm1 = imgOut[i][j];
            ym2 = ym1;
            ym1 = y1 [i][j];
        }
    }

    for (j=0; j<_PB_H; j++) {
        tp1 = SCALAR_VAL(0.0);
        tp2 = SCALAR_VAL(0.0);
        yp1 = SCALAR_VAL(0.0);
        yp2 = SCALAR_VAL(0.0);
        for (i=_PB_W-1; i>=0; i--) {
            y2[i][j] = a7*tp1 + a8*tp2 + b1*yp1 + b2*yp2;
            tp2 = tp1;
            tp1 = imgOut[i][j];
            yp2 = yp1;
            yp1 = y2[i][j];
        }
    }

    for (i=0; i<_PB_W; i++)
        for (j=0; j<_PB_H; j++)
            imgOut[i][j] = c2*(y1[i][j] + y2[i][j]);
#pragma endscop
}


static
void mpfr_exp_neg_alpha(mpfr_t rop, mpfr_t alpha)
{
  mpfr_neg (rop, alpha, MPFR_RNDN);
  mpfr_exp (rop, rop, MPFR_RNDN);
}


static
void kernel_deriche_mpfr(int w, int h, mpfr_t alpha,
       mpfr_t POLYBENCH_2D(imgIn, W, H, w, h),
       mpfr_t POLYBENCH_2D(imgOut, W, H, w, h),
       mpfr_t POLYBENCH_2D(y1, W, H, w, h),
       mpfr_t POLYBENCH_2D(y2, W, H, w, h)) {
    int i,j;
    mpfr_t xm1, tm1, ym1, ym2;
    mpfr_t xp1, xp2;
    mpfr_t tp1, tp2;
    mpfr_t yp1, yp2;

    mpfr_t k;
    mpfr_t a1, a2, a3, a4, a5, a6, a7, a8;
    mpfr_t b1, b2, c1, c2;
    mpfr_t exp_neg_alpha, tmp1, tmp2, tmp3;

    mpfr_inits2(PREC_BITS, xm1, tm1, ym1, ym2, xp1, xp2, tp1, tp2,
                yp1, yp2, k, a1, a2, a3, a4, a5, a6, a7, a8,
                b1, b2, c1, c2, exp_neg_alpha, tmp1, tmp2, tmp3,
                (mpfr_ptr) 0);

#pragma scop
   mpfr_exp_neg_alpha(exp_neg_alpha, alpha);
   mpfr_si_sub(tmp1, 1, exp_neg_alpha, MPFR_RNDN);
   mpfr_mul(tmp1, tmp1, tmp1, MPFR_RNDN);
   mpfr_mul_si(tmp2, alpha, 2, MPFR_RNDN);
   mpfr_mul(tmp2, tmp2, exp_neg_alpha, MPFR_RNDN);
   mpfr_add_si(tmp2, tmp2, 1, MPFR_RNDN);
   mpfr_mul_si(tmp3, alpha, 2, MPFR_RNDN);
   mpfr_exp(tmp3, tmp3, MPFR_RNDN);
   mpfr_sub(tmp2, tmp2, tmp3, MPFR_RNDN);
   mpfr_div(k, tmp1, tmp2, MPFR_RNDN);

   mpfr_set(a1, k, MPFR_RNDN);
   mpfr_set(a5, k, MPFR_RNDN);
   mpfr_mul(tmp1, k, exp_neg_alpha, MPFR_RNDN);
   mpfr_sub_si(tmp2, alpha, 1, MPFR_RNDN);
   mpfr_mul(a2, tmp1, tmp2, MPFR_RNDN);
   mpfr_set(a6, a2, MPFR_RNDN);
   mpfr_add_si(tmp2, alpha, 1, MPFR_RNDN);
   mpfr_mul(a3, tmp1, tmp2, MPFR_RNDN);
   mpfr_set(a7, a3, MPFR_RNDN);
   mpfr_mul_si(tmp2, alpha, -2, MPFR_RNDN);
   mpfr_exp(tmp2, tmp2, MPFR_RNDN);
   mpfr_mul(a4, k, tmp2, MPFR_RNDN);
   mpfr_neg(a4, a4, MPFR_RNDN);
   mpfr_set(a8, a4, MPFR_RNDN);
   mpfr_set_si(tmp1, 2, MPFR_RNDN);
   mpfr_neg(tmp2, alpha, MPFR_RNDN);
   mpfr_pow(b1, tmp1, tmp2, MPFR_RNDN);
   mpfr_mul_si(tmp2, alpha, -2, MPFR_RNDN);
   mpfr_exp(b2, tmp2, MPFR_RNDN);
   mpfr_neg(b2, b2, MPFR_RNDN);
   mpfr_set_si(c1, 1, MPFR_RNDN);
   mpfr_set_si(c2, 1, MPFR_RNDN);

   for (i=0; i<_PB_W; i++) {
        mpfr_set_si(ym1, 0, MPFR_RNDN);
        mpfr_set_si(ym2, 0, MPFR_RNDN);
        mpfr_set_si(xm1, 0, MPFR_RNDN);
        for (j=0; j<_PB_H; j++) {
            mpfr_mul(tmp1, a1, imgIn[i][j], MPFR_RNDN);
            mpfr_mul(tmp2, a2, xm1, MPFR_RNDN);
            mpfr_add(tmp1, tmp1, tmp2, MPFR_RNDN);
            mpfr_mul(tmp2, b1, ym1, MPFR_RNDN);
            mpfr_add(tmp1, tmp1, tmp2, MPFR_RNDN);
            mpfr_mul(tmp2, b2, ym2, MPFR_RNDN);
            mpfr_add(y1[i][j], tmp1, tmp2, MPFR_RNDN);
            mpfr_set(xm1, imgIn[i][j], MPFR_RNDN);
            mpfr_set(ym2, ym1, MPFR_RNDN);
            mpfr_set(ym1, y1[i][j], MPFR_RNDN);
        }
    }

    for (i=0; i<_PB_W; i++) {
        mpfr_set_si(yp1, 0, MPFR_RNDN);
        mpfr_set_si(yp2, 0, MPFR_RNDN);
        mpfr_set_si(xp1, 0, MPFR_RNDN);
        mpfr_set_si(xp2, 0, MPFR_RNDN);
        for (j=_PB_H-1; j>=0; j--) {
            mpfr_mul(tmp1, a3, xp1, MPFR_RNDN);
            mpfr_mul(tmp2, a4, xp2, MPFR_RNDN);
            mpfr_add(tmp1, tmp1, tmp2, MPFR_RNDN);
            mpfr_mul(tmp2, b1, yp1, MPFR_RNDN);
            mpfr_add(tmp1, tmp1, tmp2, MPFR_RNDN);
            mpfr_mul(tmp2, b2, yp2, MPFR_RNDN);
            mpfr_add(y2[i][j], tmp1, tmp2, MPFR_RNDN);
            mpfr_set(xp2, xp1, MPFR_RNDN);
            mpfr_set(xp1, imgIn[i][j], MPFR_RNDN);
            mpfr_set(yp2, yp1, MPFR_RNDN);
            mpfr_set(yp1, y2[i][j], MPFR_RNDN);
        }
    }

    for (i=0; i<_PB_W; i++)
        for (j=0; j<_PB_H; j++) {
            mpfr_add(tmp1, y1[i][j], y2[i][j], MPFR_RNDN);
            mpfr_mul(imgOut[i][j], c1, tmp1, MPFR_RNDN);
        }

    for (j=0; j<_PB_H; j++) {
        mpfr_set_si(tm1, 0, MPFR_RNDN);
        mpfr_set_si(ym1, 0, MPFR_RNDN);
        mpfr_set_si(ym2, 0, MPFR_RNDN);
        for (i=0; i<_PB_W; i++) {
            mpfr_mul(tmp1, a5, imgOut[i][j], MPFR_RNDN);
            mpfr_mul(tmp2, a6, tm1, MPFR_RNDN);
            mpfr_add(tmp1, tmp1, tmp2, MPFR_RNDN);
            mpfr_mul(tmp2, b1, ym1, MPFR_RNDN);
            mpfr_add(tmp1, tmp1, tmp2, MPFR_RNDN);
            mpfr_mul(tmp2, b2, ym2, MPFR_RNDN);
            mpfr_add(y1[i][j], tmp1, tmp2, MPFR_RNDN);
            mpfr_set(tm1, imgOut[i][j], MPFR_RNDN);
            mpfr_set(ym2, ym1, MPFR_RNDN);
            mpfr_set(ym1, y1[i][j], MPFR_RNDN);
        }
    }

    for (j=0; j<_PB_H; j++) {
        mpfr_set_si(tp1, 0, MPFR_RNDN);
        mpfr_set_si(tp2, 0, MPFR_RNDN);
        mpfr_set_si(yp1, 0, MPFR_RNDN);
        mpfr_set_si(yp2, 0, MPFR_RNDN);
        for (i=_PB_W-1; i>=0; i--) {
            mpfr_mul(tmp1, a7, tp1, MPFR_RNDN);
            mpfr_mul(tmp2, a8, tp2, MPFR_RNDN);
            mpfr_add(tmp1, tmp1, tmp2, MPFR_RNDN);
            mpfr_mul(tmp2, b1, yp1, MPFR_RNDN);
            mpfr_add(tmp1, tmp1, tmp2, MPFR_RNDN);
            mpfr_mul(tmp2, b2, yp2, MPFR_RNDN);
            mpfr_add(y2[i][j], tmp1, tmp2, MPFR_RNDN);
            mpfr_set(tp2, tp1, MPFR_RNDN);
            mpfr_set(tp1, imgOut[i][j], MPFR_RNDN);
            mpfr_set(yp2, yp1, MPFR_RNDN);
            mpfr_set(yp1, y2[i][j], MPFR_RNDN);
        }
    }

    for (i=0; i<_PB_W; i++)
        for (j=0; j<_PB_H; j++) {
            mpfr_add(tmp1, y1[i][j], y2[i][j], MPFR_RNDN);
            mpfr_mul(imgOut[i][j], c2, tmp1, MPFR_RNDN);
        }
#pragma endscop

    mpfr_clears(xm1, tm1, ym1, ym2, xp1, xp2, tp1, tp2,
                yp1, yp2, k, a1, a2, a3, a4, a5, a6, a7, a8,
                b1, b2, c1, c2, exp_neg_alpha, tmp1, tmp2, tmp3,
                (mpfr_ptr) 0);
}


int main(int argc, char** argv)
{
  int w = W;
  int h = H;

  DATA_TYPE alpha;
  mpfr_t alpha_mpfr;
  POLYBENCH_2D_ARRAY_DECL(imgIn, DATA_TYPE, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(imgOut, DATA_TYPE, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(y1, DATA_TYPE, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(y2, DATA_TYPE, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(imgIn_mpfr, mpfr_t, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(imgOut_mpfr, mpfr_t, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(y1_mpfr, mpfr_t, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(y2_mpfr, mpfr_t, W, H, w, h);

  mpfr_set_default_prec(PREC_BITS);
  mpfr_init2(alpha_mpfr, PREC_BITS);
  {
    int i, j;
    for (i = 0; i < W; i++)
      for (j = 0; j < H; j++) {
        mpfr_init2((*imgIn_mpfr)[i][j], PREC_BITS);
        mpfr_init2((*imgOut_mpfr)[i][j], PREC_BITS);
        mpfr_init2((*y1_mpfr)[i][j], PREC_BITS);
        mpfr_init2((*y2_mpfr)[i][j], PREC_BITS);
      }
  }

  init_array (w, h, &alpha, POLYBENCH_ARRAY(imgIn), POLYBENCH_ARRAY(imgOut));
  init_array_mpfr (w, h, alpha_mpfr, POLYBENCH_ARRAY(imgIn_mpfr));

  polybench_start_instruments;

  kernel_deriche (w, h, alpha, POLYBENCH_ARRAY(imgIn), POLYBENCH_ARRAY(imgOut), POLYBENCH_ARRAY(y1), POLYBENCH_ARRAY(y2));
  kernel_deriche_mpfr (w, h, alpha_mpfr, POLYBENCH_ARRAY(imgIn_mpfr), POLYBENCH_ARRAY(imgOut_mpfr), POLYBENCH_ARRAY(y1_mpfr), POLYBENCH_ARRAY(y2_mpfr));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(w, h, POLYBENCH_ARRAY(imgOut), POLYBENCH_ARRAY(imgOut_mpfr)));

  {
    int i, j;
    for (i = 0; i < W; i++)
      for (j = 0; j < H; j++) {
        mpfr_clear((*imgIn_mpfr)[i][j]);
        mpfr_clear((*imgOut_mpfr)[i][j]);
        mpfr_clear((*y1_mpfr)[i][j]);
        mpfr_clear((*y2_mpfr)[i][j]);
      }
    mpfr_clear(alpha_mpfr);
    mpfr_free_cache();
  }

  POLYBENCH_FREE_ARRAY(imgIn);
  POLYBENCH_FREE_ARRAY(imgOut);
  POLYBENCH_FREE_ARRAY(y1);
  POLYBENCH_FREE_ARRAY(y2);
  POLYBENCH_FREE_ARRAY(imgIn_mpfr);
  POLYBENCH_FREE_ARRAY(imgOut_mpfr);
  POLYBENCH_FREE_ARRAY(y1_mpfr);
  POLYBENCH_FREE_ARRAY(y2_mpfr);

  return 0;
}
