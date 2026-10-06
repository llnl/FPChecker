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
#include <mpi.h>

#include "block.h"
#include "comm.h"
#include "timer.h"
#include "proto.h"

// Space filling curves for load balancing - Morton scheme.
void sfc(void)
{
   int in, n, i, j, m, nfac, fac[25], n_m_tmp, n_m_tot;;
   double t1, t2, t3, t4, t5, tp, tm, tu;
   block *bp;

   tp = tm = tu = 0.0;

   t3 = t4 = t5 = 0.0;
   t1 = timer();
   for (in = 0, num_dots = 0; in < sorted_index[num_refine+1]; in++) {
      bp = &blocks[n = sorted_list[in].n];
      bp->new_proc = my_pe;
      if ((num_dots+1) > max_num_dots) {
         printf("%d ERROR: need more spots\n", my_pe);
         exit(-1);
      }
      spots[num_dots].number = bp->number;
      spots[num_dots].num_prime = bp->num_prime;
      spots[num_dots].n = n;
      spots[num_dots].proc = my_pe;
      spots[num_dots++].new_proc = 0;
   }
   max_active_dot = num_dots;
   for (n = num_dots; n < max_num_dots; n++)
      spots[n].number = -1;

   // Use space filling curve (implicitly) using a RCB-like framework
   nfac = factor(num_pes, fac);
   for (i = nfac-1, j = 0; i >= 0; i--, j++) {
      sfc_sort(j, fac[i]);
      move_spots(j, fac[i]);
   }

   // first have to move information from dots back to original core,
   // then will update processor block is moving to, and then its neighbors
   for (n = 0; n < num_pes; n++)
      to[n] = 0;
   for (n_m_tmp = i = 0; i < max_active_dot; i++)
      if (spots[i].number >= 0 && spots[i].proc != my_pe) {
         to[spots[i].proc]++;
         n_m_tmp++;
      }

   MPI_Allreduce(&n_m_tmp, &n_m_tot, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   t4 = timer();
   t2 = t4 - t1;
   if (n_m_tot) {  // Only move dots and blocks if there is something to move
      MPI_Alltoall(to, 1, MPI_INT, from, 1, MPI_INT, MPI_COMM_WORLD);

      move_spots_back();

      if (limit_move && !first) {
         m = (limit_move*num_active)/100;
         /* change to move more refined blocks first */
         /* for (in = 0; in < sorted_index[num_refine+1]; in++) */
         for (in = sorted_index[num_refine+1]-1; in >= 0; in--) {
            n = sorted_list[in].n;
            if (blocks[n].new_proc != my_pe) {
               m--;
               if (m < 0) {
                  from[blocks[n].new_proc]--;
                  blocks[n].new_proc = my_pe;
                  n_m_tmp--;
               }
            }
         }
         MPI_Alltoall(from, 1, MPI_INT, to, 1, MPI_INT, MPI_COMM_WORLD);
         MPI_Allreduce(&n_m_tmp, &n, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
         if (!my_pe && report_perf & 8)
            printf("Move %d blocks out of %d possible to load balance\n",
                   n, n_m_tot);
      }
      num_moved_lb += n_m_tmp;

      t5 = timer();
      t3 = t5 - t4;
      t4 = t5;

      move_blocks(&tp, &tm, &tu);
   }

   total_dots_used += max_active_dot;
   if (max_active_dot > max_dots_used)
      max_dots_used = max_active_dot;
   t5 = timer() - t4;
   timer_lb_misc += timer() - t1 - t2 - t3 - tp - tm - tu;
   timer_lb_sort += t2;
   timer_lb_pa += tp;
   timer_lb_mv += tm;
   timer_lb_un += tu;
   timer_lb_mb += t3;
   timer_lb_ma += t5;
}

// Sort by binning the spots according to the original parent block.  Then
// continue to bin by each level of block until a divisor can be found.
// Communicate the dots at the end of each stage.  At the end, use the dots
// to determine where to send the blocks to.
void sfc_sort(int div, int fact)
{
   int i, j, sum, total_dots, part, max_bin, done, level,
       bin1[fact], extra[fact];
   num_sz base, divider[fact];

   MPI_Allreduce(&num_dots, &total_dots, 1, MPI_INT, MPI_SUM, comms[div]);

   max_bin = num_pes*init_block_x*init_block_y*init_block_z;
   for (i = 0; i < max_bin; i++)
      bin[i] = 0;

   for (i = 0; i < max_active_dot; i++)
      if (spots[i].number >= 0)
         // map block back to original block (according to number)
         bin[spots[i].num_prime/p8[num_refine]]++;

   MPI_Allreduce(bin, gbin, max_bin, MPI_INT, MPI_SUM, comms[div]);

   part = (total_dots+fact-1)/fact;
   for (sum = j = i = 0; i < max_bin && j < (fact-1); i++) {
      // find the bins that have the dividers
      sum += gbin[i];
      while (sum >= (j+1)*part && j < (fact-1)) {
         bin1[j] = gbin[i];
         extra[j] = sum - (j+1)*part;
         divider[j++] = i;
      }
   }

   for (j = 0; j < (fact-1); j++)
      // if extra is 0 we have a divider (the beginning of the next bin)
      if (!extra[j])
         divider[j] = p8[num_refine]*(divider[j]+1);
      else {
         for (i = 0; i < max_active_dot; i++)
            if (spots[i].number >= 0)
               if (spots[i].num_prime/p8[num_refine] == divider[j])
                  spots[i].new_proc = -1;
         base = p8[num_refine]*divider[j];
         // keep going down the tree until we can identify a divider
         for (done = 0, level = 1; level <= num_refine && !done; level++) {
            for (i = 0; i < 8; i++)
               bin[i] = 0;
            for (i = 0; i < max_active_dot; i++)
               if (spots[i].number >= 0 && spots[i].new_proc == -1)
                  bin[(spots[i].num_prime - base)/p8[num_refine-level]]++;

            MPI_Allreduce(bin, gbin, 8, MPI_INT, MPI_SUM, comms[div]);

            part = bin1[j] - extra[j];
            for (sum = i = 0; i < 8; i++) {
               sum += gbin[i];
               if (sum >= part) {
                  extra[j] = sum - part;
                  divider[j] = i;
                  bin1[j] = gbin[i];
                  break;
               }
            }
            if (!extra[j]) {
               divider[j] = base + (divider[j]+1)*p8[num_refine-level];
               done = 1;
            } else if (bin1[j] == 8 && group_blocks) {
               // don't split terminal octants
               if (extra[j] > 4)
                  divider[j] = base + (divider[j]+1)*p8[num_refine-level];
               else
                  divider[j] = base + divider[j]*p8[num_refine-level];
               done = 1;
            } else if (bin1[j] == 8 && level == num_refine) {
               divider[j] = base + divider[j]*p8[num_refine-level] +
                            (8 - extra[j])*p8[num_refine-level-1];
               done = 1;
            } else {
               for (i = 0; i < max_active_dot; i++)
                  if (spots[i].number >= 0 && spots[i].new_proc == -1) {
                     if ((spots[i].num_prime - base)/p8[num_refine-level] !=
                         divider[j])
                        spots[i].new_proc = 0;
                  }
               base += divider[j]*p8[num_refine-level];
            }
            if (done)
               for (i = 0; i < max_active_dot; i++)
                  if (spots[i].number >= 0 && spots[i].new_proc == -1)
                     spots[i].new_proc = 0;
         }
      }

   // assign new division to spots
   for (i = 0; i < max_active_dot; i++)
      if (spots[i].number >= 0) {
         for (j = 0; j < (fact-1); j++)
            if (spots[i].num_prime < divider[j]) {
               spots[i].new_proc = j;
               break;
            }
         if (j == (fact-1))
            spots[i].new_proc = j;
      }
}

void move_spots(int div, int fact)
{
   int i, j, d, sg, mg, partner, type, off[fact+1], which, err, nr;
   int *send_int = (int *) send_buff;
   int *recv_int = (int *) recv_buff;
   long long *send_ll, *recv_ll;
   MPI_Status status;

   sg = np[div]/fact;
   mg = me[div]/sg;

   for (i = 0; i < fact; i++)
      bin[i] = 0;

   // determine which proc to send dots to
   for (d = 0; d < max_active_dot; d++)
      if (spots[d].number >= 0)
         bin[spots[d].new_proc]++;

   type = 30;
   for (i = 0; i < fact; i++)
      if (i != mg) {
         partner = me[div]%sg + i*sg;
         MPI_Irecv(&gbin[i], 1, MPI_INT, partner, type, comms[div],
                   &request[i]);
      }

   for (i = 0; i < fact; i++)
      if (i != mg) {
         partner = me[div]%sg + i*sg;
         MPI_Send(&bin[i], 1, MPI_INT, partner, type, comms[div]);
      }

   type = 31;
   off[0] = 0;
   for (nr = i = 0; i < fact; i++)
      if (i != mg) {
         err = MPI_Wait(&request[i], &status);
         if (gbin[i] > 0) {
            partner = me[div]%sg + i*sg;
            MPI_Irecv(&recv_int[off[i]], 6*gbin[i], MPI_INT, partner,
                      type, comms[div], &request[i]);
            off[i+1] = off[i] + 6*gbin[i];
            nr++;
         } else {
            off[i+1] = off[i];
            request[i] = MPI_REQUEST_NULL;
         }
      } else {
         off[i+1] = off[i];
         request[i] = MPI_REQUEST_NULL;
      }

   for (i = 0; i < fact; i++)
      if (i != mg && bin[i] > 0) {
         for (j = d = 0; d < max_active_dot; d++)
            if (spots[d].number >= 0 && spots[d].new_proc == i) {
               send_ll = (long long *) &send_int[j];
               j += 4;
               send_ll[0] = (long long) spots[d].number;
               send_ll[1] = (long long) spots[d].num_prime;
               send_int[j++] = spots[d].n;
               send_int[j++] = spots[d].proc;
               spots[d].number = -1;
               num_dots--;
            }

         partner = me[div]%sg + i*sg;
         MPI_Send(send_int, 6*bin[i], MPI_INT, partner, type, comms[div]);
      }

   for (d = i = 0; i < nr; i++) {
      err = MPI_Waitany(fact, request, &which, &status);
      for (j = off[which]; j < off[which+1]; ) {
         for ( ; d < max_num_dots; d++)
            if (spots[d].number < 0)
               break;
         if (d == max_num_dots) {
            printf("%d ERROR: need more dots in move_dots %d %d\n",
                   my_pe, max_num_dots, num_dots);
            exit(-1);
         }
         recv_ll = (long long *) &recv_int[j];
         j += 4;
         spots[d].number = (num_sz) recv_ll[0];
         spots[d].num_prime = (num_sz) recv_ll[1];
         spots[d].n = recv_int[j++];
         spots[d].proc = recv_int[j++];
         num_dots++;
         if ((d+1) > max_active_dot)
            max_active_dot = d+1;
      }
   }
}

void move_spots_back(void)
{
   int i, j, d, nr, err, which;
   int *send_int = (int *) send_buff;
   int *recv_int = (int *) recv_buff;
   MPI_Status status;

   gbin[0] = 0;
   for (nr = i = 0; i < num_pes; i++)
      if (from[i] > 0) {
         gbin[i+1] = gbin[i] + 2*from[i];
         MPI_Irecv(&recv_int[gbin[i]], 2*from[i], MPI_INT, i, 50,
                   MPI_COMM_WORLD, &request[i]);
         nr++;
      } else {
         gbin[i+1] = gbin[i];
         request[i] = MPI_REQUEST_NULL;
      }

   for (i = 0; i < num_pes; i++)
      if (to[i] > 0) {
         for (j = d = 0; d < max_active_dot; d++)
            if (spots[d].number >= 0 && spots[d].proc == i) {
               send_int[j++] = spots[d].n;
               send_int[j++] = my_pe;
            }
         MPI_Send(send_int, 2*to[i], MPI_INT, i, 50, MPI_COMM_WORLD);
      }

   for (i = 0; i < nr; i++) {
      err = MPI_Waitany(num_pes, request, &which, &status);
      for (j = 0; j < from[which]; j++)
         blocks[recv_int[gbin[which]+2*j]].new_proc =
               recv_int[gbin[which]+2*j+1];
   }
}
