/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* fdtd-2d.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
#include "fdtd-2d.h"


/* Array initialization. */
static
void init_array (int tmax,
		 int nx,
		 int ny,
		 DATA_TYPE POLYBENCH_2D(ex,NX,NY,nx,ny),
		 DATA_TYPE POLYBENCH_2D(ey,NX,NY,nx,ny),
		 DATA_TYPE POLYBENCH_2D(hz,NX,NY,nx,ny),
		 DATA_TYPE POLYBENCH_1D(_fict_,TMAX,tmax))
{
  int i, j;

  for (i = 0; i < tmax; i++)
    _fict_[i] = (DATA_TYPE) i;
  for (i = 0; i < nx; i++)
    for (j = 0; j < ny; j++) {
      ex[i][j] = ((DATA_TYPE) i*(j+1)) / nx;
      ey[i][j] = ((DATA_TYPE) i*(j+2)) / ny;
      hz[i][j] = ((DATA_TYPE) i*(j+3)) / nx;
    }
}

static
void init_array_double (int tmax, int nx, int ny,
                 double POLYBENCH_2D(ex,NX,NY,nx,ny),
                 double POLYBENCH_2D(ey,NX,NY,nx,ny),
                 double POLYBENCH_2D(hz,NX,NY,nx,ny),
                 double POLYBENCH_1D(_fict_,TMAX,tmax))
{
  int i, j;

  for (i = 0; i < tmax; i++)
    _fict_[i] = (double) i;
  for (i = 0; i < nx; i++)
    for (j = 0; j < ny; j++) {
      ex[i][j] = ((double) i*(j+1)) / nx;
      ey[i][j] = ((double) i*(j+2)) / ny;
      hz[i][j] = ((double) i*(j+3)) / nx;
    }
}

static
void print_array(int nx,
                 int ny,
                 DATA_TYPE POLYBENCH_2D(ex,NX,NY,nx,ny),
                 DATA_TYPE POLYBENCH_2D(ey,NX,NY,nx,ny),
                 DATA_TYPE POLYBENCH_2D(hz,NX,NY,nx,ny),
                 double POLYBENCH_2D(ex_double,NX,NY,nx,ny),
                 double POLYBENCH_2D(ey_double,NX,NY,nx,ny),
                 double POLYBENCH_2D(hz_double,NX,NY,nx,ny))
{
  int i, j;
  DATA_TYPE max_value = 0;
  DATA_TYPE sum = 0;
  DATA_TYPE norm = 0;
  double max_value_double = 0;
  double sum_double = 0;
  double norm_double = 0;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("fdtd");
  for (i = 0; i < nx; i++)
    for (j = 0; j < ny; j++) {
      DATA_TYPE values[3] = {ex[i][j], ey[i][j], hz[i][j]};
      double values_double[3] = {ex_double[i][j], ey_double[i][j], hz_double[i][j]};
      int k;

      for (k = 0; k < 3; k++) {
        DATA_TYPE value = values[k];
        double value_double = values_double[k];

        if (value < 0)
          value = -value;
        if (value_double < 0.0)
          value_double = -value_double;

        if (value > max_value)
          max_value = value;
        if (value_double > max_value_double)
          max_value_double = value_double;
      }
    }

  if (max_value != 0) {
    for (i = 0; i < nx; i++)
      for (j = 0; j < ny; j++) {
        DATA_TYPE scaled_ex = ex[i][j] / max_value;
        sum += scaled_ex * scaled_ex;
        DATA_TYPE scaled_ey = ey[i][j] / max_value;
        sum += scaled_ey * scaled_ey;
        DATA_TYPE scaled_hz = hz[i][j] / max_value;
        sum += scaled_hz * scaled_hz;
      }
    norm = SQRT_FUN(sum);
  }

  if (max_value_double != 0) {
    for (i = 0; i < nx; i++)
      for (j = 0; j < ny; j++) {
        double scaled_ex = ex_double[i][j] / max_value_double;
        sum_double += scaled_ex * scaled_ex;
        double scaled_ey = ey_double[i][j] / max_value_double;
        sum_double += scaled_ey * scaled_ey;
        double scaled_hz = hz_double[i][j] / max_value_double;
        sum_double += scaled_hz * scaled_hz;
      }
    norm_double = sqrt(sum_double);
  }

  fprintf(POLYBENCH_DUMP_TARGET, "Max value in fdtd: %.7e\n", max_value);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of fdtd: %.7e\n", norm);
  fprintf(POLYBENCH_DUMP_TARGET, "Max value in fdtd_double: %.17e\n", max_value_double);
  fprintf(POLYBENCH_DUMP_TARGET, "Norm of fdtd_double: %.17e\n", norm_double);

  double norm_error = norm_double - (double)norm;
  fprintf(POLYBENCH_DUMP_TARGET, "Norm error: %.17e\n", norm_error);

  POLYBENCH_DUMP_END("fdtd");
  POLYBENCH_DUMP_FINISH;
}

