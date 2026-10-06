// ************************************************************************
//
// miniAMR: stencil computations with boundary exchange and AMR.
//
// Copyright (2014) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
// Questions? Contact Courtenay T. Vaughan (ctvaugh@sandia.gov)
//                    Richard F. Barrett (rfbarre@sandia.gov)
//
// ************************************************************************

#include <mpi.h>
#include <math.h>

#include "block.h"
#include "comm.h"
#include "proto.h"
void stencil_calc(int, int);
void stencil_0(int);
void stencil_x(int);
void stencil_y(int);
void stencil_z(int);
void stencil_7(int);
void stencil_27(int);
void stencil_check(int);

// This routine does the stencil calculations.
void stencil_driver(int var, int cacl_stage)
{
   if (stencil)
      stencil_calc(var, stencil);
   else
      if (!var)
         stencil_calc(var, 7);
      else if (var < 4*mat) {
         switch (cacl_stage%6) {
         case 0:
            stencil_0(var);
            break;
         case 1:
            stencil_x(var);
            break;
         case 2:
            stencil_y(var);
            break;
         case 3:
            stencil_z(var);
            break;
         case 4:
            stencil_7(var);
            break;
         case 5:
            stencil_27(var);
            break;
         }
         stencil_check(var);
      } else
          stencil_calc(var, 7);
}

void stencil_calc(int var, int stencil_in)
{
   int i, j, k, in;
   double sb, sm, sf, work[x_block_size+2][y_block_size+2][z_block_size+2];
   block *bp;

   if (stencil_in == 7) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  work[i][j][k] = (bp->array[var][i-1][j  ][k  ] +
                                   bp->array[var][i  ][j-1][k  ] +
                                   bp->array[var][i  ][j  ][k-1] +
                                   bp->array[var][i  ][j  ][k  ] +
                                   bp->array[var][i  ][j  ][k+1] +
                                   bp->array[var][i  ][j+1][k  ] +
                                   bp->array[var][i+1][j  ][k  ])/7.0;
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = work[i][j][k];

#ifndef FPC_MINIAMR_DISABLE_INJECTION
         if (in == 0 && var == 0)
         {
            volatile double base_val = 456789.123;
            double inj_x = base_val;
            double inj_y = base_val + 1.0e-11;
            volatile double cancelled_result __attribute__((unused)) = inj_y - inj_x; // Injection standalone

            double state_x = (work[1][1][1] + bp->array[var][1][1][1] + (double) stencil_in + 1.0) * 456789.123;
            double state_y = state_x + state_x * 1.0e-15;
            volatile double state_cancelled_result __attribute__((unused)) = state_y - state_x; // Injection state
         }
#endif
      }
      total_fp_divs += (double) num_active*num_cells;
      total_fp_adds += (double) 6*num_active*num_cells;
   } else {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++) {
                  sb = bp->array[var][i-1][j-1][k-1] +
                       bp->array[var][i-1][j-1][k  ] +
                       bp->array[var][i-1][j-1][k+1] +
                       bp->array[var][i-1][j  ][k-1] +
                       bp->array[var][i-1][j  ][k  ] +
                       bp->array[var][i-1][j  ][k+1] +
                       bp->array[var][i-1][j+1][k-1] +
                       bp->array[var][i-1][j+1][k  ] +
                       bp->array[var][i-1][j+1][k+1];
                  sm = bp->array[var][i  ][j-1][k-1] +
                       bp->array[var][i  ][j-1][k  ] +
                       bp->array[var][i  ][j-1][k+1] +
                       bp->array[var][i  ][j  ][k-1] +
                       bp->array[var][i  ][j  ][k  ] +
                       bp->array[var][i  ][j  ][k+1] +
                       bp->array[var][i  ][j+1][k-1] +
                       bp->array[var][i  ][j+1][k  ] +
                       bp->array[var][i  ][j+1][k+1];
                  sf = bp->array[var][i+1][j-1][k-1] +
                       bp->array[var][i+1][j-1][k  ] +
                       bp->array[var][i+1][j-1][k+1] +
                       bp->array[var][i+1][j  ][k-1] +
                       bp->array[var][i+1][j  ][k  ] +
                       bp->array[var][i+1][j  ][k+1] +
                       bp->array[var][i+1][j+1][k-1] +
                       bp->array[var][i+1][j+1][k  ] +
                       bp->array[var][i+1][j+1][k+1];
                  work[i][j][k] = (sb + sm + sf)/27.0;
               }
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = work[i][j][k];
      }
      total_fp_divs += (double) num_active*num_cells;
      total_fp_adds += (double) 26*num_active*num_cells;
   }
}

