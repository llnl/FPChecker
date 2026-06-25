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

#include <polybench.h>
#include "deriche.h"

static
void init_array (int w, int h, DATA_TYPE* alpha,
		 DATA_TYPE POLYBENCH_2D(imgIn,W,H,w,h),
		 DATA_TYPE POLYBENCH_2D(imgOut,W,H,w,h))
{
  int i, j;

  *alpha=0.25; //parameter of the filter

  for (i = 0; i < w; i++)
     for (j = 0; j < h; j++)
	imgIn[i][j] = (DATA_TYPE) ((313*i+991*j)%65536) / 65535.0;
}

static
void init_array_long_double (int w, int h, long double* alpha,
			 long double POLYBENCH_2D(imgIn,W,H,w,h),
			 long double POLYBENCH_2D(imgOut,W,H,w,h))
{
  int i, j;

  *alpha=(long double)0.25L; //parameter of the filter

  for (i = 0; i < w; i++)
     for (j = 0; j < h; j++)
	{
	  imgIn[i][j] = (long double) ((313*i+991*j)%65536) / 65535.0L;
	}
}

static
void print_array(int w, int h,
		 DATA_TYPE POLYBENCH_2D(imgOut,W,H,w,h),
		 long double POLYBENCH_2D(imgOut_double,W,H,w,h))
{
  int i, j;

  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;

  long double max_value_double = 0;
  long double sum_double = 0;
  long double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("imgOut");

  for (i = 0; i < w; i++) {
    for (j = 0; j < h; j++) {
      DATA_TYPE value = imgOut[i][j];
      long double value_double = imgOut_double[i][j];

      if (value < 0)
        value = -value;

      if (value_double < 0.0L)
        value_double = -value_double;

      if (value > max_value)
        max_value = value;

      if (value_double > max_value_double)
        max_value_double = value_double;
    }
  }

  if (max_value != 0) {
    for (i = 0; i < w; i++) {
      for (j = 0; j < h; j++) {
        DATA_TYPE scaled = imgOut[i][j] / max_value;
        sum += scaled * scaled;
      }
    }
    norm = SQRT_FUN(sum);
  }

  if (max_value_double != 0) {
    for (i = 0; i < w; i++) {
      for (j = 0; j < h; j++) {
        long double scaled = imgOut_double[i][j] / max_value_double;
        sum_double += scaled * scaled;
      }
    }
    norm_double = sqrtl(sum_double);
  }

  fprintf(POLYBENCH_DUMP_TARGET, "Max value in imgOut: %.17e\n", max_value);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of imgOut: %.17e\n", norm);
  fprintf(POLYBENCH_DUMP_TARGET, "Max value in imgOut_double: %.21Le\n", max_value_double);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of imgOut_double: %.21Le\n", norm_double);

  long double norm_error = norm_double - (long double)norm;
  fprintf(POLYBENCH_DUMP_TARGET, "Norm error: %.21Le\n", norm_error);

  POLYBENCH_DUMP_END("imgOut");
  POLYBENCH_DUMP_FINISH;
}

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
void kernel_deriche_long_double(int w, int h, long double alpha,
       long double POLYBENCH_2D(imgIn, W, H, w, h),
       long double POLYBENCH_2D(imgOut, W, H, w, h),
       long double POLYBENCH_2D(y1, W, H, w, h),
       long double POLYBENCH_2D(y2, W, H, w, h)) {
    int i,j;
    long double xm1, tm1, ym1, ym2;
    long double xp1, xp2;
    long double tp1, tp2;
    long double yp1, yp2;

    long double k;
    long double a1, a2, a3, a4, a5, a6, a7, a8;
    long double b1, b2, c1, c2;

#pragma scop
   k = (1.0L-expl(-alpha))*(1.0L-expl(-alpha))/(1.0L+2.0L*alpha*expl(-alpha)-expl(2.0L*alpha));
   a1 = a5 = k;
   a2 = a6 = k*expl(-alpha)*(alpha-1.0L);
   a3 = a7 = k*expl(-alpha)*(alpha+1.0L);
   a4 = a8 = -k*expl(-2.0L*alpha);
   b1 =  powl(2.0L,-alpha);
   b2 = -expl(-2.0L*alpha);
   c1 = c2 = 1.0L;

   for (i=0; i<_PB_W; i++) {
        ym1 = 0.0L;
        ym2 = 0.0L;
        xm1 = 0.0L;
        for (j=0; j<_PB_H; j++) {
            y1[i][j] = a1*imgIn[i][j] + a2*xm1 + b1*ym1 + b2*ym2;
            xm1 = imgIn[i][j];
            ym2 = ym1;
            ym1 = y1[i][j];
        }
    }

    for (i=0; i<_PB_W; i++) {
        yp1 = 0.0L;
        yp2 = 0.0L;
        xp1 = 0.0L;
        xp2 = 0.0L;
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
        tm1 = 0.0L;
        ym1 = 0.0L;
        ym2 = 0.0L;
        for (i=0; i<_PB_W; i++) {
            y1[i][j] = a5*imgOut[i][j] + a6*tm1 + b1*ym1 + b2*ym2;
            tm1 = imgOut[i][j];
            ym2 = ym1;
            ym1 = y1 [i][j];
        }
    }

    for (j=0; j<_PB_H; j++) {
        tp1 = 0.0L;
        tp2 = 0.0L;
        yp1 = 0.0L;
        yp2 = 0.0L;
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

int main(int argc, char** argv)
{
  int w = W;
  int h = H;

  DATA_TYPE alpha;
  long double alpha_long_double;
  POLYBENCH_2D_ARRAY_DECL(imgIn, DATA_TYPE, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(imgOut, DATA_TYPE, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(y1, DATA_TYPE, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(y2, DATA_TYPE, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(imgIn_long_double, long double, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(imgOut_long_double, long double, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(y1_long_double, long double, W, H, w, h);
  POLYBENCH_2D_ARRAY_DECL(y2_long_double, long double, W, H, w, h);

  init_array (w, h, &alpha, POLYBENCH_ARRAY(imgIn), POLYBENCH_ARRAY(imgOut));
  init_array_long_double (w, h, &alpha_long_double, POLYBENCH_ARRAY(imgIn_long_double), POLYBENCH_ARRAY(imgOut_long_double));

  polybench_start_instruments;

  kernel_deriche (w, h, alpha, POLYBENCH_ARRAY(imgIn), POLYBENCH_ARRAY(imgOut), POLYBENCH_ARRAY(y1), POLYBENCH_ARRAY(y2));
  kernel_deriche_long_double (w, h, alpha_long_double, POLYBENCH_ARRAY(imgIn_long_double), POLYBENCH_ARRAY(imgOut_long_double), POLYBENCH_ARRAY(y1_long_double), POLYBENCH_ARRAY(y2_long_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(w, h, POLYBENCH_ARRAY(imgOut), POLYBENCH_ARRAY(imgOut_long_double)));

  POLYBENCH_FREE_ARRAY(imgIn);
  POLYBENCH_FREE_ARRAY(imgOut);
  POLYBENCH_FREE_ARRAY(y1);
  POLYBENCH_FREE_ARRAY(y2);
  POLYBENCH_FREE_ARRAY(imgIn_long_double);
  POLYBENCH_FREE_ARRAY(imgOut_long_double);
  POLYBENCH_FREE_ARRAY(y1_long_double);
  POLYBENCH_FREE_ARRAY(y2_long_double);

  return 0;
}