static
void kernel_fdtd_2d(int tmax,
                    int nx,
                    int ny,
                    DATA_TYPE POLYBENCH_2D(ex,NX,NY,nx,ny),
                    DATA_TYPE POLYBENCH_2D(ey,NX,NY,nx,ny),
                    DATA_TYPE POLYBENCH_2D(hz,NX,NY,nx,ny),
                    DATA_TYPE POLYBENCH_1D(_fict_,TMAX,tmax))
{
  int t, i, j;

#pragma scop

  for(t = 0; t < _PB_TMAX; t++)
    {
      for (j = 0; j < _PB_NY; j++)
        ey[0][j] = _fict_[t];
      for (i = 1; i < _PB_NX; i++)
        for (j = 0; j < _PB_NY; j++)
          ey[i][j] = ey[i][j] - SCALAR_VAL(0.5)*(hz[i][j]-hz[i-1][j]);
      for (i = 0; i < _PB_NX; i++)
        for (j = 1; j < _PB_NY; j++)
          ex[i][j] = ex[i][j] - SCALAR_VAL(0.5)*(hz[i][j]-hz[i][j-1]);
      for (i = 0; i < _PB_NX - 1; i++)
        for (j = 0; j < _PB_NY - 1; j++)
          hz[i][j] = hz[i][j] - SCALAR_VAL(0.7)*  (ex[i][j+1] - ex[i][j] +
                                       ey[i+1][j] - ey[i][j]);
    }

#pragma endscop
}

static
void kernel_fdtd_2d_double(int tmax,
                    int nx,
                    int ny,
                    double POLYBENCH_2D(ex,NX,NY,nx,ny),
                    double POLYBENCH_2D(ey,NX,NY,nx,ny),
                    double POLYBENCH_2D(hz,NX,NY,nx,ny),
                    double POLYBENCH_1D(_fict_,TMAX,tmax))
{
  int t, i, j;

#pragma scop

  for(t = 0; t < _PB_TMAX; t++)
    {
      for (j = 0; j < _PB_NY; j++)
        ey[0][j] = _fict_[t];
      for (i = 1; i < _PB_NX; i++)
        for (j = 0; j < _PB_NY; j++)
          ey[i][j] = ey[i][j] - 0.5*(hz[i][j]-hz[i-1][j]);
      for (i = 0; i < _PB_NX; i++)
        for (j = 1; j < _PB_NY; j++)
          ex[i][j] = ex[i][j] - 0.5*(hz[i][j]-hz[i][j-1]);
      for (i = 0; i < _PB_NX - 1; i++)
        for (j = 0; j < _PB_NY - 1; j++)
          hz[i][j] = hz[i][j] - 0.7*  (ex[i][j+1] - ex[i][j] +
                                       ey[i+1][j] - ey[i][j]);
    }

#pragma endscop
}

int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int tmax = TMAX;
  int nx = NX;
  int ny = NY;

  POLYBENCH_2D_ARRAY_DECL(ex, DATA_TYPE, NX, NY, nx, ny);
  POLYBENCH_2D_ARRAY_DECL(ey, DATA_TYPE, NX, NY, nx, ny);
  POLYBENCH_2D_ARRAY_DECL(hz, DATA_TYPE, NX, NY, nx, ny);
  POLYBENCH_1D_ARRAY_DECL(_fict_, DATA_TYPE, TMAX, tmax);
  POLYBENCH_2D_ARRAY_DECL(ex_double, double, NX, NY, nx, ny);
  POLYBENCH_2D_ARRAY_DECL(ey_double, double, NX, NY, nx, ny);
  POLYBENCH_2D_ARRAY_DECL(hz_double, double, NX, NY, nx, ny);
  POLYBENCH_1D_ARRAY_DECL(_fict_double, double, TMAX, tmax);

  init_array (tmax, nx, ny, POLYBENCH_ARRAY(ex), POLYBENCH_ARRAY(ey), POLYBENCH_ARRAY(hz), POLYBENCH_ARRAY(_fict_));
  init_array_double (tmax, nx, ny, POLYBENCH_ARRAY(ex_double), POLYBENCH_ARRAY(ey_double), POLYBENCH_ARRAY(hz_double), POLYBENCH_ARRAY(_fict_double));

  polybench_start_instruments;

  kernel_fdtd_2d (tmax, nx, ny, POLYBENCH_ARRAY(ex), POLYBENCH_ARRAY(ey), POLYBENCH_ARRAY(hz), POLYBENCH_ARRAY(_fict_));
  kernel_fdtd_2d_double (tmax, nx, ny, POLYBENCH_ARRAY(ex_double), POLYBENCH_ARRAY(ey_double), POLYBENCH_ARRAY(hz_double), POLYBENCH_ARRAY(_fict_double));

  polybench_stop_instruments;
  polybench_print_instruments;

  polybench_prevent_dce(print_array(nx, ny, POLYBENCH_ARRAY(ex), POLYBENCH_ARRAY(ey), POLYBENCH_ARRAY(hz), POLYBENCH_ARRAY(ex_double), POLYBENCH_ARRAY(ey_double), POLYBENCH_ARRAY(hz_double)));

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(ex);
  POLYBENCH_FREE_ARRAY(ey);
  POLYBENCH_FREE_ARRAY(hz);
  POLYBENCH_FREE_ARRAY(_fict_);
  POLYBENCH_FREE_ARRAY(ex_double);
  POLYBENCH_FREE_ARRAY(ey_double);
  POLYBENCH_FREE_ARRAY(hz_double);
  POLYBENCH_FREE_ARRAY(_fict_double);

  return 0;
}