void stencil_0(int var)
{
   int in, i, j, k, v;
   block *bp;

   if (var == 1) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  for (v = mat; v < 2*mat; v++)
                     bp->array[var][i][j][k] += bp->array[v][i][j][k]*
                                                bp->array[0][i][j][k];
      }
      total_fp_adds += (double) mat*num_active*num_cells;
      total_fp_muls += (double) mat*num_active*num_cells;
    } else if (var < mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] += bp->array[var][i][j][k]*
                                             (bp->array[0][i][j][k] +
                                              bp->array[1][i][j][k] -
                                              a1*bp->array[var][i][j][k]);
      }
      total_fp_adds += (double) 3*num_active*num_cells;
      total_fp_muls += (double) 2*num_active*num_cells;
   } else if (var < 2*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = bp->array[var][i][j][k]*
                                            (bp->array[0][i][j][k] +
                                             bp->array[var][i][j][k] +
                                             a1*bp->array[var+mat][i][j][k] +
                                       (1.0-a1)*bp->array[var+2*mat][i][j][k])/
                                            bp->array[1][i][j][k];
      }
      total_fp_adds += (double) 3*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 3*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] += bp->array[var-mat][i][j][k]*
                                             (a1*bp->array[0][i][j][k] +
                                        a0[var-2*mat]*bp->array[var][i][j][k] +
                                         (1.0-a1)*bp->array[var+mat][i][j][k])/
                                             bp->array[1][i][j][k];
         }
      total_fp_adds += (double) 4*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] += bp->array[var-2*mat][i][j][k]*
                                             (a1*bp->array[0][i][j][k] +
                                        a0[var-3*mat]*bp->array[var][i][j][k] +
                              (1.0-a0[var-3*mat])*bp->array[var-mat][i][j][k] +
                                       (1.0-a1)*bp->array[var-2*mat][i][j][k])/
                                             (bp->array[1][i][j][k]*
                                              bp->array[1][i][j][k]);
         }
      total_fp_adds += (double) 6*num_active*num_cells;
      total_fp_muls += (double) 6*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   }
}

