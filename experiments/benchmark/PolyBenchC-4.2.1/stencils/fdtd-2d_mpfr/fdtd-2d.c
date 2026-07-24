/**
 * fdtd-2d MPFR reference variant.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <mpfr.h>

#include <polybench.h>
#include "fdtd-2d.h"

#define PREC_BITS 500

static
void init_array(int tmax, int nx, int ny,
		DATA_TYPE POLYBENCH_2D(ex,NX,NY,nx,ny),
		DATA_TYPE POLYBENCH_2D(ey,NX,NY,nx,ny),
		DATA_TYPE POLYBENCH_2D(hz,NX,NY,nx,ny),
		DATA_TYPE POLYBENCH_1D(_fict_,TMAX,tmax))
{
  int i, j;

  for (i = 0; i < tmax; i++)
    _fict_[i] = (DATA_TYPE)i;
  for (i = 0; i < nx; i++)
    for (j = 0; j < ny; j++) {
      ex[i][j] = ((DATA_TYPE)i * (j + 1)) / nx;
      ey[i][j] = ((DATA_TYPE)i * (j + 2)) / ny;
      hz[i][j] = ((DATA_TYPE)i * (j + 3)) / nx;
    }
}

static
void init_array_mpfr(int tmax, int nx, int ny,
		     mpfr_t POLYBENCH_2D(ex,NX,NY,nx,ny),
		     mpfr_t POLYBENCH_2D(ey,NX,NY,nx,ny),
		     mpfr_t POLYBENCH_2D(hz,NX,NY,nx,ny),
		     mpfr_t POLYBENCH_1D(_fict_,TMAX,tmax))
{
  int i, j;

  for (i = 0; i < tmax; i++)
    mpfr_set_si(_fict_[i], i, MPFR_RNDN);

  for (i = 0; i < nx; i++)
    for (j = 0; j < ny; j++) {
      mpfr_set_si(ex[i][j], i * (j + 1), MPFR_RNDN);
      mpfr_div_si(ex[i][j], ex[i][j], nx, MPFR_RNDN);

      mpfr_set_si(ey[i][j], i * (j + 2), MPFR_RNDN);
      mpfr_div_si(ey[i][j], ey[i][j], ny, MPFR_RNDN);

      mpfr_set_si(hz[i][j], i * (j + 3), MPFR_RNDN);
      mpfr_div_si(hz[i][j], hz[i][j], nx, MPFR_RNDN);
    }
}

static
void update_mpfr_max(mpfr_t max_value, mpfr_t value, mpfr_t absval)
{
  mpfr_abs(absval, value, MPFR_RNDN);
  if (mpfr_cmp(absval, max_value) > 0)
    mpfr_set(max_value, absval, MPFR_RNDN);
}

static
void add_scaled_square(mpfr_t sum, mpfr_t value, mpfr_t max_value, mpfr_t scaled)
{
  mpfr_div(scaled, value, max_value, MPFR_RNDN);
  mpfr_mul(scaled, scaled, scaled, MPFR_RNDN);
  mpfr_add(sum, sum, scaled, MPFR_RNDN);
}

static
void print_array(int nx, int ny,
		 DATA_TYPE POLYBENCH_2D(ex,NX,NY,nx,ny),
		 DATA_TYPE POLYBENCH_2D(ey,NX,NY,nx,ny),
		 DATA_TYPE POLYBENCH_2D(hz,NX,NY,nx,ny),
		 mpfr_t POLYBENCH_2D(ex_mpfr,NX,NY,nx,ny),
		 mpfr_t POLYBENCH_2D(ey_mpfr,NX,NY,nx,ny),
		 mpfr_t POLYBENCH_2D(hz_mpfr,NX,NY,nx,ny))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm_fp32 = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("fdtd");

  for (i = 0; i < nx; i++)
    for (j = 0; j < ny; j++) {
      DATA_TYPE values[3] = {ex[i][j], ey[i][j], hz[i][j]};
      int k;

      for (k = 0; k < 3; k++) {
	DATA_TYPE value = values[k];
	if (value < 0)
	  value = -value;
	if (value > max_value)
	  max_value = value;
      }
    }

  if (max_value != 0) {
    for (i = 0; i < nx; i++)
      for (j = 0; j < ny; j++) {
	DATA_TYPE scaled_ex = ex[i][j] / max_value;
	DATA_TYPE scaled_ey = ey[i][j] / max_value;
	DATA_TYPE scaled_hz = hz[i][j] / max_value;
	sum += scaled_ex * scaled_ex;
	sum += scaled_ey * scaled_ey;
	sum += scaled_hz * scaled_hz;
      }
    norm_fp32 = SQRT_FUN(sum);
  }

  mpfr_t max_mpfr, sum_mpfr, norm_mpfr, scaled, absval;
  mpfr_t norm_fp32_m, abs_err;
  mpfr_init2(max_mpfr, PREC_BITS);
  mpfr_init2(sum_mpfr, PREC_BITS);
  mpfr_init2(norm_mpfr, PREC_BITS);
  mpfr_init2(scaled, PREC_BITS);
  mpfr_init2(absval, PREC_BITS);
  mpfr_init2(norm_fp32_m, PREC_BITS);
  mpfr_init2(abs_err, PREC_BITS);

  mpfr_set_si(max_mpfr, 0, MPFR_RNDN);
  mpfr_set_si(sum_mpfr, 0, MPFR_RNDN);
  mpfr_set_si(norm_mpfr, 0, MPFR_RNDN);

  for (i = 0; i < nx; i++)
    for (j = 0; j < ny; j++) {
      update_mpfr_max(max_mpfr, ex_mpfr[i][j], absval);
      update_mpfr_max(max_mpfr, ey_mpfr[i][j], absval);
      update_mpfr_max(max_mpfr, hz_mpfr[i][j], absval);
    }

  if (mpfr_cmp_si(max_mpfr, 0) != 0) {
    for (i = 0; i < nx; i++)
      for (j = 0; j < ny; j++) {
	add_scaled_square(sum_mpfr, ex_mpfr[i][j], max_mpfr, scaled);
	add_scaled_square(sum_mpfr, ey_mpfr[i][j], max_mpfr, scaled);
	add_scaled_square(sum_mpfr, hz_mpfr[i][j], max_mpfr, scaled);
      }
    mpfr_sqrt(norm_mpfr, sum_mpfr, MPFR_RNDN);
  }

  fprintf(POLYBENCH_DUMP_TARGET, "Norm (fp32) : %.7e\n", norm_fp32);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm (mpfr) : ");
  mpfr_out_str(POLYBENCH_DUMP_TARGET, 10, 20, norm_mpfr, MPFR_RNDN);
  fprintf(POLYBENCH_DUMP_TARGET, "\n");

  mpfr_set_flt(norm_fp32_m, norm_fp32, MPFR_RNDN);
  mpfr_sub(abs_err, norm_mpfr, norm_fp32_m, MPFR_RNDN);
  mpfr_abs(abs_err, abs_err, MPFR_RNDN);

  fprintf(POLYBENCH_DUMP_TARGET, "Abs error   : ");
  mpfr_out_str(POLYBENCH_DUMP_TARGET, 10, 20, abs_err, MPFR_RNDN);
  fprintf(POLYBENCH_DUMP_TARGET, "\n");

  mpfr_clear(max_mpfr);
  mpfr_clear(sum_mpfr);
  mpfr_clear(norm_mpfr);
  mpfr_clear(scaled);
  mpfr_clear(absval);
  mpfr_clear(norm_fp32_m);
  mpfr_clear(abs_err);

  POLYBENCH_DUMP_END("fdtd");
  POLYBENCH_DUMP_FINISH;
}

static
void kernel_fdtd_2d(int tmax, int nx, int ny,
		    DATA_TYPE POLYBENCH_2D(ex,NX,NY,nx,ny),
		    DATA_TYPE POLYBENCH_2D(ey,NX,NY,nx,ny),
		    DATA_TYPE POLYBENCH_2D(hz,NX,NY,nx,ny),
		    DATA_TYPE POLYBENCH_1D(_fict_,TMAX,tmax))
{
  int t, i, j;

#pragma scop
  for (t = 0; t < _PB_TMAX; t++) {
    for (j = 0; j < _PB_NY; j++)
      ey[0][j] = _fict_[t];
    for (i = 1; i < _PB_NX; i++)
      for (j = 0; j < _PB_NY; j++)
	ey[i][j] = ey[i][j] - SCALAR_VAL(0.5) * (hz[i][j] - hz[i-1][j]);
    for (i = 0; i < _PB_NX; i++)
      for (j = 1; j < _PB_NY; j++)
	ex[i][j] = ex[i][j] - SCALAR_VAL(0.5) * (hz[i][j] - hz[i][j-1]);
    for (i = 0; i < _PB_NX - 1; i++)
      for (j = 0; j < _PB_NY - 1; j++)
	hz[i][j] = hz[i][j] - SCALAR_VAL(0.7) *
	  (ex[i][j+1] - ex[i][j] + ey[i+1][j] - ey[i][j]);
  }
#pragma endscop
}

static
void kernel_fdtd_2d_mpfr(int tmax, int nx, int ny,
			 mpfr_t POLYBENCH_2D(ex,NX,NY,nx,ny),
			 mpfr_t POLYBENCH_2D(ey,NX,NY,nx,ny),
			 mpfr_t POLYBENCH_2D(hz,NX,NY,nx,ny),
			 mpfr_t POLYBENCH_1D(_fict_,TMAX,tmax))
{
  int t, i, j;
  mpfr_t c05, c07, diff, term, update;
  mpfr_init2(c05, PREC_BITS);
  mpfr_init2(c07, PREC_BITS);
  mpfr_init2(diff, PREC_BITS);
  mpfr_init2(term, PREC_BITS);
  mpfr_init2(update, PREC_BITS);
  mpfr_set_d(c05, 0.5, MPFR_RNDN);
  mpfr_set_d(c07, 0.7, MPFR_RNDN);

#pragma scop
  for (t = 0; t < _PB_TMAX; t++) {
    for (j = 0; j < _PB_NY; j++)
      mpfr_set(ey[0][j], _fict_[t], MPFR_RNDN);

    for (i = 1; i < _PB_NX; i++)
      for (j = 0; j < _PB_NY; j++) {
	mpfr_sub(diff, hz[i][j], hz[i-1][j], MPFR_RNDN);
	mpfr_mul(update, c05, diff, MPFR_RNDN);
	mpfr_sub(ey[i][j], ey[i][j], update, MPFR_RNDN);
      }

    for (i = 0; i < _PB_NX; i++)
      for (j = 1; j < _PB_NY; j++) {
	mpfr_sub(diff, hz[i][j], hz[i][j-1], MPFR_RNDN);
	mpfr_mul(update, c05, diff, MPFR_RNDN);
	mpfr_sub(ex[i][j], ex[i][j], update, MPFR_RNDN);
      }

    for (i = 0; i < _PB_NX - 1; i++)
      for (j = 0; j < _PB_NY - 1; j++) {
	mpfr_sub(diff, ex[i][j+1], ex[i][j], MPFR_RNDN);
	mpfr_sub(term, ey[i+1][j], ey[i][j], MPFR_RNDN);
	mpfr_add(term, diff, term, MPFR_RNDN);
	mpfr_mul(update, c07, term, MPFR_RNDN);
	mpfr_sub(hz[i][j], hz[i][j], update, MPFR_RNDN);
      }
  }
#pragma endscop

  mpfr_clear(c05);
  mpfr_clear(c07);
  mpfr_clear(diff);
  mpfr_clear(term);
  mpfr_clear(update);
}

int main(int argc, char** argv)
{
  int tmax = TMAX;
  int nx = NX;
  int ny = NY;
  int i, j;

  POLYBENCH_2D_ARRAY_DECL(ex, DATA_TYPE, NX, NY, nx, ny);
  POLYBENCH_2D_ARRAY_DECL(ey, DATA_TYPE, NX, NY, nx, ny);
  POLYBENCH_2D_ARRAY_DECL(hz, DATA_TYPE, NX, NY, nx, ny);
  POLYBENCH_1D_ARRAY_DECL(_fict_, DATA_TYPE, TMAX, tmax);

  POLYBENCH_2D_ARRAY_DECL(ex_mpfr, mpfr_t, NX, NY, nx, ny);
  POLYBENCH_2D_ARRAY_DECL(ey_mpfr, mpfr_t, NX, NY, nx, ny);
  POLYBENCH_2D_ARRAY_DECL(hz_mpfr, mpfr_t, NX, NY, nx, ny);
  POLYBENCH_1D_ARRAY_DECL(_fict_mpfr, mpfr_t, TMAX, tmax);

  mpfr_set_default_prec(PREC_BITS);
  for (i = 0; i < NX; i++)
    for (j = 0; j < NY; j++) {
      mpfr_init2((*ex_mpfr)[i][j], PREC_BITS);
      mpfr_init2((*ey_mpfr)[i][j], PREC_BITS);
      mpfr_init2((*hz_mpfr)[i][j], PREC_BITS);
    }
  for (i = 0; i < TMAX; i++)
    mpfr_init2((*_fict_mpfr)[i], PREC_BITS);

  init_array(tmax, nx, ny, POLYBENCH_ARRAY(ex), POLYBENCH_ARRAY(ey),
	     POLYBENCH_ARRAY(hz), POLYBENCH_ARRAY(_fict_));
  init_array_mpfr(tmax, nx, ny, POLYBENCH_ARRAY(ex_mpfr), POLYBENCH_ARRAY(ey_mpfr),
		  POLYBENCH_ARRAY(hz_mpfr), POLYBENCH_ARRAY(_fict_mpfr));

  polybench_start_instruments;

  kernel_fdtd_2d(tmax, nx, ny, POLYBENCH_ARRAY(ex), POLYBENCH_ARRAY(ey),
		 POLYBENCH_ARRAY(hz), POLYBENCH_ARRAY(_fict_));
  kernel_fdtd_2d_mpfr(tmax, nx, ny, POLYBENCH_ARRAY(ex_mpfr), POLYBENCH_ARRAY(ey_mpfr),
		      POLYBENCH_ARRAY(hz_mpfr), POLYBENCH_ARRAY(_fict_mpfr));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(nx, ny, POLYBENCH_ARRAY(ex),
				    POLYBENCH_ARRAY(ey), POLYBENCH_ARRAY(hz),
				    POLYBENCH_ARRAY(ex_mpfr), POLYBENCH_ARRAY(ey_mpfr),
				    POLYBENCH_ARRAY(hz_mpfr)));

  for (i = 0; i < NX; i++)
    for (j = 0; j < NY; j++) {
      mpfr_clear((*ex_mpfr)[i][j]);
      mpfr_clear((*ey_mpfr)[i][j]);
      mpfr_clear((*hz_mpfr)[i][j]);
    }
  for (i = 0; i < TMAX; i++)
    mpfr_clear((*_fict_mpfr)[i]);
  mpfr_free_cache();

  POLYBENCH_FREE_ARRAY(ex);
  POLYBENCH_FREE_ARRAY(ey);
  POLYBENCH_FREE_ARRAY(hz);
  POLYBENCH_FREE_ARRAY(_fict_);
  POLYBENCH_FREE_ARRAY(ex_mpfr);
  POLYBENCH_FREE_ARRAY(ey_mpfr);
  POLYBENCH_FREE_ARRAY(hz_mpfr);
  POLYBENCH_FREE_ARRAY(_fict_mpfr);

  return 0;
}
