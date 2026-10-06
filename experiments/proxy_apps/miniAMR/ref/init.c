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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#include "block.h"
#include "comm.h"
#include "hilbert.h"
#include "proto.h"

// Initialize the problem and setup initial blocks.
void init(void)
{
   int n, var, i, j, k, l, m, o, size, dir, i1, i2, j1, j2, k1, k2, ib, jb, kb,
       *start, *pos[3], pos1[init_x][init_y][init_z], set, npx1, npy1, npz1,
       pes, fact, fac[25], nfac, f, nhs, div[30][2], n_hs[3], fa[3][10], n2[3],
       n2m, nf[3];
   num_sz num;
   block *bp;

   tol = pow(10.0, ((double) -error_tol));

   total_fp_divs = total_fp_adds = total_fp_muls = 0.0;
   p2[0] = p8[0] = 1;
   for (i = 0; i < (num_refine+1); i++) {
      p8[i+1] = p8[i]*8;
      p2[i+1] = p2[i]*2;
      sorted_index[i] = 0;
   }
   sorted_index[num_refine+1] = 0;
   block_start[0] = 0;
   local_max_b = global_max_b = init_block_x*init_block_y*init_block_z;
   num = num_pes*global_max_b;
   for (i = 1; i <= num_refine; i++) {
      block_start[i] = block_start[i-1] + num;
      num *= 8;
      num_blocks[i] = 0;
      local_num_blocks[i] = 0;
   }

   /* initialize for communication arrays, which are initialized below */
   zero_comm_list();
   par_b.num_comm_part = par_b.num_cases = 0;
   par_p.num_comm_part = par_p.num_cases = 0;

   num_cells = x_block_size*y_block_size*z_block_size;
   x_block_half = x_block_size/2;
   y_block_half = y_block_size/2;
   z_block_half = z_block_size/2;

#ifndef FPC_MINIAMR_DISABLE_INJECTION
   {
      volatile double base_val = 123456.789;
      double inj_x = base_val;
      double inj_y = base_val + 1.0e-11;
      volatile double cancelled_result __attribute__((unused)) = inj_y - inj_x; // Injection standalone

      double state_x = (tol + (double) num_cells + (double) num_vars + 1.0) * 123456.789;
      double state_y = state_x + state_x * 1.0e-15;
      volatile double state_cancelled_result __attribute__((unused)) = state_y - state_x; // Injection state
   }
#endif

   if (!code) {
      /* for E/W (X dir) messages:
         0: whole -> whole (7), 1: whole -> whole (27),
         2: whole -> quarter, 3: quarter -> whole */
      msg_len[0][0] = msg_len[0][1] = y_block_size*z_block_size;
      msg_len[0][2] = msg_len[0][3] = y_block_half*z_block_half;
      /* for N/S (Y dir) messages */
      msg_len[1][0] = x_block_size*z_block_size;
      msg_len[1][1] = (x_block_size+2)*z_block_size;
      msg_len[1][2] = msg_len[1][3] = x_block_half*z_block_half;
      /* for U/D (Z dir) messages */
      msg_len[2][0] = x_block_size*y_block_size;
      msg_len[2][1] = (x_block_size+2)*(y_block_size+2);
      msg_len[2][2] = msg_len[2][3] = x_block_half*y_block_half;
   } else if (code == 1) {
      /* for E/W (X dir) messages */
      msg_len[0][0] = msg_len[0][1] = (y_block_size+2)*(z_block_size+2);
      msg_len[0][2] = (y_block_half+1)*(z_block_half+1);
      msg_len[0][3] = (y_block_half+2)*(z_block_half+2);
      /* for N/S (Y dir) messages */
      msg_len[1][0] = msg_len[1][1] = (x_block_size+2)*(z_block_size+2);
      msg_len[1][2] = (x_block_half+1)*(z_block_half+1);
      msg_len[1][3] = (x_block_half+2)*(z_block_half+2);
      /* for U/D (Z dir) messages */
      msg_len[2][0] = msg_len[2][1] = (x_block_size+2)*(y_block_size+2);
      msg_len[2][2] = (x_block_half+1)*(y_block_half+1);
      msg_len[2][3] = (x_block_half+2)*(y_block_half+2);
   } else {
      /* for E/W (X dir) messages */
      msg_len[0][0] = msg_len[0][1] = (y_block_size+2)*(z_block_size+2);
      msg_len[0][2] = (y_block_half+1)*(z_block_half+1);
      msg_len[0][3] = (y_block_size+2)*(z_block_size+2);
      /* for N/S (Y dir) messages */
      msg_len[1][0] = msg_len[1][1] = (x_block_size+2)*(z_block_size+2);
      msg_len[1][2] = (x_block_half+1)*(z_block_half+1);
      msg_len[1][3] = (x_block_size+2)*(z_block_size+2);
      /* for U/D (Z dir) messages */
      msg_len[2][0] = msg_len[2][1] = (x_block_size+2)*(y_block_size+2);
      msg_len[2][2] = (x_block_half+1)*(y_block_half+1);
      msg_len[2][3] = (x_block_size+2)*(y_block_size+2);
   }

   // Some initialization for initial block positioning
   if (lb_method < 2) {
      start = (int *) ma_malloc(num_pes*sizeof(int), __FILE__, __LINE__);
      for (i = 0; i < 3; i++) {
         pos[i] = (int *) ma_malloc(num_pes*sizeof(int), __FILE__, __LINE__);
         for (j = 0; j < num_pes; j++)
            pos[i][j] = 0;
      }
      init_x *= init_block_x;
      init_y *= init_block_y;
      init_z *= init_block_z;
   }

   /* Determine position of each core in initial mesh */
   npx1 = npx;
   npy1 = npy;
   npz1 = npz;
   nfac = factor(num_pes, fac);
   max_num_req = num_pes;
   request = (MPI_Request *) ma_malloc(max_num_req*sizeof(MPI_Request),
                                       __FILE__, __LINE__);
   if (nonblocking)
      s_req = (MPI_Request *) ma_malloc(max_num_req*sizeof(MPI_Request),
                                        __FILE__, __LINE__);
   pes = 1;
   if (lb_method < 2)
      start[0] = 0;
   size = num_pes;
   comms = (MPI_Comm *) ma_malloc((nfac+1)*sizeof(MPI_Comm),
                                  __FILE__, __LINE__);
   me = (int *) ma_malloc((nfac+1)*sizeof(int), __FILE__, __LINE__);
   np = (int *) ma_malloc((nfac+1)*sizeof(int), __FILE__, __LINE__);
   dirs = (int *) ma_malloc(nfac*sizeof(int), __FILE__, __LINE__);
   comms[0] = MPI_COMM_WORLD;
   me[0] = my_pe;
   np[0] = num_pes;
   // Initialize for all load balance methods
   for (n = 0, i = nfac; i > 0; i--, n++) {
      fact = fac[i-1];
      dir = find_dir(fact, npx1, npy1, npz1);
      if (dir == 0)
         npx1 /= fact;
      else
         if (dir == 1)
            npy1 /= fact;
         else
            npz1 /= fact;
      dirs[n] = dir;
      size /= fact;
      set = me[n]/size;
      MPI_Comm_split(comms[n], set, me[n], &comms[n+1]);
      MPI_Comm_rank(comms[n+1], &me[n+1]);
      MPI_Comm_size(comms[n+1], &np[n+1]);
      if (lb_method < 2) {
         // Establish block ordering for RCB and Morton SFC
         for (j = pes-1; j >= 0; j--)
            for (k = 0; k < fact; k++) {
               m = j*fact + k;
               if (!k)
                  start[m] = start[j];
               else
                  start[m] = start[m-1] + size;
               for (l = start[m], o = 0; o < size; l++, o++)
                  pos[dir][l] = pos[dir][l]*fact + k;
            }
         pes *= fact;
      }
   }

   if (lb_method < 2)
      for (i = 0; i < num_pes; i++)
         pos1[pos[0][i]][pos[1][i]][pos[2][i]] = i;
   else if (lb_method == 2) {
      // initialize
      nhs = init_x*init_y*init_z;
      start = (int *) ma_malloc((nhs+1)*sizeof(int), __FILE__, __LINE__);
      for (i = 0; i < 3; i++) {
         pos[i] = (int *) ma_malloc((nhs+1)*sizeof(int), __FILE__, __LINE__);
         for (j = 0; j < nhs+1; j++)
            pos[i][j] = 0;
      }

      // Establish block ordering for Peano Hilbert SFC
      n_hs[0] = init_x;
      n_hs[1] = init_y;
      n_hs[2] = init_z;
      for (i = 0; i < 3; i++) {
         nf[i] = factor(n_hs[i], fa[i]);
         for (n2[i] = j = 0; j < nf[i]; j++)
            if (fa[i][j] == 2)
               n2[i]++;
      }
      // find minimum number of factors of 2 in each direction
      if (n2[0] < n2[1])
         if (n2[0] < n2[2])
            n2m = n2[0];
         else
            n2m = n2[2];
      else
         if (n2[1] < n2[2])
            n2m = n2[1];
         else
            n2m = n2[2];
      for (nfac = i = 0; i < 3; i++) {
         n_hs[i] /= p2[n2m];
         nfac += (nf[i] -= n2m);
      }
      for (j = -1, i = 0; i < nfac; i++) {
         // order the factors and associated directions
         // want largest factors first and try to alternate directions
         if (n_hs[0] > n_hs[1] || (n_hs[0] == n_hs[1] && j == 1))
            if (n_hs[0] > n_hs[2] || (n_hs[0] == n_hs[2] && j == 2))
               dir = 0;
            else
               dir = 2;
         else
            if (n_hs[1] > n_hs[2] || (n_hs[1] == n_hs[2] && j == 2))
               dir = 1;
            else
               dir = 2;
         nf[dir]--;
         div[i][0] = fa[dir][nf[dir]+n2m];
         div[i][1] = j = dir;
         n_hs[dir] /= fa[dir][nf[dir]+n2m];
      }

      pes = 1;
      start[0] = 0;
      size = nhs;
      for (i = 0; i < nfac; i++) {
         if (i != (nfac-1) && div[i][1] == div[i+1][1])
            // if two factors in a row are from the same direction, combine
            div[i+1][0] *= div[i][0];
         else {
            // This may not be quite right, I will look at it again later.
            // The odd factors are fine but more than one factor of 2 can
            // cause some problems resulting in a non continuous curve
            // that there may be no way to resolve.
            size /= div[i][0];
            for (j = pes-1; j >= 0; j--)
               for (k = 0; k < div[i][0]; k++) {
                  m = j*div[i][0] + k;
                  if (!k)
                     start[m] = start[j];
                  else
                     start[m] = start[m-1] + size;
                  if (!((!(pos[(div[i][1]+1)%3][start[m]]%2))^
                     (!(pos[(div[i][1]+2)%3][start[m]]%2))))
                     for (l = start[m], o = 0; o < size; l++, o++)
                        pos[div[i][1]][l] = pos[div[i][1]][l]*div[i][0] + k;
                  else
                     for (l = start[m], o = 0; o < size; l++, o++)
                        pos[div[i][1]][l] = pos[div[i][1]][l]*div[i][0] +
                                            div[i][0] - k - 1;
               }
            pes *= div[i][0];
         }
      }
      // At this point we have an ordering, and we can build a Peano Curve
      // out of the odd factors and the left over factors of two (we need a
      // factor of 2 in each direction to construct a Hilbert Curve).
      // Repurpose start array to contain a block type array which contains
      // a direction where the next block will be and the octant in the
      // current block where the curve runs and k is the starting octant
      // in the next block.
      for (i = 0; i < nhs; i++)
         for (j = 0; j < 3; j++)
            pos[j][i] *= p2[n2m];
      // fictitious last element
      pos[0][nhs] = n_hs[0] + p2[n2m];
      pos[1][nhs] = n_hs[1];
      pos[2][nhs] = n_hs[2];
      for (k = i = 0; i < nhs; i += p8[n2m])
         // In the following if the current box is seperated by more than
         // p2[n2m] then we have a discontinuity.
         if (pos[0][i] != pos[0][i+p8[n2m]])
            if (pos[0][i+p8[n2m]] == (pos[0][i] + p2[n2m]) ||
                pos[0][i+p8[n2m]] < (pos[0][i] - p2[n2m])) {
               start[i] = next[0][k][0];
               k        = next[0][k][1];
            } else {
               start[i] = next[1][k][0];
               k        = next[1][k][1];
            }
         else if (pos[1][i] != pos[1][i+p8[n2m]])
            if (pos[1][i+p8[n2m]] == (pos[1][i] + p2[n2m]) ||
                pos[1][i+p8[n2m]] < (pos[1][i] - p2[n2m])) {
               start[i] = next[2][k][0];
               k        = next[2][k][1];
            } else {
               start[i] = next[3][k][0];
               k        = next[3][k][1];
            }
         else // differs in z direction
            if (pos[2][i+p8[n2m]] == (pos[2][i] + p2[n2m]) ||
                pos[2][i+p8[n2m]] < (pos[2][i] - p2[n2m])) {
               start[i] = next[4][k][0];
               k        = next[4][k][1];
            } else {
               start[i] = next[5][k][0];
               k        = next[5][k][1];
            }
      // Recursively generate Hilbert blocks into the above Peano curve.
      for (j = n2m; j > 0; j--)
         for (i = 0; i < nhs; ) {
            l = start[i];
            i1 = pos[0][i];
            j1 = pos[1][i];
            k1 = pos[2][i];
            for (k = 0; k < 8; k++, i += p8[j-1]) {
               m = hilbert[l][k][0];   //nominal block number
               start[i] = hilbert[l][k][1];
               pos[0][i] = i1 + (m%2)*p2[j-1];
               pos[1][i] = j1 + ((m/2)%2)*p2[j-1];
               pos[2][i] = k1 + (m/4)*p2[j-1];
            }
         }
      for (i = 0; i < nhs; i++)
         pos1[pos[0][i]][pos[1][i]][pos[2][i]] = i;
      // end of Peano Hilbert SFC initilization
   } else { // truncated Hilbert SFC initialization
      // determine largest direction and then the power of two that contains it
      i = init_x;
      if (init_y > i)
         i = init_y;
      if (init_z > i)
         i = init_z;
      for (n2m = 0; p2[n2m] < i; n2m++)
         ;

      // allocate memory
      nhs = p8[n2m];
      start = (int *) ma_malloc((nhs)*sizeof(int), __FILE__, __LINE__);
      for (i = 0; i < nhs; i++)
         start[i] = 0;
      for (i = 0; i < 3; i++) {
         pos[i] = (int *) ma_malloc((nhs)*sizeof(int), __FILE__, __LINE__);
         for (j = 0; j < nhs; j++)
            pos[i][j] = 0;
      }

      // Recursively generate Hilbert blocks starting with unit cube
      for (j = n2m; j > 0; j--)
         for (i = 0; i < nhs; ) {
            l = start[i];
            i1 = pos[0][i];
            j1 = pos[1][i];
            k1 = pos[2][i];
            for (k = 0; k < 8; k++, i += p8[j-1]) {
               m = hilbert[l][k][0];   //nominal block number
               start[i] = hilbert[l][k][1];
               pos[0][i] = i1 + (m%2)*p2[j-1];
               pos[1][i] = j1 + ((m/2)%2)*p2[j-1];
               pos[2][i] = k1 + (m/4)*p2[j-1];
            }
         }

      // Pull out the truncated portion that is used
      // Variables for i == 0 will always exist, so this is safe
      for (i = j = 0; i < nhs; i++)
         if (pos[0][i] < init_x && pos[1][i] < init_y && pos[2][i] < init_z) {
            pos1[pos[0][i]][pos[1][i]][pos[2][i]] = j;
            if (i != j) // correct previous block orientation and try on this
               if (pos[0][i] != pos[0][i1]) {
                  l = start[j1]%2;
                  m = start[j1]%8;
                  if ((pos[0][i] == (pos[0][i1] + 1)) ^ (l == 0)) {
                     start[j1] = 8 + m;
                     start[j] = (m/4)*4 + (m + 2)%4;
                  } else {
                     start[j1] = m;
                     start[j] = m;
                  }
               } else if (pos[1][i] != pos[1][i1]) {
                  l = start[j1]%4;
                  m = start[j1]%8;
                  if ((pos[1][i] == (pos[1][i1] + 1)) ^ (l < 2)) {
                     start[j1] = 16 + (m + 4)%8;
                     start[j] = (m + 4)%8;
                  } else {
                     start[j1] = 8 + m;
                     start[j] = m;
                  }
               } else {
                  l = start[j1]%8;
                  if ((pos[2][i] == (pos[2][i1] + 1)) ^ (l < 4)) {
                     start[j1] = l;
                     start[j] = (l/2)*2 + (l + 1)%2;
                  } else {
                     start[j1] = 16 + l;
                     start[j] = l;
                  }
               }
            i1 = i;
            j1 = j;
            j++;
         }
   }  // end of truncated Hilbert SFC initialization

   if (!stencil) {
      mat = num_vars/4;
      a1 = ((double) rand())/((double) RAND_MAX);
      for (i = 0; i < (num_vars/4); i++)
         a0[i] = ((double) rand())/((double) RAND_MAX);
   }
   num_active = max_active_block = local_max_b;
   global_active = num_active*num_pes;
   num_parents = max_active_parent = 0;
   size = p2[num_refine+1];  /* block size is p2[num_refine+1-level]
                              * smallest block is size p2[1], so can find
                              * its center */
   mesh_size[0] = init_x*size;
   max_mesh_size = mesh_size[0];
   mesh_size[1] = init_y*size;
   if (mesh_size[1] > max_mesh_size)
      max_mesh_size = mesh_size[1];
   mesh_size[2] = init_z*size;
   if (mesh_size[2] > max_mesh_size)
      max_mesh_size = mesh_size[2];
   if ((num_pes+1) > max_mesh_size)
      max_mesh_size = num_pes + 1;
   if (!lb_method) {
      bin  = (int *) ma_malloc(max_mesh_size*sizeof(int), __FILE__, __LINE__);
      gbin = (int *) ma_malloc(max_mesh_size*sizeof(int), __FILE__, __LINE__);
   } else {
      bin  = (int *) ma_malloc((global_active+1)*sizeof(int), __FILE__,
                               __LINE__);
      gbin = (int *) ma_malloc((global_active+1)*sizeof(int), __FILE__,
                               __LINE__);
   }
   if (stencil == 7)
      f = 0;
   else
      f = 1;
   for (o = n = k1 = k = 0; k < npz; k++)
      for (k2 = 0; k2 < init_block_z; k1++, k2++)
         for (j1 = j = 0; j < npy; j++)
            for (j2 = 0; j2 < init_block_y; j1++, j2++)
               for (i1 = i = 0; i < npx; i++)
                  for (i2 = 0; i2 < init_block_x; i1++, i2++, n++) {
                     if (lb_method >= 2) {
                        pes = pos1[i1][j1][k1];
                        m = pes/num_active;
                     } else
                        m = pos1[i][j][k];
                     if (m == my_pe) {
                        if (lb_method >= 2)
                           // for Hilbert need to reorder initial blocks
                           o = pes%num_active;
                        bp = &blocks[o];
                        bp->level = 0;
                        bp->number = n;
                        if (lb_method < 2) {
                           bp->num_prime = n*p8[num_refine];
                           bp->b_type = 0;
                        } else {
                           bp->num_prime = pes*p8[num_refine];
                           bp->b_type = start[pes];
                        }
                        bp->parent = -1;
                        bp->parent_node = my_pe;
                        bp->cen[0] = i1*size + size/2;
                        bp->cen[1] = j1*size + size/2;
                        bp->cen[2] = k1*size + size/2;
                        add_sorted_list(o, n, 0);
                        for (var = 0; var < num_vars; var++) {
                           // zero block and then initialize it
                           for (ib = 0; ib <= x_block_size+1; ib++)
                              for (jb = 0; jb <= y_block_size+1; jb++)
                                 for (kb = 0; kb <= z_block_size+1; kb++)
                                    bp->array[var][ib][jb][kb] = 0.0;
                           for (ib = 1; ib <= x_block_size; ib++)
                              for (jb = 1; jb <= y_block_size; jb++)
                                 for (kb = 1; kb <= z_block_size; kb++)
                                    bp->array[var][ib][jb][kb] =
                                       ((double) rand())/((double) RAND_MAX);
                        }
                        for (ib = 0; ib < 6; ib++)
                           bp->nei_refine[ib] = 0;
                        if (lb_method < 2) { // RCB or Morton SFC
                           if (i2 == 0)
                              if (i == 0) { /* 0 boundary */
                                 bp->nei_level[0] = -2;
                                 bp->nei[0][0][0] = 0;
                              } else {      /* boundary with neighbor core */
                                 bp->nei_level[0] = 0;
                                 bp->nei[0][0][0] = -1 - pos1[i-1][j][k];
                                 add_comm_list(0, o, pos1[i-1][j][k], 0+f,
                                            bp->cen[2]*mesh_size[1]+bp->cen[1],
                                            bp->cen[0] - size/2);
                              }
                           else {          /* neighbor on core */
                              bp->nei_level[0] = 0;
                              bp->nei[0][0][0] = o - 1;
                           }
                           if (i2 == (init_block_x - 1))
                              if (i == (npx - 1)) { /* 1 boundary */
                                 bp->nei_level[1] = -2;
                                 bp->nei[1][0][0] = 0;
                              } else {      /* boundary with neighbor core */
                                 bp->nei_level[1] = 0;
                                 bp->nei[1][0][0] = -1 - pos1[i+1][j][k];
                                 add_comm_list(0, o, pos1[i+1][j][k], 10+f,
                                            bp->cen[2]*mesh_size[1]+bp->cen[1],
                                            bp->cen[0] + size/2);
                              }
                           else {          /* neighbor on core */
                              bp->nei_level[1] = 0;
                              bp->nei[1][0][0] = o + 1;
                           }
                           if (j2 == 0)
                              if (j == 0) { /* 0 boundary */
                                 bp->nei_level[2] = -2;
                                 bp->nei[2][0][0] = 0;
                              } else {      /* boundary with neighbor core */
                                 bp->nei_level[2] = 0;
                                 bp->nei[2][0][0] = -1 - pos1[i][j-1][k];
                                 add_comm_list(1, o, pos1[i][j-1][k], 0+f,
                                            bp->cen[2]*mesh_size[0]+bp->cen[0],
                                            bp->cen[1] - size/2);
                              }
                           else {          /* neighbor on core */
                              bp->nei_level[2] = 0;
                              bp->nei[2][0][0] = o - init_block_x;
                           }
                           if (j2 == (init_block_y - 1))
                              if (j == (npy - 1)) { /* 1 boundary */
                                 bp->nei_level[3] = -2;
                                 bp->nei[3][0][0] = 0;
                              } else {      /* boundary with neighbor core */
                                 bp->nei_level[3] = 0;
                                 bp->nei[3][0][0] = -1 - pos1[i][j+1][k];
                                 add_comm_list(1, o, pos1[i][j+1][k], 10+f,
                                               bp->cen[2]*mesh_size[0]+bp->cen[0],
                                               bp->cen[1] + size/2);
                              }
                           else {          /* neighbor on core */
                              bp->nei_level[3] = 0;
                              bp->nei[3][0][0] = o + init_block_x;
                           }
                           if (k2 == 0)
                              if (k == 0) { /* 0 boundary */
                                 bp->nei_level[4] = -2;
                                 bp->nei[4][0][0] = 0;
                              } else {      /* boundary with neighbor core */
                                 bp->nei_level[4] = 0;
                                 bp->nei[4][0][0] = -1 - pos1[i][j][k-1];
                                 add_comm_list(2, o, pos1[i][j][k-1], 0+f,
                                            bp->cen[1]*mesh_size[0]+bp->cen[0],
                                            bp->cen[2] - size/2);
                              }
                           else {          /* neighbor on core */
                              bp->nei_level[4] = 0;
                              bp->nei[4][0][0] = o - init_block_x*init_block_y;
                           }
                           if (k2 == (init_block_z - 1))
                              if (k == (npz - 1)) { /* 1 boundary */
                                 bp->nei_level[5] = -2;
                                 bp->nei[5][0][0] = 0;
                              } else {      /* boundary with neighbor core */
                                 bp->nei_level[5] = 0;
                                 bp->nei[5][0][0] = -1 - pos1[i][j][k+1];
                                 add_comm_list(2, o, pos1[i][j][k+1], 10+f,
                                            bp->cen[1]*mesh_size[0]+bp->cen[0],
                                            bp->cen[2] + size/2);
                              }
                           else {          /* neighbor on core */
                              bp->nei_level[5] = 0;
                              bp->nei[5][0][0] = o + init_block_x*init_block_y;
                           } // end of RCB and Morton SFC boundary init
                        } else { // Hilbert SFC
                           if (i1 == 0) { // 0 boundary
                              bp->nei_level[0] = -2;
                              bp->nei[0][0][0] = 0;
                           } else if ((pes = pos1[i1-1][j1][k1]/num_active)
                                      == m) { // neighbor on core
                              bp->nei_level[0] = 0;
                              bp->nei[0][0][0] = pos1[i1-1][j1][k1]%num_active;
                           } else { // boundary with neighbor core
                              bp->nei_level[0] = 0;
                              bp->nei[0][0][0] = -1 - pes;
                              add_comm_list(0, o, pes, 0+f,
                                            bp->cen[2]*mesh_size[1]+bp->cen[1],
                                            bp->cen[0] - size/2);
                           }
                           if (i1 == (init_x-1)) { // 1 boundary
                              bp->nei_level[1] = -2;
                              bp->nei[1][0][0] = 0;
                           } else if ((pes = pos1[i1+1][j1][k1]/num_active)
                                      == m) { // neighbor on core
                              bp->nei_level[1] = 0;
                              bp->nei[1][0][0] = pos1[i1+1][j1][k1]%num_active;
                           } else { // boundary with neighbor core
                              bp->nei_level[1] = 0;
                              bp->nei[1][0][0] = -1 - pes;
                              add_comm_list(0, o, pes, 10+f,
                                            bp->cen[2]*mesh_size[1]+bp->cen[1],
                                            bp->cen[0] + size/2);
                           }
                           if (j1 == 0) { // 0 boundary
                              bp->nei_level[2] = -2;
                              bp->nei[2][0][0] = 0;
                           } else if ((pes = pos1[i1][j1-1][k1]/num_active)
                                      == m) { // neighbor on core
                              bp->nei_level[2] = 0;
                              bp->nei[2][0][0] = pos1[i1][j1-1][k1]%num_active;
                           } else { // boundary with neighbor core
                              bp->nei_level[2] = 0;
                              bp->nei[2][0][0] = -1 - pes;
                              add_comm_list(1, o, pes, 0+f,
                                            bp->cen[2]*mesh_size[0]+bp->cen[0],
                                            bp->cen[1] - size/2);
                           }
                           if (j1 == (init_y-1)) { // 1 boundary
                              bp->nei_level[3] = -2;
                              bp->nei[3][0][0] = 0;
                           } else if ((pes = pos1[i1][j1+1][k1]/num_active)
                                      == m) { // neighbor on core
                              bp->nei_level[3] = 0;
                              bp->nei[3][0][0] = pos1[i1][j1+1][k1]%num_active;
                           } else { // boundary with neighbor core
                              bp->nei_level[3] = 0;
                              bp->nei[3][0][0] = -1 - pes;
                              add_comm_list(1, o, pes, 10+f,
                                            bp->cen[2]*mesh_size[0]+bp->cen[0],
                                            bp->cen[1] + size/2);
                           }
                           if (k1 == 0) { // 0 boundary
                              bp->nei_level[4] = -2;
                              bp->nei[4][0][0] = 0;
                           } else if ((pes = pos1[i1][j1][k1-1]/num_active)
                                      == m) { // neighbor on core
                              bp->nei_level[4] = 0;
                              bp->nei[4][0][0] = pos1[i1][j1][k1-1]%num_active;
                           } else { // boundary with neighbor core
                              bp->nei_level[4] = 0;
                              bp->nei[4][0][0] = -1 - pes;
                              add_comm_list(2, o, pes, 0+f,
                                            bp->cen[1]*mesh_size[0]+bp->cen[0],
                                            bp->cen[2] - size/2);
                           }
                           if (k1 == (init_z-1)) { // 1 boundary
                              bp->nei_level[5] = -2;
                              bp->nei[5][0][0] = 0;
                           } else if ((pes = pos1[i1][j1][k1+1]/num_active)
                                      == m) { // neighbor on core
                              bp->nei_level[5] = 0;
                              bp->nei[5][0][0] = pos1[i1][j1][k1+1]%num_active;
                           } else { // boundary with neighbor core
                              bp->nei_level[5] = 0;
                              bp->nei[5][0][0] = -1 - pes;
                              add_comm_list(2, o, pes, 10+f,
                                            bp->cen[1]*mesh_size[0]+bp->cen[0],
                                            bp->cen[2] + size/2);
                           }
                        } // end if Hilbert SFC boundary conditions
                        o++;
                     }
                  }

   check_buff_size();

   for (var = 0; var < num_vars; var++)
      grid_sum[var] = check_sum(var);
}