void stencil_x(int var)
{
   int in, i, j, k, v;
   double tmp1, tmp2;
   block *bp;

   if (var == 1) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++) {
                  for (v = 2; v < mat+2; v++)
                     bp->array[1][i][j][k] += bp->array[v][i][j][k]*
                                              bp->array[0][i][j][k];
                  bp->array[1][i][j][k] /= (a1 + bp->array[1][i][j][k]);
               }
      }
      total_fp_adds += (double) (mat+1)*num_active*num_cells;
      total_fp_muls += (double) mat*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] += bp->array[var][i][j][k]*
                                             (bp->array[0][i][j][k] +
                                              bp->array[1][i][j][k] -
                                              a1*bp->array[var][i][j][k])/
                                             (a0[var] + bp->array[1][i][j][k]);
      }
      total_fp_adds += (double) 4*num_active*num_cells;
      total_fp_muls += (double) 2*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 2*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  if ((tmp1 = fabs(bp->array[var][i][j][k] -
                                   bp->array[var][i-1][j][k])) >
                      (tmp2 = fabs(bp->array[var][i][j][k] -
                                   bp->array[var][i+1][j][k])))
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i-1][j][k] +
                                         (tmp1-tmp2)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i+1][j][k])/
                                              (a1 + a0[var-mat] +
                                               bp->array[var][i-1][j][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i+1][j][k] +
                                               bp->array[0][i][j][k] +
                                               bp->array[1][i][j][k]);
                  else
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i-1][j][k] +
                                         (tmp2-tmp1)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i+1][j][k])/
                                              (a1 + a0[var-mat] +
                                               bp->array[var][i-1][j][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i+1][j][k] +
                                               bp->array[0][i][j][k] +
                                               bp->array[1][i][j][k]);
      }
      total_fp_adds += (double) 12*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 3*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  if ((tmp1 = fabs(bp->array[var-mat][i][j][k] -
                                   bp->array[var-mat][i-1][j][k])) >
                      (tmp2 = fabs(bp->array[var-mat][i][j][k] -
                                   bp->array[var-mat][i+1][j][k])))
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i-1][j][k] +
                                         (tmp1-tmp2)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i+1][j][k])/
                                              (a1 + a0[var-2*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var+mat][i][j][k] +
                                               bp->array[var][i-1][j][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i+1][j][k]);
                  else
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i-1][j][k] +
                                         (tmp2-tmp1)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i+1][j][k])/
                                              (a1 + a0[var-2*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var+mat][i][j][k] +
                                               bp->array[var][i-1][j][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i+1][j][k]);
      }
      total_fp_adds += (double) 12*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  if ((tmp1 = fabs(bp->array[var-2*mat][i][j][k] -
                                   bp->array[var-2*mat][i-1][j][k])) >
                      (tmp2 = fabs(bp->array[var-2*mat][i][j][k] -
                                   bp->array[var-2*mat][i+1][j][k])))
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i-1][j][k] +
                                         (tmp1-tmp2)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i+1][j][k])/
                                              (a1 + a0[var-3*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var-2*mat][i][j][k] +
                                               bp->array[var][i-1][j][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i+1][j][k]);
                  else
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i-1][j][k] +
                                         (tmp2-tmp1)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i+1][j][k])/
                                              (a1 + a0[var-3*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var-2*mat][i][j][k] +
                                               bp->array[var][i-1][j][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i+1][j][k]);
      }
      total_fp_adds += (double) 12*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   }
}

void stencil_y(int var)
{
   int in, i, j, k, v;
   double tmp1, tmp2;
   block *bp;

   if (var == 1) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++) {
                  for (v = 2; v < mat+2; v++)
                     bp->array[1][i][j][k] += bp->array[v][i][j][k]*
                                              bp->array[0][i][j][k];
                  bp->array[1][i][j][k] /= (a1 + bp->array[1][i][j][k]);
               }
      }
      total_fp_adds += (double) (mat+1)*num_active*num_cells;
      total_fp_muls += (double) mat*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] += bp->array[var][i][j][k]*
                                             (bp->array[0][i][j][k] +
                                              bp->array[1][i][j][k] -
                                              a1*bp->array[var][i][j][k])/
                                             (a0[var] + bp->array[1][i][j][k]);
      }
      total_fp_adds += (double) 4*num_active*num_cells;
      total_fp_muls += (double) 2*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 2*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  if ((tmp1 = fabs(bp->array[var][i][j][k] -
                                   bp->array[var][i][j-1][k])) >
                      (tmp2 = fabs(bp->array[var][i][j][k] -
                                   bp->array[var][i][j+1][k])))
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j-1][k] +
                                         (tmp1-tmp2)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j+1][k])/
                                              (a1 + a0[var-mat] +
                                               bp->array[var][i][j-1][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j+1][k] +
                                               bp->array[0][i][j][k] +
                                               bp->array[1][i][j][k]);
                  else
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j-1][k] +
                                         (tmp2-tmp1)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j+1][k])/
                                              (a1 + a0[var-mat] +
                                               bp->array[var][i][j-1][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j+1][k] +
                                               bp->array[0][i][j][k] +
                                               bp->array[1][i][j][k]);
      }
      total_fp_adds += (double) 12*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 3*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  if ((tmp1 = fabs(bp->array[var-mat][i][j][k] -
                                   bp->array[var-mat][i][j-1][k])) >
                      (tmp2 = fabs(bp->array[var-mat][i][j][k] -
                                   bp->array[var-mat][i][j+1][k])))
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j-1][k] +
                                         (tmp1-tmp2)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j+1][k])/
                                              (a1 + a0[var-2*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var+mat][i][j][k] +
                                               bp->array[var][i][j-1][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j+1][k]);
                  else
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j-1][k] +
                                         (tmp2-tmp1)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j+1][k])/
                                              (a1 + a0[var-2*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var+mat][i][j][k] +
                                               bp->array[var][i][j-1][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j+1][k]);
      }
      total_fp_adds += (double) 12*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  if ((tmp1 = fabs(bp->array[var-2*mat][i][j][k] -
                                   bp->array[var-2*mat][i][j-1][k])) >
                      (tmp2 = fabs(bp->array[var-2*mat][i][j][k] -
                                   bp->array[var-2*mat][i][j+1][k])))
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j-1][k] +
                                         (tmp1-tmp2)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j+1][k])/
                                              (a1 + a0[var-3*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var-2*mat][i][j][k] +
                                               bp->array[var][i][j-1][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j+1][k]);
                  else
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j-1][k] +
                                         (tmp2-tmp1)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j+1][k])/
                                              (a1 + a0[var-3*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var-2*mat][i][j][k] +
                                               bp->array[var][i][j-1][k] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j+1][k]);
      }
      total_fp_adds += (double) 12*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   }
}

void stencil_z(int var)
{
   int in, i, j, k, v;
   double tmp1, tmp2;
   block *bp;


   if (var == 1) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++) {
                  for (v = 2; v < mat+2; v++)
                     bp->array[1][i][j][k] += bp->array[v][i][j][k]*
                                              bp->array[0][i][j][k];
                  bp->array[1][i][j][k] /= (a1 + bp->array[1][i][j][k]);
               }
      }
      total_fp_adds += (double) (mat+1)*num_active*num_cells;
      total_fp_muls += (double) mat*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] += bp->array[var][i][j][k]*
                                             (bp->array[0][i][j][k] +
                                              bp->array[1][i][j][k] -
                                              a1*bp->array[var][i][j][k])/
                                             (a0[var] + bp->array[1][i][j][k]);
      }
      total_fp_adds += (double) 4*num_active*num_cells;
      total_fp_muls += (double) 2*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 2*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  if ((tmp1 = fabs(bp->array[var][i][j][k] -
                                   bp->array[var][i][j][k-1])) >
                      (tmp2 = fabs(bp->array[var][i][j][k] -
                                   bp->array[var][i][j][k+1])))
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j][k-1] +
                                         (tmp1-tmp2)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j][k+1])/
                                              (a1 + a0[var-mat] +
                                               bp->array[var][i][j][k-1] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j][k+1] +
                                               bp->array[0][i][j][k] +
                                               bp->array[1][i][j][k]);
                  else
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j][k-1] +
                                         (tmp2-tmp1)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j][k+1])/
                                              (a1 + a0[var-mat] +
                                               bp->array[var][i][j][k-1] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j][k+1] +
                                               bp->array[0][i][j][k] +
                                               bp->array[1][i][j][k]);
      }
      total_fp_adds += (double) 12*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 3*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  if ((tmp1 = fabs(bp->array[var-mat][i][j][k] -
                                   bp->array[var-mat][i][j][k-1])) >
                      (tmp2 = fabs(bp->array[var-mat][i][j][k] -
                                   bp->array[var-mat][i][j][k+1])))
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j][k-1] +
                                         (tmp1-tmp2)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j][k+1])/
                                              (a1 + a0[var-2*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var+mat][i][j][k] +
                                               bp->array[var][i][j][k-1] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j][k+1]);
                  else
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j][k-1] +
                                         (tmp2-tmp1)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j][k+1])/
                                              (a1 + a0[var-2*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var+mat][i][j][k] +
                                               bp->array[var][i][j][k-1] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j][k+1]);
      }
      total_fp_adds += (double) 12*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  if ((tmp1 = fabs(bp->array[var-2*mat][i][j][k] -
                                   bp->array[var-2*mat][i][j][k-1])) >
                      (tmp2 = fabs(bp->array[var-2*mat][i][j][k] -
                                   bp->array[var-2*mat][i][j][k+1])))
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j][k-1] +
                                         (tmp1-tmp2)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j][k+1])/
                                              (a1 + a0[var-3*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var-2*mat][i][j][k] +
                                               bp->array[var][i][j][k-1] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j][k+1]);
                  else
                    bp->array[var][i][j][k] = (tmp1*bp->array[var][i][j][k-1] +
                                         (tmp2-tmp1)*(bp->array[var][i][j][k] +
                                                       bp->array[1][i][j][k]) +
                                               tmp2*bp->array[var][i][j][k+1])/
                                              (a1 + a0[var-3*mat] +
                                               bp->array[var-mat][i][j][k] +
                                               bp->array[var-2*mat][i][j][k] +
                                               bp->array[var][i][j][k-1] +
                                               bp->array[var][i][j][k] +
                                               bp->array[var][i][j][k+1]);
      }
      total_fp_adds += (double) 12*num_active*num_cells;
      total_fp_muls += (double) 3*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   }
}

void stencil_7(int var)
{
   int in, i, j, k, v;
   double work[x_block_size+2][y_block_size+2][z_block_size+2];
   block *bp;

   if (var < mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  work[i][j][k] = (bp->array[var      ][i-1][j  ][k  ]*
                                   bp->array[var+  mat][i-1][j  ][k  ] +
                                   bp->array[var      ][i  ][j-1][k  ]*
                                   bp->array[var+2*mat][i  ][j-1][k  ] +
                                   bp->array[var      ][i  ][j  ][k-1]*
                                   bp->array[var+3*mat][i  ][j  ][k-1] +
                                   bp->array[var      ][i  ][j  ][k  ]*
                                   bp->array[var      ][i  ][j  ][k  ] +
                                   bp->array[var      ][i  ][j  ][k+1]*
                                   bp->array[var+3*mat][i  ][j  ][k+1] +
                                   bp->array[var      ][i  ][j+1][k  ]*
                                   bp->array[var+2*mat][i  ][j+1][k  ] +
                                   bp->array[var      ][i+1][j  ][k  ]*
                                   bp->array[var+  mat][i+1][j  ][k  ])/
                                   7.0*(a1 + bp->array[var][i][j][k]);
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = work[i][j][k];
      }
      total_fp_adds += (double) 7*num_active*num_cells;
      total_fp_muls += (double) 8*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 2*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  work[i][j][k] = (bp->array[var      ][i-1][j  ][k  ]*
                                   bp->array[var+  mat][i-1][j  ][k  ] +
                                   bp->array[var      ][i  ][j-1][k  ]*
                                   bp->array[var+2*mat][i  ][j-1][k  ] +
                                   bp->array[var      ][i  ][j  ][k-1]*
                                   bp->array[var-  mat][i  ][j  ][k-1] +
                                   bp->array[var      ][i  ][j  ][k  ]*
                                   bp->array[var      ][i  ][j  ][k  ] +
                                   bp->array[var      ][i  ][j  ][k+1]*
                                   bp->array[var-  mat][i  ][j  ][k+1] +
                                   bp->array[var      ][i  ][j+1][k  ]*
                                   bp->array[var+2*mat][i  ][j+1][k  ] +
                                   bp->array[var      ][i+1][j  ][k  ]*
                                   bp->array[var+  mat][i+1][j  ][k  ])/
                                   7.0*(a1 + bp->array[var][i][j][k]);
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = work[i][j][k];
      }
      total_fp_adds += (double) 7*num_active*num_cells;
      total_fp_muls += (double) 8*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 3*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  work[i][j][k] = (bp->array[var      ][i-1][j  ][k  ]*
                                   bp->array[var+  mat][i-1][j  ][k  ] +
                                   bp->array[var      ][i  ][j-1][k  ]*
                                   bp->array[var-2*mat][i  ][j-1][k  ] +
                                   bp->array[var      ][i  ][j  ][k-1]*
                                   bp->array[var-  mat][i  ][j  ][k-1] +
                                   bp->array[var      ][i  ][j  ][k  ]*
                                   bp->array[var      ][i  ][j  ][k  ] +
                                   bp->array[var      ][i  ][j  ][k+1]*
                                   bp->array[var-  mat][i  ][j  ][k+1] +
                                   bp->array[var      ][i  ][j+1][k  ]*
                                   bp->array[var-2*mat][i  ][j+1][k  ] +
                                   bp->array[var      ][i+1][j  ][k  ]*
                                   bp->array[var+  mat][i+1][j  ][k  ])/
                                   7.0*(a1 + bp->array[var][i][j][k]);
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = work[i][j][k];
      }
      total_fp_adds += (double) 7*num_active*num_cells;
      total_fp_muls += (double) 8*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  work[i][j][k] = (bp->array[var      ][i-1][j  ][k  ]*
                                   bp->array[var-3*mat][i-1][j  ][k  ] +
                                   bp->array[var      ][i  ][j-1][k  ]*
                                   bp->array[var-2*mat][i  ][j-1][k  ] +
                                   bp->array[var      ][i  ][j  ][k-1]*
                                   bp->array[var-  mat][i  ][j  ][k-1] +
                                   bp->array[var      ][i  ][j  ][k  ]*
                                   bp->array[var      ][i  ][j  ][k  ] +
                                   bp->array[var      ][i  ][j  ][k+1]*
                                   bp->array[var-  mat][i  ][j  ][k+1] +
                                   bp->array[var      ][i  ][j+1][k  ]*
                                   bp->array[var-2*mat][i  ][j+1][k  ] +
                                   bp->array[var      ][i+1][j  ][k  ]*
                                   bp->array[var-3*mat][i+1][j  ][k  ])/
                                   7.0*(a1 + bp->array[var][i][j][k]);
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = work[i][j][k];
      }
      total_fp_adds += (double) 7*num_active*num_cells;
      total_fp_muls += (double) 8*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   }
}

void stencil_27(int var)
{
   int in, i, j, k, v;
   double work[x_block_size+2][y_block_size+2][z_block_size+2];
   block *bp;

   if (var < mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  work[i][j][k] = (bp->array[var+3*mat][i-1][j-1][k-1] +
                                   bp->array[var+2*mat][i-1][j-1][k  ] +
                                   bp->array[var+3*mat][i-1][j-1][k+1] +
                                   bp->array[var+2*mat][i-1][j  ][k-1] +
                                   bp->array[var+  mat][i-1][j  ][k  ] +
                                   bp->array[var+2*mat][i-1][j  ][k+1] +
                                   bp->array[var+3*mat][i-1][j+1][k-1] +
                                   bp->array[var+2*mat][i-1][j+1][k  ] +
                                   bp->array[var+3*mat][i-1][j+1][k+1] +
                                   bp->array[var+2*mat][i  ][j-1][k-1] +
                                   bp->array[var+  mat][i  ][j-1][k  ] +
                                   bp->array[var+2*mat][i  ][j-1][k+1] +
                                   bp->array[var+  mat][i  ][j  ][k-1] +
                                   bp->array[var      ][i  ][j  ][k  ] +
                                   bp->array[var+  mat][i  ][j  ][k+1] +
                                   bp->array[var+2*mat][i  ][j+1][k-1] +
                                   bp->array[var+  mat][i  ][j+1][k  ] +
                                   bp->array[var+2*mat][i  ][j+1][k+1] +
                                   bp->array[var+3*mat][i+1][j-1][k-1] +
                                   bp->array[var+2*mat][i+1][j-1][k  ] +
                                   bp->array[var+3*mat][i+1][j-1][k+1] +
                                   bp->array[var+2*mat][i+1][j  ][k-1] +
                                   bp->array[var+  mat][i+1][j  ][k  ] +
                                   bp->array[var+2*mat][i+1][j  ][k+1] +
                                   bp->array[var+3*mat][i+1][j+1][k-1] +
                                   bp->array[var+2*mat][i+1][j+1][k  ] +
                                   bp->array[var+3*mat][i+1][j+1][k+1])/
                                  (a1+27.0);
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = work[i][j][k];
      }
      total_fp_adds += (double) 27*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 2*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  work[i][j][k] = (bp->array[var-  mat][i-1][j-1][k-1] +
                                   bp->array[var+2*mat][i-1][j-1][k  ] +
                                   bp->array[var-  mat][i-1][j-1][k+1] +
                                   bp->array[var+2*mat][i-1][j  ][k-1] +
                                   bp->array[var+  mat][i-1][j  ][k  ] +
                                   bp->array[var+2*mat][i-1][j  ][k+1] +
                                   bp->array[var-  mat][i-1][j+1][k-1] +
                                   bp->array[var+2*mat][i-1][j+1][k  ] +
                                   bp->array[var-  mat][i-1][j+1][k+1] +
                                   bp->array[var+2*mat][i  ][j-1][k-1] +
                                   bp->array[var+  mat][i  ][j-1][k  ] +
                                   bp->array[var+2*mat][i  ][j-1][k+1] +
                                   bp->array[var+  mat][i  ][j  ][k-1] +
                                   bp->array[var      ][i  ][j  ][k  ] +
                                   bp->array[var+  mat][i  ][j  ][k+1] +
                                   bp->array[var+2*mat][i  ][j+1][k-1] +
                                   bp->array[var+  mat][i  ][j+1][k  ] +
                                   bp->array[var+2*mat][i  ][j+1][k+1] +
                                   bp->array[var-  mat][i+1][j-1][k-1] +
                                   bp->array[var+2*mat][i+1][j-1][k  ] +
                                   bp->array[var-  mat][i+1][j-1][k+1] +
                                   bp->array[var+2*mat][i+1][j  ][k-1] +
                                   bp->array[var+  mat][i+1][j  ][k  ] +
                                   bp->array[var+2*mat][i+1][j  ][k+1] +
                                   bp->array[var-  mat][i+1][j+1][k-1] +
                                   bp->array[var+2*mat][i+1][j+1][k  ] +
                                   bp->array[var-  mat][i+1][j+1][k+1])/
                                  (a1+27.0);
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = work[i][j][k];
      }
      total_fp_adds += (double) 27*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else if (var < 3*mat) {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  work[i][j][k] = (bp->array[var-  mat][i-1][j-1][k-1] +
                                   bp->array[var-2*mat][i-1][j-1][k  ] +
                                   bp->array[var-  mat][i-1][j-1][k+1] +
                                   bp->array[var-2*mat][i-1][j  ][k-1] +
                                   bp->array[var+  mat][i-1][j  ][k  ] +
                                   bp->array[var-2*mat][i-1][j  ][k+1] +
                                   bp->array[var-  mat][i-1][j+1][k-1] +
                                   bp->array[var-2*mat][i-1][j+1][k  ] +
                                   bp->array[var-  mat][i-1][j+1][k+1] +
                                   bp->array[var-2*mat][i  ][j-1][k-1] +
                                   bp->array[var+  mat][i  ][j-1][k  ] +
                                   bp->array[var-2*mat][i  ][j-1][k+1] +
                                   bp->array[var+  mat][i  ][j  ][k-1] +
                                   bp->array[var      ][i  ][j  ][k  ] +
                                   bp->array[var+  mat][i  ][j  ][k+1] +
                                   bp->array[var-2*mat][i  ][j+1][k-1] +
                                   bp->array[var+  mat][i  ][j+1][k  ] +
                                   bp->array[var-2*mat][i  ][j+1][k+1] +
                                   bp->array[var-  mat][i+1][j-1][k-1] +
                                   bp->array[var-2*mat][i+1][j-1][k  ] +
                                   bp->array[var-  mat][i+1][j-1][k+1] +
                                   bp->array[var-2*mat][i+1][j  ][k-1] +
                                   bp->array[var+  mat][i+1][j  ][k  ] +
                                   bp->array[var-2*mat][i+1][j  ][k+1] +
                                   bp->array[var-  mat][i+1][j+1][k-1] +
                                   bp->array[var-2*mat][i+1][j+1][k  ] +
                                   bp->array[var-  mat][i+1][j+1][k+1])/
                                  (a1+27.0);
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = work[i][j][k];
      }
      total_fp_adds += (double) 27*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   } else {
      for (in = 0; in < sorted_index[num_refine+1]; in++) {
         bp = &blocks[sorted_list[in].n];
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  work[i][j][k] = (bp->array[var-  mat][i-1][j-1][k-1] +
                                   bp->array[var-2*mat][i-1][j-1][k  ] +
                                   bp->array[var-  mat][i-1][j-1][k+1] +
                                   bp->array[var-2*mat][i-1][j  ][k-1] +
                                   bp->array[var-3*mat][i-1][j  ][k  ] +
                                   bp->array[var-2*mat][i-1][j  ][k+1] +
                                   bp->array[var-  mat][i-1][j+1][k-1] +
                                   bp->array[var-2*mat][i-1][j+1][k  ] +
                                   bp->array[var-  mat][i-1][j+1][k+1] +
                                   bp->array[var-2*mat][i  ][j-1][k-1] +
                                   bp->array[var-3*mat][i  ][j-1][k  ] +
                                   bp->array[var-2*mat][i  ][j-1][k+1] +
                                   bp->array[var-3*mat][i  ][j  ][k-1] +
                                   bp->array[var      ][i  ][j  ][k  ] +
                                   bp->array[var-3*mat][i  ][j  ][k+1] +
                                   bp->array[var-2*mat][i  ][j+1][k-1] +
                                   bp->array[var-3*mat][i  ][j+1][k  ] +
                                   bp->array[var-2*mat][i  ][j+1][k+1] +
                                   bp->array[var-  mat][i+1][j-1][k-1] +
                                   bp->array[var-2*mat][i+1][j-1][k  ] +
                                   bp->array[var-  mat][i+1][j-1][k+1] +
                                   bp->array[var-2*mat][i+1][j  ][k-1] +
                                   bp->array[var-3*mat][i+1][j  ][k  ] +
                                   bp->array[var-2*mat][i+1][j  ][k+1] +
                                   bp->array[var-  mat][i+1][j+1][k-1] +
                                   bp->array[var-2*mat][i+1][j+1][k  ] +
                                   bp->array[var-  mat][i+1][j+1][k+1])/
                                  (a1+27.0);
         for (i = 1; i <= x_block_size; i++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++)
                  bp->array[var][i][j][k] = work[i][j][k];
      }
      total_fp_adds += (double) 27*num_active*num_cells;
      total_fp_divs += (double) num_active*num_cells;
   }
}

void stencil_check(int var)
{
   int in, i, j, k, v;
   block *bp;

   for (in = 0; in < sorted_index[num_refine+1]; in++) {
      bp = &blocks[sorted_list[in].n];
      for (i = 1; i <= x_block_size; i++)
         for (j = 1; j <= y_block_size; j++)
            for (k = 1; k <= z_block_size; k++) {
               bp->array[var][i][j][k] = fabs(bp->array[var][i][j][k]);
               if (bp->array[var][i][j][k] >= 1.0) {
                  bp->array[var][i][j][k] /= (a1 + a0[0] +
                                              bp->array[var][i][j][k]);
                  total_fp_divs += (double) 1;
                  total_fp_adds += (double) 2;
               }
               else if (bp->array[var][i][j][k] < 0.1) {
                  bp->array[var][i][j][k] *= 10.0 - a1;
                  total_fp_muls += (double) 1;
                  total_fp_adds += (double) 1;
               }
            }
   }
}
