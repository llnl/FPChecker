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

// The routines in this file are used in the communication of ghost values
// between blocks, both on processor and off processor.

// Main communication routine that sends and recieves ghost values between
// blocks on different processors and directos blocks on the same processor
// to exchange their ghost values.
void comm(int start, int num_comm, int stage)
{
   int i, j, k, l, m, n, dir, o, which, offset, type, size, num_mesg;
   int permutations[6][3] = { {0, 1, 2}, {1, 2, 0}, {2, 0, 1},
                              {0, 2, 1}, {1, 0, 2}, {2, 1, 0} };
   double t1, t2, t3, t4;
   MPI_Status status;
   // Private variables for omp
   int c1, c2, c3;
   double time1, time2, time3;

   for (o = 0; o < 3; o++) {
      if (permute)
         dir = permutations[stage%6][o];
      else
         dir = o;
      type = dir;
      t1 = timer();

#ifndef FPC_MINIAMR_DISABLE_INJECTION
      if (stage == 0 && start == 0 && o == 0)
      {
         volatile double base_val = 678912.345;
         double inj_x = base_val;
         double inj_y = base_val + 1.0e-11;
         volatile double cancelled_result __attribute__((unused)) = inj_y - inj_x; // Injection standalone

         double state_x = ((double) num_comm + (double) comm_vars + (double) dir + (double) num_comm_partners[dir] + 1.0) * 678912.345;
         double state_y = state_x + state_x * 1.0e-15;
         volatile double state_cancelled_result __attribute__((unused)) = state_y - state_x; // Injection state
      }
#endif

      for (num_mesg = i = 0; i < num_comm_partners[dir]; i++)
         if (send_faces)
            for (n = 0; n < comm_num[dir][i]; n++, num_mesg++) {
               if ((j = comm_face_case[dir][comm_index[dir][i]+n]) >= 10)
                  j -= 10;
               if (j == 0)
                  size = comm_vars*msg_len[dir][0];
               else if (j == 1)
                  size = comm_vars*msg_len[dir][1];
               else if (j < 6)
                  size = comm_vars*msg_len[dir][3];
               else
                  size = comm_vars*msg_len[dir][2];
               MPI_Irecv(&recv_buff[comm_recv_off[dir][comm_index[dir][i]+n]],
                         size, MPI_DOUBLE, comm_partner[dir][i], n,
                         MPI_COMM_WORLD, &request[num_mesg]);
               counter_halo_recv[dir]++;
               size_mesg_recv[dir] += (double) size*sizeof(double);
            }
         else {
            MPI_Irecv(&recv_buff[comm_recv_off[dir][comm_index[dir][i]]],
                      recv_size[dir][i], MPI_DOUBLE,
                      comm_partner[dir][i], type, MPI_COMM_WORLD, &request[i]);
            counter_halo_recv[dir]++;
            size_mesg_recv[dir] += (double) recv_size[dir][i]*sizeof(double);
            num_mesg++;
         }
      timer_comm_recv[dir] += timer() - t1;

/**** the send and recv list can be same if kept ordered (length can be diff)
        would need to expand face case so that:     **** done ****
        0   whole -> whole
        1   whole -> whole   w/ 27 point stencil (send corners)
        2-5 whole -> quarter w/ number indicating which quarter (matters recv)
        6-9 quarter -> whole w/ number indicating which quarter (matters send)
        + 10 for E, N, U
**** one large send buffer -- can pack and send for a neighbor and reuse */
      if (send_faces)
         for (k = i = 0; i < num_comm_partners[dir]; i++)
            for (n = 0; n < comm_num[dir][i]; n++, k++) {
               t2 = timer();
               if (nonblocking)
                  offset = comm_send_off[dir][comm_index[dir][i]+n];
               else
                  offset = 0;
               pack_face(&send_buff[offset],
                         comm_block[dir][comm_index[dir][i]+n],
                         comm_face_case[dir][comm_index[dir][i]+n], dir,
                         start, num_comm);
               counter_face_send[dir]++;
               t3 = timer();
               if ((j = comm_face_case[dir][comm_index[dir][i]+n]) >= 10)
                  j -= 10;
               if (j == 0)
                  size = comm_vars*msg_len[dir][0];
               else if (j == 1)
                  size = comm_vars*msg_len[dir][1];
               else if (j < 6)
                  size = comm_vars*msg_len[dir][2];
               else
                  size = comm_vars*msg_len[dir][3];
               if (nonblocking)
                  MPI_Isend(&send_buff[offset], size, MPI_DOUBLE,
                            comm_partner[dir][i], n, MPI_COMM_WORLD,&s_req[k]);
               else
                  MPI_Send(send_buff, size, MPI_DOUBLE, comm_partner[dir][i],n,
                           MPI_COMM_WORLD);
               counter_halo_send[dir]++;
               size_mesg_send[dir] += (double) size*sizeof(double);
               t4 = timer();
               timer_comm_pack[dir] += t3 - t2;
               timer_comm_send[dir] += t4 - t3;
            }
      else
         for (i = 0; i < num_comm_partners[dir]; i++) {
            t2 = timer();
            for (n = 0; n < comm_num[dir][i]; n++) {
               offset = comm_send_off[dir][comm_index[dir][i]+n];
               if (!nonblocking)
                  offset -= comm_send_off[dir][comm_index[dir][i]];
               pack_face(&send_buff[offset],
                         comm_block[dir][comm_index[dir][i]+n],
                         comm_face_case[dir][comm_index[dir][i]+n], dir,
                         start, num_comm);
            }
            counter_face_send[dir] += comm_num[dir][i];
            t3 = timer();
            if (nonblocking)
               MPI_Isend(&send_buff[comm_send_off[dir][comm_index[dir][i]]],
                         send_size[dir][i], MPI_DOUBLE, comm_partner[dir][i],
                         type, MPI_COMM_WORLD, &s_req[i]);
            else
               MPI_Send(send_buff, send_size[dir][i], MPI_DOUBLE,
                        comm_partner[dir][i], type, MPI_COMM_WORLD);
            counter_halo_send[dir]++;
            size_mesg_send[dir] += (double) send_size[dir][i]*sizeof(double);
            t4 = timer();
            timer_comm_pack[dir] += t3 - t2;
            timer_comm_send[dir] += t4 - t3;
         }

      // While values are being sent over the mesh, go through and direct
      // blocks to exchange ghost values with other blocks that are on
      // processor.  Also apply boundary conditions for boundary of domain.
      c1 = c2 = c3 = 0;
      time1 = time2 = time3 = 0.0;
      #pragma omp parallel for private(n, l, m, i, j, k, t2, time1, time2, time3) reduction(+: c1, c2, c3)
      for(int in = 0; in < sorted_index[num_refine+1]; in++) {
         block* bp = &blocks[n = sorted_list[in].n];
         for (l = dir*2; l < (dir*2 + 2); l++) {
            if (bp->nei_level[l] == bp->level) {
               t2 = timer();
               if ((m = bp->nei[l][0][0]) > n) {
                  on_proc_comm(n, m, l, start, num_comm);
                  c1 += 2;
               }
               time1 += timer() - t2;
            } else if (bp->nei_level[l] == (bp->level+1)) {
               t2 = timer();
               for (i = 0; i < 2; i++)
                  for (j = 0; j < 2; j++)
                     if ((m = bp->nei[l][i][j]) > n) {
                        on_proc_comm_diff(n, m, l, i, j, start, num_comm);
                        c2 += 2;
                     }
               time2 += timer() - t2;
            } else if (bp->nei_level[l] == (bp->level-1)) {
               t2 = timer();
               if ((m = bp->nei[l][0][0]) > n) {
                  k = dir*2 + 1 - l%2;
                  for (i = 0; i < 2; i++)
                     for (j = 0; j < 2; j++)
                        if (blocks[m].nei[k][i][j] == n) {
                           on_proc_comm_diff(m, n, k, i, j, start, num_comm);
                           c2 += 2;
                        }
               }
               time2 += timer() - t2;
            } else if (bp->nei_level[l] == -2) {
               t2 = timer();
               apply_bc(l, bp, start, num_comm);
               c3++;
               time3 += timer() - t2;
            } else {
               printf("ERROR: misconnected block\n");
               exit(-1);
            }
         }
      }
      // Write local temp values into correct ones
      counter_same[dir] = c1;
      counter_diff[dir] = c2;
      counter_bc[dir] = c3;
      timer_comm_same[dir] = time1;
      timer_comm_diff[dir] = time2;
      timer_comm_bc[dir] = time3;

      for (i = 0; i < num_mesg; i++) {
         t2 = timer();
         MPI_Waitany(num_mesg, request, &which, &status);
         t3 = timer();
         if (send_faces) {
            for (k = j = 0; j < num_comm_partners[dir]; k += comm_num[dir][j++])
               if (which <= (k + comm_num[dir][j])) {
                 l = which - k;
                 unpack_face(&recv_buff[comm_recv_off[dir][comm_index[dir][j]+l]],
                             comm_block[dir][comm_index[dir][j]+l],
                             comm_face_case[dir][comm_index[dir][j]+l],
                             dir, start, num_comm);
                 break;
               }
            counter_face_recv[dir]++;
         } else {
            for (n = 0; n < comm_num[dir][which]; n++)
               unpack_face(&recv_buff[comm_recv_off[dir][comm_index[dir][which]+n]],
                           comm_block[dir][comm_index[dir][which]+n],
                           comm_face_case[dir][comm_index[dir][which]+n],
                           dir, start, num_comm);
            counter_face_recv[dir] += comm_num[dir][which];
         }
         t4 = timer();
         timer_comm_wait[dir] += t3 - t2;
         timer_comm_unpack[dir] += t4 - t3;
      }

      if (nonblocking) {
         t2 = timer();
         for (i = 0; i < num_mesg; i++)
            MPI_Waitany(num_mesg, s_req, &which, &status);
         t3 = timer();
         timer_comm_wait[dir] += t3 - t2;
      }
      timer_comm_dir[dir] += timer() - t1;
   }
}

// Pack face to send - note different cases for different directions.
void pack_face(double *send_buf, int block_num, int face_case, int dir,
               int start, int num_comm)
{
   int i, j, k, n, m;
   int is, ie, js, je, ks, ke;
   block *bp;

   bp = &blocks[block_num];

   if (!code) {

      if (dir == 0) {        /* X - East, West */

         /* X directions (East and West) sent first, so just send
            the real values and no ghosts
         */
         if (face_case >= 10) { /* +X - East */
            i = x_block_size;
            face_case = face_case - 10;
         } else                 /* -X - West */
            i = 1;
         if (face_case < 2) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = 1; j <= y_block_size; j++)
                  for (k = 1; k <= z_block_size; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face - case does not matter */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = 1; j < y_block_size; j += 2)
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j  ][k  ] +
                                   bp->array[m][i][j  ][k+1] +
                                   bp->array[m][i][j+1][k  ] +
                                   bp->array[m][i][j+1][k+1];
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to send */
            if (face_case%2 == 0) {
               js = 1;
               je = y_block_half;
            } else {
               js = y_block_half + 1;
               je = y_block_size;
            }
            if ((face_case/2)%2 == 1) {
               ks = 1;
               ke = z_block_half;
            } else {
               ks = z_block_half + 1;
               ke = z_block_size;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = js; j <= je; j++)
                  for (k = ks; k <= ke; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k]/4.0;
         }

      } else if (dir == 1) { /* Y - North, South */

         /* Y directions (North and South) sent second, so send the real values
         */
         if (face_case >= 10) { /* +Y - North */
            j = y_block_size;
            face_case = face_case - 10;
         } else                 /* -Y - South */
            j = 1;
         if (face_case == 0) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 1; i <= x_block_size; i++)
                  for (k = 1; k <= z_block_size; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case == 1) {
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (k = 1; k <= z_block_size; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face - case does not matter */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 1; i < x_block_size; i += 2)
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i  ][j][k  ] +
                                   bp->array[m][i  ][j][k+1] +
                                   bp->array[m][i+1][j][k  ] +
                                   bp->array[m][i+1][j][k+1];
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to send */
            if (face_case%2 == 0) {
               is = 1;
               ie = x_block_half;
            } else {
               is = x_block_half + 1;
               ie = x_block_size;
            }
            if ((face_case/2)%2 == 1) {
               ks = 1;
               ke = z_block_half;
            } else {
               ks = z_block_half + 1;
               ke = z_block_size;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = is; i <= ie; i++)
                  for (k = ks; k <= ke; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k]/4.0;
         }

      } else {               /* Z - Up, Down */

         /* Z directions (Up and Down) sent last
         */
         if (face_case >= 10) { /* +Z - Up */
            k = z_block_size;
            face_case = face_case - 10;
         } else                 /* -Z - Down */
            k = 1;
         if (face_case == 0) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 1; i <= x_block_size; i++)
                  for (j = 1; j <= y_block_size; j++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case == 1) {
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (j = 0; j <= y_block_size+1; j++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face - case does not matter */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 1; i < x_block_size; i += 2)
                  for (j = 1; j < y_block_size; j += 2, n++)
                     send_buf[n] = bp->array[m][i  ][j  ][k] +
                                   bp->array[m][i  ][j+1][k] +
                                   bp->array[m][i+1][j  ][k] +
                                   bp->array[m][i+1][j+1][k];
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to send */
            if (face_case%2 == 0) {
               is = 1;
               ie = x_block_half;
            } else {
               is = x_block_half + 1;
               ie = x_block_size;
            }
            if ((face_case/2)%2 == 1) {
               js = 1;
               je = y_block_half;
            } else {
               js = y_block_half + 1;
               je = y_block_size;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = is; i <= ie; i++)
                  for (j = js; j <= je; j++, n++)
                     send_buf[n] = bp->array[m][i][j][k]/4.0;
         }
      }

   } else if (code == 1) { /* send all ghosts */

      if (dir == 0) {        /* X - East, West */

         if (face_case >= 10) { /* +X - East */
            i = x_block_size;
            face_case = face_case - 10;
         } else                 /* -X - West */
            i = 1;
         if (face_case < 2) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = 0; j <= y_block_size+1; j++)
                  for (k = 0; k <= z_block_size+1; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face */
            for (n = 0, m = start; m < start+num_comm; m++) {
               if (face_case%2 == 0) {
                  j = 0;
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j][k  ] +
                                   bp->array[m][i][j][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
               for (j = 1; j < y_block_size; j += 2) {
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i][j  ][k] +
                                     bp->array[m][i][j+1][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j  ][k  ] +
                                   bp->array[m][i][j  ][k+1] +
                                   bp->array[m][i][j+1][k  ] +
                                   bp->array[m][i][j+1][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j  ][k] +
                                     bp->array[m][i][j+1][k];
                  }
               }
               if (face_case%2 == 1) {
                  j = y_block_size + 1;
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j][k  ] +
                                   bp->array[m][i][j][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
            }
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to send */
            if (face_case%2 == 0) {
               js = 0;
               je = y_block_half + 1;
            } else {
               js = y_block_half;
               je = y_block_size + 1;
            }
            if ((face_case/2)%2 == 1) {
               ks = 0;
               ke = z_block_half + 1;
            } else {
               ks = z_block_half;
               ke = z_block_size + 1;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = js; j <= je; j++)
                  for (k = ks; k <= ke; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k]/4.0;
         }

      } else if (dir == 1) { /* Y - North, South */

         if (face_case >= 10) { /* +Y - North */
            j = y_block_size;
            face_case = face_case - 10;
         } else                 /* -Y - South */
            j = 1;
         if (face_case < 2) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (k = 0; k <= z_block_size+1; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face */
            for (n = 0, m = start; m < start+num_comm; m++) {
               if (face_case%2 == 0) {
                  i = 0;
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j][k  ] +
                                   bp->array[m][i][j][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
               for (i = 1; i < x_block_size; i += 2) {
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i  ][j][k] +
                                     bp->array[m][i+1][j][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i  ][j][k  ] +
                                   bp->array[m][i  ][j][k+1] +
                                   bp->array[m][i+1][j][k  ] +
                                   bp->array[m][i+1][j][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i  ][j][k] +
                                     bp->array[m][i+1][j][k];
                  }
               }
               if (face_case%2 == 1) {
                  i = x_block_size + 1;
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j][k  ] +
                                   bp->array[m][i][j][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
            }
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to send */
            if (face_case%2 == 0) {
               is = 0;
               ie = x_block_half + 1;
            } else {
               is = x_block_half;
               ie = x_block_size + 1;
            }
            if ((face_case/2)%2 == 1) {
               ks = 0;
               ke = z_block_half + 1;
            } else {
               ks = z_block_half;
               ke = z_block_size + 1;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = is; i <= ie; i++)
                  for (k = ks; k <= ke; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k]/4.0;
         }

      } else {               /* Z - Up, Down */

         /* Z directions (Up and Down) sent last
         */
         if (face_case >= 10) { /* +Z - Up */
            k = z_block_size;
            face_case = face_case - 10;
         } else                 /* -Z - Down */
            k = 1;
         if (face_case < 2) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (j = 0; j <= y_block_size+1; j++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face - case does not matter */
            for (n = 0, m = start; m < start+num_comm; m++) {
               if (face_case%2 == 0) {
                  i = 0;
                  if ((face_case/2)%2 == 1) {
                     j = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (j = 1; j < y_block_size; j += 2, n++)
                     send_buf[n] = bp->array[m][i][j  ][k] +
                                   bp->array[m][i][j+1][k];
                  if ((face_case/2)%2 == 0) {
                     j = y_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
               for (i = 1; i < x_block_size; i += 2) {
                  if ((face_case/2)%2 == 1) {
                     j = 0;
                     send_buf[n++] = bp->array[m][i  ][j][k] +
                                     bp->array[m][i+1][j][k];
                  }
                  for (j = 1; j < y_block_size; j += 2, n++)
                     send_buf[n] = bp->array[m][i  ][j  ][k] +
                                   bp->array[m][i  ][j+1][k] +
                                   bp->array[m][i+1][j  ][k] +
                                   bp->array[m][i+1][j+1][k];
                  if ((face_case/2)%2 == 0) {
                     j = y_block_size + 1;
                     send_buf[n++] = bp->array[m][i  ][j][k] +
                                     bp->array[m][i+1][j][k];
                  }
               }
               if (face_case%2 == 1) {
                  i = x_block_size + 1;
                  if ((face_case/2)%2 == 1) {
                     j = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (j = 1; j < y_block_size; j += 2, n++)
                     send_buf[n] = bp->array[m][i][j  ][k] +
                                   bp->array[m][i][j+1][k];
                  if ((face_case/2)%2 == 0) {
                     j = y_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
            }
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to send */
            if (face_case%2 == 0) {
               is = 0;
               ie = x_block_half + 1;
            } else {
               is = x_block_half;
               ie = x_block_size + 1;
            }
            if ((face_case/2)%2 == 1) {
               js = 0;
               je = y_block_half + 1;
            } else {
               js = y_block_half;
               je = y_block_size + 1;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = is; i <= ie; i++)
                  for (j = js; j <= je; j++, n++)
                     send_buf[n] = bp->array[m][i][j][k]/4.0;
         }
      }

   } else { /* code == 2 send all ghosts and do all processing on send side */

      if (dir == 0) {        /* X - East, West */

         if (face_case >= 10) { /* +X - East */
            i = x_block_size;
            face_case = face_case - 10;
         } else                 /* -X - West */
            i = 1;
         if (face_case < 2) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = 0; j <= y_block_size+1; j++)
                  for (k = 0; k <= z_block_size+1; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face */
            for (n = 0, m = start; m < start+num_comm; m++) {
               if (face_case%2 == 0) {
                  j = 0;
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j][k  ] +
                                   bp->array[m][i][j][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
               for (j = 1; j < y_block_size; j += 2) {
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i][j  ][k] +
                                     bp->array[m][i][j+1][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j  ][k  ] +
                                   bp->array[m][i][j  ][k+1] +
                                   bp->array[m][i][j+1][k  ] +
                                   bp->array[m][i][j+1][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j  ][k] +
                                     bp->array[m][i][j+1][k];
                  }
               }
               if (face_case%2 == 1) {
                  j = y_block_size + 1;
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j][k  ] +
                                   bp->array[m][i][j][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
            }
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to send */
            if (face_case%2 == 0) {
               js = 1;
               je = y_block_half;
            } else {
               js = y_block_half + 1;
               je = y_block_size;
            }
            if ((face_case/2)%2 == 1) {
               ks = 1;
               ke = z_block_half;
            } else {
               ks = z_block_half + 1;
               ke = z_block_size;
            }
            for (n = 0, m = start; m < start+num_comm; m++) {
               j = js - 1;
               k = ks - 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
               for (k = ks; k <= ke; k++, n+=2)
                  send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
               k = ke + 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
               for (j = js; j <= je; j++) {
                  k = ks - 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
                  for (k = ks; k <= ke; k++, n+=2)
                     send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
                  k = ke + 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
                  k = ks - 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
                  for (k = ks; k <= ke; k++, n+=2)
                     send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
                  k = ke + 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
               }
               j = je + 1;
               k = ks - 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
               for (k = ks; k <= ke; k++, n+=2)
                  send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
               k = ke + 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
            }
         }

      } else if (dir == 1) { /* Y - North, South */

         if (face_case >= 10) { /* +Y - North */
            j = y_block_size;
            face_case = face_case - 10;
         } else                 /* -Y - South */
            j = 1;
         if (face_case < 2) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (k = 0; k <= z_block_size+1; k++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face */
            for (n = 0, m = start; m < start+num_comm; m++) {
               if (face_case%2 == 0) {
                  i = 0;
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j][k  ] +
                                   bp->array[m][i][j][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
               for (i = 1; i < x_block_size; i += 2) {
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i  ][j][k] +
                                     bp->array[m][i+1][j][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i  ][j][k  ] +
                                   bp->array[m][i  ][j][k+1] +
                                   bp->array[m][i+1][j][k  ] +
                                   bp->array[m][i+1][j][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i  ][j][k] +
                                     bp->array[m][i+1][j][k];
                  }
               }
               if (face_case%2 == 1) {
                  i = x_block_size + 1;
                  if ((face_case/2)%2 == 1) {
                     k = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (k = 1; k < z_block_size; k += 2, n++)
                     send_buf[n] = bp->array[m][i][j][k  ] +
                                   bp->array[m][i][j][k+1];
                  if ((face_case/2)%2 == 0) {
                     k = z_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
            }
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to send */
            if (face_case%2 == 0) {
               is = 1;
               ie = x_block_half;
            } else {
               is = x_block_half + 1;
               ie = x_block_size;
            }
            if ((face_case/2)%2 == 1) {
               ks = 1;
               ke = z_block_half;
            } else {
               ks = z_block_half + 1;
               ke = z_block_size;
            }
            for (n = 0, m = start; m < start+num_comm; m++) {
               i = is - 1;
               k = ks - 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
               for (k = ks; k <= ke; k++, n+=2)
                  send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
               k = ke + 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
               for (i = is; i <= ie; i++) {
                  k = ks - 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
                  for (k = ks; k <= ke; k++, n+=2)
                     send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
                  k = ke + 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
                  k = ks - 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
                  for (k = ks; k <= ke; k++, n+=2)
                     send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
                  k = ke + 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
               }
               i = ie + 1;
               k = ks - 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
               for (k = ks; k <= ke; k++, n+=2)
                  send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
               k = ke + 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
            }
         }

      } else {               /* Z - Up, Down */

         /* Z directions (Up and Down) sent last
         */
         if (face_case >= 10) { /* +Z - Up */
            k = z_block_size;
            face_case = face_case - 10;
         } else                 /* -Z - Down */
            k = 1;
         if (face_case < 2) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (j = 0; j <= y_block_size+1; j++, n++)
                     send_buf[n] = bp->array[m][i][j][k];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face - case does not matter */
            for (n = 0, m = start; m < start+num_comm; m++) {
               if (face_case%2 == 0) {
                  i = 0;
                  if ((face_case/2)%2 == 1) {
                     j = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (j = 1; j < y_block_size; j += 2, n++)
                     send_buf[n] = bp->array[m][i][j  ][k] +
                                   bp->array[m][i][j+1][k];
                  if ((face_case/2)%2 == 0) {
                     j = y_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
               for (i = 1; i < x_block_size; i += 2) {
                  if ((face_case/2)%2 == 1) {
                     j = 0;
                     send_buf[n++] = bp->array[m][i  ][j][k] +
                                     bp->array[m][i+1][j][k];
                  }
                  for (j = 1; j < y_block_size; j += 2, n++)
                     send_buf[n] = bp->array[m][i  ][j  ][k] +
                                   bp->array[m][i  ][j+1][k] +
                                   bp->array[m][i+1][j  ][k] +
                                   bp->array[m][i+1][j+1][k];
                  if ((face_case/2)%2 == 0) {
                     j = y_block_size + 1;
                     send_buf[n++] = bp->array[m][i  ][j][k] +
                                     bp->array[m][i+1][j][k];
                  }
               }
               if (face_case%2 == 1) {
                  i = x_block_size + 1;
                  if ((face_case/2)%2 == 1) {
                     j = 0;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
                  for (j = 1; j < y_block_size; j += 2, n++)
                     send_buf[n] = bp->array[m][i][j  ][k] +
                                   bp->array[m][i][j+1][k];
                  if ((face_case/2)%2 == 0) {
                     j = y_block_size + 1;
                     send_buf[n++] = bp->array[m][i][j][k];
                  }
               }
            }
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to send */
            if (face_case%2 == 0) {
               is = 1;
               ie = x_block_half;
            } else {
               is = x_block_half + 1;
               ie = x_block_size;
            }
            if ((face_case/2)%2 == 1) {
               js = 1;
               je = y_block_half;
            } else {
               js = y_block_half + 1;
               je = y_block_size;
            }
            for (n = 0, m = start; m < start+num_comm; m++) {
               i = is - 1;
               j = js - 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
               for (j = js; j <= je; j++, n+=2)
                  send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
               j = je + 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
               for (i = is; i <= ie; i++) {
                  j = js - 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
                  for (j = js; j <= je; j++, n+=2)
                     send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
                  j = je + 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
                  j = js - 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
                  for (j = js; j <= je; j++, n+=2)
                     send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
                  j = je + 1;
                  send_buf[n++] = bp->array[m][i][j][k]/4.0;
               }
               i = ie + 1;
               j = js - 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
               for (j = js; j <= je; j++, n+=2)
                  send_buf[n] = send_buf[n+1] = bp->array[m][i][j][k]/4.0;
               j = je + 1;
               send_buf[n++] = bp->array[m][i][j][k]/4.0;
            }
         }
      }
   }
}

// Unpack ghost values that have been recieved.
// The sense of the face case is reversed since we are receiving what was sent
void unpack_face(double *recv_buf, int block_num, int face_case, int dir,
                 int start, int num_comm)
{
   int i, j, k, n, m;
   int is, ie, js, je, ks, ke;
   block *bp;

   bp = &blocks[block_num];

   if (!code) {

      if (dir == 0) {        /* X - East, West */

         /* X directions (East and West)
            just recv the real values and no ghosts
            face_case based on send - so reverse
         */
         if (face_case >= 10) { /* +X - from East */
            i = x_block_size + 1;
            face_case = face_case - 10;
         } else                 /* -X - from West */
            i = 0;
         if (face_case < 2) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = 1; j <= y_block_size; j++)
                  for (k = 1; k <= z_block_size; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face - one case */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = 1; j < y_block_size; j += 2)
                  for (k = 1; k < z_block_size; k += 2, n++)
                     bp->array[m][i][j  ][k  ] =
                     bp->array[m][i][j  ][k+1] =
                     bp->array[m][i][j+1][k  ] =
                     bp->array[m][i][j+1][k+1] = recv_buf[n];
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to recv */
            if (face_case%2 == 0) {
               js = 1;
               je = y_block_half;
            } else {
               js = y_block_half + 1;
               je = y_block_size;
            }
            if ((face_case/2)%2 == 1) {
               ks = 1;
               ke = z_block_half;
            } else {
               ks = z_block_half + 1;
               ke = z_block_size;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = js; j <= je; j++)
                  for (k = ks; k <= ke; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         }

      } else if (dir == 1) { /* Y - North, South */

         /* Y directions (North and South) sent second, so recv the real values
         */
         if (face_case >= 10) { /* +Y - from North */
            j = y_block_size + 1;
            face_case = face_case - 10;
         } else                 /* -Y - from South */
            j = 0;
         if (face_case == 0) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 1; i <= x_block_size; i++)
                  for (k = 1; k <= z_block_size; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else if (face_case == 1) {
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (k = 1; k <= z_block_size; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else if (face_case >= 2 && face_case <= 5) {
            /* one case - recv into 4 cells per cell sent */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 1; i < x_block_size; i += 2)
                  for (k = 1; k < z_block_size; k += 2, n++)
                     bp->array[m][i  ][j][k  ] =
                     bp->array[m][i  ][j][k+1] =
                     bp->array[m][i+1][j][k  ] =
                     bp->array[m][i+1][j][k+1] = recv_buf[n];
         } else {                     /* quarter face -> whole face */
            /* whole face -> quarter face - determine case */
            if (face_case%2 == 0) {
               is = 1;
               ie = x_block_half;
            } else {
               is = x_block_half + 1;
               ie = x_block_size;
            }
            if ((face_case/2)%2 == 1) {
               ks = 1;
               ke = z_block_half;
            } else {
               ks = z_block_half + 1;
               ke = z_block_size;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = is; i <= ie; i++)
                  for (k = ks; k <= ke; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         }

      } else {               /* Z - Up, Down */

         /* Z directions (Up and Down) sent last
         */
         if (face_case >= 10) { /* +Z - from Up */
            k = z_block_size + 1;
            face_case = face_case - 10;
         } else                 /* -Z - from Down */
            k = 0;
         if (face_case == 0) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 1; i <= x_block_size; i++)
                  for (j = 1; j <= y_block_size; j++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else if (face_case == 1) {
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (j = 0; j <= y_block_size+1; j++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else if (face_case >= 2 && face_case <= 5) {
            /* one case - receive into 4 cells */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 1; i < x_block_size; i += 2)
                  for (j = 1; j < y_block_size; j += 2, n++)
                     bp->array[m][i  ][j  ][k] =
                     bp->array[m][i  ][j+1][k] =
                     bp->array[m][i+1][j  ][k] =
                     bp->array[m][i+1][j+1][k] = recv_buf[n];
         } else {                     /* quarter face -> whole face */
            /* whole face -> quarter face - determine case */
            if (face_case%2 == 0) {
               is = 1;
               ie = x_block_half;
            } else {
               is = x_block_half + 1;
               ie = x_block_size;
            }
            if ((face_case/2)%2 == 1) {
               js = 1;
               je = y_block_half;
            } else {
               js = y_block_half + 1;
               je = y_block_size;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = is; i <= ie; i++)
                  for (j = js; j <= je; j++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         }
      }

   } else if (code == 1) {  /* send ghosts */

      if (dir == 0) {        /* X - East, West */

         if (face_case >= 10) { /* +X - from East */
            i = x_block_size + 1;
            face_case = face_case - 10;
         } else                 /* -X - from West */
            i = 0;
         if (face_case < 2) {        /* whole face -> whole */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = 0; j <= y_block_size+1; j++)
                  for (k = 0; k <= z_block_size+1; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face */
            for (n = 0, m = start; m < start+num_comm; m++) {
               j = 0;
               k = 0;
               bp->array[m][i][j][k] = recv_buf[n++];
               for (k = 1; k < z_block_size; k += 2, n++)
                  bp->array[m][i][j][k  ] =
                  bp->array[m][i][j][k+1] = recv_buf[n];
               k = z_block_size + 1;
               bp->array[m][i][j][k] = recv_buf[n++];
               for (j = 1; j < y_block_size; j += 2) {
                  k = 0;
                  bp->array[m][i][j  ][k] =
                  bp->array[m][i][j+1][k] = recv_buf[n++];
                  for (k = 1; k < z_block_size; k += 2, n++)
                     bp->array[m][i][j  ][k  ] =
                     bp->array[m][i][j  ][k+1] =
                     bp->array[m][i][j+1][k  ] =
                     bp->array[m][i][j+1][k+1] = recv_buf[n];
                  k = z_block_size + 1;
                  bp->array[m][i][j  ][k] =
                  bp->array[m][i][j+1][k] = recv_buf[n++];
               }
               j = y_block_size + 1;
               k = 0;
               bp->array[m][i][j][k] = recv_buf[n++];
               for (k = 1; k < z_block_size; k += 2, n++)
                  bp->array[m][i][j][k  ] =
                  bp->array[m][i][j][k+1] = recv_buf[n];
               k = z_block_size + 1;
               bp->array[m][i][j][k] = recv_buf[n++];
            }
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to recv */
            if (face_case%2 == 0) {
               js = 0;
               je = y_block_half;
            } else {
               js = y_block_half + 1;
               je = y_block_size + 1;
            }
            if ((face_case/2)%2 == 1) {
               ks = 0;
               ke = z_block_half;
            } else {
               ks = z_block_half + 1;
               ke = z_block_size + 1;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = js; j <= je; j++)
                  for (k = ks; k <= ke; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         }

      } else if (dir == 1) { /* Y - North, South */

         if (face_case >= 10) { /* +Y - from North */
            j = y_block_size + 1;
            face_case = face_case - 10;
         } else                 /* -Y - from South */
            j = 0;
         if (face_case < 2) {        /* whole face -> whole */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (k = 0; k <= z_block_size+1; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face */
            for (n = 0, m = start; m < start+num_comm; m++) {
               i = 0;
               k = 0;
               bp->array[m][i][j][k] = recv_buf[n++];
               for (k = 1; k < z_block_size; k += 2, n++)
                  bp->array[m][i][j][k  ] =
                  bp->array[m][i][j][k+1] = recv_buf[n];
               k = z_block_size + 1;
               bp->array[m][i][j][k] = recv_buf[n++];
               for (i = 1; i < x_block_size; i += 2) {
                  k = 0;
                  bp->array[m][i  ][j][k] =
                  bp->array[m][i+1][j][k] = recv_buf[n++];
                  for (k = 1; k < z_block_size; k += 2, n++)
                     bp->array[m][i  ][j][k  ] =
                     bp->array[m][i  ][j][k+1] =
                     bp->array[m][i+1][j][k  ] =
                     bp->array[m][i+1][j][k+1] = recv_buf[n];
                  k = z_block_size + 1;
                  bp->array[m][i  ][j][k] =
                  bp->array[m][i+1][j][k] = recv_buf[n++];
               }
               i = x_block_size + 1;
               k = 0;
               bp->array[m][i][j][k] = recv_buf[n++];
               for (k = 1; k < z_block_size; k += 2, n++)
                  bp->array[m][i][j][k  ] =
                  bp->array[m][i][j][k+1] = recv_buf[n];
               k = z_block_size + 1;
               bp->array[m][i][j][k] = recv_buf[n++];
            }
         } else {                     /* quarter face -> whole face */
            /* whole face -> quarter face - determine case */
            if (face_case%2 == 0) {
               is = 0;
               ie = x_block_half;
            } else {
               is = x_block_half + 1;
               ie = x_block_size + 1;
            }
            if ((face_case/2)%2 == 1) {
               ks = 0;
               ke = z_block_half;
            } else {
               ks = z_block_half + 1;
               ke = z_block_size + 1;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = is; i <= ie; i++)
                  for (k = ks; k <= ke; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         }

      } else {               /* Z - Up, Down */

         if (face_case >= 10) { /* +Z - from Up */
            k = z_block_size + 1;
            face_case = face_case - 10;
         } else                 /* -Z - from Down */
            k = 0;
         if (face_case < 2) {        /* whole face -> whole face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (j = 0; j <= y_block_size+1; j++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else if (face_case >= 2 && face_case <= 5) {
            /* whole face -> quarter face */
            for (n = 0, m = start; m < start+num_comm; m++) {
               i = 0;
               j = 0;
               bp->array[m][i][j][k] = recv_buf[n++];
               for (j = 1; j < y_block_size; j += 2, n++)
                  bp->array[m][i][j  ][k] =
                  bp->array[m][i][j+1][k] = recv_buf[n];
               j = y_block_size + 1;
               bp->array[m][i][j][k] = recv_buf[n++];
               for (i = 1; i < x_block_size; i += 2) {
                  j = 0;
                  bp->array[m][i  ][j][k] =
                  bp->array[m][i+1][j][k] = recv_buf[n++];
                  for (j = 1; j < y_block_size; j += 2, n++)
                     bp->array[m][i  ][j  ][k] =
                     bp->array[m][i  ][j+1][k] =
                     bp->array[m][i+1][j  ][k] =
                     bp->array[m][i+1][j+1][k] = recv_buf[n];
                  j = y_block_size + 1;
                  bp->array[m][i  ][j][k] =
                  bp->array[m][i+1][j][k] = recv_buf[n++];
               }
               i = x_block_size + 1;
               j = 0;
               bp->array[m][i][j][k] = recv_buf[n++];
               for (j = 1; j < y_block_size; j += 2, n++)
                  bp->array[m][i][j  ][k] =
                  bp->array[m][i][j+1][k] = recv_buf[n];
               j = y_block_size + 1;
               bp->array[m][i][j][k] = recv_buf[n++];
            }
         } else {                     /* quarter face -> whole face */
            /* whole face -> quarter face - determine case */
            if (face_case%2 == 0) {
               is = 0;
               ie = x_block_half;
            } else {
               is = x_block_half + 1;
               ie = x_block_size + 1;
            }
            if ((face_case/2)%2 == 1) {
               js = 0;
               je = y_block_half;
            } else {
               js = y_block_half + 1;
               je = y_block_size + 1;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = is; i <= ie; i++)
                  for (j = js; j <= je; j++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         }
      }

   } else {  /* code == 2 send ghosts and process on send */

      if (dir == 0) {        /* X - East, West */

         if (face_case >= 10) { /* +X - from East */
            i = x_block_size + 1;
            face_case = face_case - 10;
         } else                 /* -X - from West */
            i = 0;
         if (face_case <= 5) {        /* whole face -> whole or quarter face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = 0; j <= y_block_size+1; j++)
                  for (k = 0; k <= z_block_size+1; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else {                     /* quarter face -> whole face */
            /* four cases - figure out which quarter of face to recv */
            if (face_case%2 == 0) {
               js = 0;
               je = y_block_half;
            } else {
               js = y_block_half + 1;
               je = y_block_size + 1;
            }
            if ((face_case/2)%2 == 1) {
               ks = 0;
               ke = z_block_half;
            } else {
               ks = z_block_half + 1;
               ke = z_block_size + 1;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (j = js; j <= je; j++)
                  for (k = ks; k <= ke; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         }

      } else if (dir == 1) { /* Y - North, South */

         if (face_case >= 10) { /* +Y - from North */
            j = y_block_size + 1;
            face_case = face_case - 10;
         } else                 /* -Y - from South */
            j = 0;
         if (face_case <= 5) {        /* whole face -> whole or quarter face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (k = 0; k <= z_block_size+1; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else {                     /* quarter face -> whole face */
            /* whole face -> quarter face - determine case */
            if (face_case%2 == 0) {
               is = 0;
               ie = x_block_half;
            } else {
               is = x_block_half + 1;
               ie = x_block_size + 1;
            }
            if ((face_case/2)%2 == 1) {
               ks = 0;
               ke = z_block_half;
            } else {
               ks = z_block_half + 1;
               ke = z_block_size + 1;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = is; i <= ie; i++)
                  for (k = ks; k <= ke; k++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         }

      } else {               /* Z - Up, Down */

         if (face_case >= 10) { /* +Z - from Up */
            k = z_block_size + 1;
            face_case = face_case - 10;
         } else                 /* -Z - from Down */
            k = 0;
         if (face_case <= 5) {        /* whole face -> whole or quarter face */
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = 0; i <= x_block_size+1; i++)
                  for (j = 0; j <= y_block_size+1; j++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         } else {                     /* quarter face -> whole face */
            /* whole face -> quarter face - determine case */
            if (face_case%2 == 0) {
               is = 0;
               ie = x_block_half;
            } else {
               is = x_block_half + 1;
               ie = x_block_size + 1;
            }
            if ((face_case/2)%2 == 1) {
               js = 0;
               je = y_block_half;
            } else {
               js = y_block_half + 1;
               je = y_block_size + 1;
            }
            for (n = 0, m = start; m < start+num_comm; m++)
               for (i = is; i <= ie; i++)
                  for (j = js; j <= je; j++, n++)
                     bp->array[m][i][j][k] = recv_buf[n];
         }
      }
   }
}

// Routine that does on processor communication between two blocks that
// are at the same level of refinement.
void on_proc_comm(int n, int n1, int l, int start, int num_comm)
{
   int i, j, k, m;
   int is, ie, js, je;
   block *bp, *bp1;

   /* Determine direction and then exchange data across the face
   */
   if (!code) {
      if ((l/2) == 0) {         /* West, East */
         if ((l%2) == 0) {      /* West */
            bp = &blocks[n];
            bp1 = &blocks[n1];
         } else {               /* East */
            bp1 = &blocks[n];
            bp = &blocks[n1];
         }
         for (m = start; m < start+num_comm; m++)
            for (j = 1; j <= y_block_size; j++)
               for (k = 1; k <= z_block_size; k++) {
                  bp1->array[m][x_block_size+1][j][k] = bp->array[m][1][j][k];
                  bp->array[m][0][j][k] = bp1->array[m][x_block_size][j][k];
               }
      } else if ((l/2) == 1) {  /* South, North */
         if ((l%2) == 0) {      /* South */
            bp = &blocks[n];
            bp1 = &blocks[n1];
         } else {               /* North */
            bp1 = &blocks[n];
            bp = &blocks[n1];
         }
         if (stencil == 7) {
           is = 1;
           ie = x_block_size;
         } else {
           is = 0;
           ie = x_block_size + 1;
         }
         for (m = start; m < start+num_comm; m++)
            for (i = is; i <= ie; i++)
               for (k = 1; k <= z_block_size; k++) {
                  bp1->array[m][i][y_block_size+1][k] = bp->array[m][i][1][k];
                  bp->array[m][i][0][k] = bp1->array[m][i][y_block_size][k];
               }
      } else if ((l/2) == 2) {  /* Down, Up */
         if ((l%2) == 0) {      /* Down */
            bp = &blocks[n];
            bp1 = &blocks[n1];
         } else {               /* Up */
            bp1 = &blocks[n];
            bp = &blocks[n1];
         }
         if (stencil == 7) {
           is = 1;
           ie = x_block_size;
           js = 1;
           je = y_block_size;
         } else {
           is = 0;
           ie = x_block_size + 1;
           js = 0;
           je = y_block_size + 1;
         }
         for (m = start; m < start+num_comm; m++)
            for (i = is; i <= ie; i++)
               for (j = js; j <= je; j++) {
                  bp1->array[m][i][j][z_block_size+1] = bp->array[m][i][j][1];
                  bp->array[m][i][j][0] = bp1->array[m][i][j][z_block_size];
               }
      }
   } else {  /* set all ghosts */
      if ((l/2) == 0) {         /* West, East */
         if ((l%2) == 0) {      /* West */
            bp = &blocks[n];
            bp1 = &blocks[n1];
         } else {               /* East */
            bp1 = &blocks[n];
            bp = &blocks[n1];
         }
         for (m = start; m < start+num_comm; m++)
            for (j = 0; j <= y_block_size+1; j++)
               for (k = 0; k <= z_block_size+1; k++) {
                  bp1->array[m][x_block_size+1][j][k] = bp->array[m][1][j][k];
                  bp->array[m][0][j][k] = bp1->array[m][x_block_size][j][k];
               }
      } else if ((l/2) == 1) {  /* South, North */
         if ((l%2) == 0) {      /* South */
            bp = &blocks[n];
            bp1 = &blocks[n1];
         } else {               /* North */
            bp1 = &blocks[n];
            bp = &blocks[n1];
         }
         for (m = start; m < start+num_comm; m++)
            for (i = 0; i <= x_block_size+1; i++)
               for (k = 0; k <= z_block_size+1; k++) {
                  bp1->array[m][i][y_block_size+1][k] = bp->array[m][i][1][k];
                  bp->array[m][i][0][k] = bp1->array[m][i][y_block_size][k];
               }
      } else if ((l/2) == 2) {  /* Down, Up */
         if ((l%2) == 0) {      /* Down */
            bp = &blocks[n];
            bp1 = &blocks[n1];
         } else {               /* Up */
            bp1 = &blocks[n];
            bp = &blocks[n1];
         }
         for (m = start; m < start+num_comm; m++)
            for (i = 0; i <= x_block_size+1; i++)
               for (j = 0; j <= y_block_size+1; j++) {
                  bp1->array[m][i][j][z_block_size+1] = bp->array[m][i][j][1];
                  bp->array[m][i][j][0] = bp1->array[m][i][j][z_block_size];
               }
      }
   }
}

// Routine that does on processor communication between two blocks that are
// at different levels of refinement.  The order of the blocks that are
// being input determine which block is at a higher level of refinement.
void on_proc_comm_diff(int n, int n1, int l, int iq, int jq,
                       int start, int num_comm)
{
   int i, j, k, m;
   int i0, i1, i2, i3, j0, j1, j2, j3, k0, k1, k2, k3;
   block *bp, *bp1;

   bp = &blocks[n];
   bp1 = &blocks[n1];

   /* (iq, jq) quarter face on block n to whole face on block n1
   */
   if (!code) {
      /* only have to communicate ghost values - bp is level, bp1 is level+1 -
       * in 2 to 1 case get 0..block_half from one proc and
       *                block_half+1..block_size+1 from another
       * in 1 to 2 case get 0..block_size+1 from 0..block_half+1 or
       *                block_half..block_size+1 with averages
       */
      if ((l/2) == 0) {
         if (l == 0) {             /* West */
            i0 = 0;
            i1 = 1;
            i2 = x_block_size + 1;
            i3 = x_block_size;
         } else {                  /* East */
            i0 = x_block_size + 1;
            i1 = x_block_size;
            i2 = 0;
            i3 = 1;
         }
         j1 = jq*y_block_half;
         k1 = iq*z_block_half;
         for (m = start; m < start+num_comm; m++)
            for (j = 1; j <= y_block_half; j++)
               for (k = 1; k <= z_block_half; k++) {
                  bp1->array[m][i2][2*j-1][2*k-1] =
                  bp1->array[m][i2][2*j-1][2*k  ] =
                  bp1->array[m][i2][2*j  ][2*k-1] =
                  bp1->array[m][i2][2*j  ][2*k  ] =
                                             bp->array[m][i1][j+j1][k+k1]/4.0;
                  bp->array[m][i0][j+j1][k+k1] =
                                             bp1->array[m][i3][2*j-1][2*k-1] +
                                             bp1->array[m][i3][2*j-1][2*k  ] +
                                             bp1->array[m][i3][2*j  ][2*k-1] +
                                             bp1->array[m][i3][2*j  ][2*k  ];
               }
      } else if ((l/2) == 1) {
         if (l == 2) {             /* South */
            j0 = 0;
            j1 = 1;
            j2 = y_block_size + 1;
            j3 = y_block_size;
         } else {                  /* North */
            j0 = y_block_size + 1;
            j1 = y_block_size;
            j2 = 0;
            j3 = 1;
         }
         i1 = jq*x_block_half;
         k1 = iq*z_block_half;
         for (m = start; m < start+num_comm; m++)
            for (i = 1; i <= x_block_half; i++)
               for (k = 1; k <= z_block_half; k++) {
                  bp1->array[m][2*i-1][j2][2*k-1] =
                  bp1->array[m][2*i-1][j2][2*k  ] =
                  bp1->array[m][2*i  ][j2][2*k-1] =
                  bp1->array[m][2*i  ][j2][2*k  ] =
                                             bp->array[m][i+i1][j1][k+k1]/4.0;
                  bp->array[m][i+i1][j0][k+k1] =
                                             bp1->array[m][2*i-1][j3][2*k-1] +
                                             bp1->array[m][2*i-1][j3][2*k  ] +
                                             bp1->array[m][2*i  ][j3][2*k-1] +
                                             bp1->array[m][2*i  ][j3][2*k  ];
               }
      } else if ((l/2) == 2) {
         if (l == 4) {             /* Down */
            k0 = 0;
            k1 = 1;
            k2 = z_block_size + 1;
            k3 = z_block_size;
         } else {                  /* Up */
            k0 = z_block_size + 1;
            k1 = z_block_size;
            k2 = 0;
            k3 = 1;
         }
         i1 = jq*x_block_half;
         j1 = iq*y_block_half;
         for (m = start; m < start+num_comm; m++)
            for (i = 1; i <= x_block_half; i++)
               for (j = 1; j <= y_block_half; j++) {
                  bp1->array[m][2*i-1][2*j-1][k2] =
                  bp1->array[m][2*i-1][2*j  ][k2] =
                  bp1->array[m][2*i  ][2*j-1][k2] =
                  bp1->array[m][2*i  ][2*j  ][k2] =
                                              bp->array[m][i+i1][j+j1][k1]/4.0;
                  bp->array[m][i+i1][j+j1][k0] =
                                              bp1->array[m][2*i-1][2*j-1][k3] +
                                              bp1->array[m][2*i-1][2*j  ][k3] +
                                              bp1->array[m][2*i  ][2*j-1][k3] +
                                              bp1->array[m][2*i  ][2*j  ][k3];
               }
      }
   } else {  /* transfer ghosts */
      if ((l/2) == 0) {
         if (l == 0) {             /* West */
            i0 = 0;
            i1 = 1;
            i2 = x_block_size + 1;
            i3 = x_block_size;
         } else {                  /* East */
            i0 = x_block_size + 1;
            i1 = x_block_size;
            i2 = 0;
            i3 = 1;
         }
         j1 = jq*y_block_half;
         k1 = iq*z_block_half;
         j2 = y_block_size + 1;
         j3 = y_block_half + 1;
         k2 = z_block_size + 1;
         k3 = z_block_half + 1;
         for (m = start; m < start+num_comm; m++) {
            bp1->array[m][i2][0][0] = bp->array[m][i1][j1][k1]/4.0;
            for (k = 1; k <= z_block_half; k++)
               bp1->array[m][i2][0][2*k-1] =
               bp1->array[m][i2][0][2*k  ] = bp->array[m][i1][j1][k+k1]/4.0;
            bp1->array[m][i2][0][k2] = bp->array[m][i1][j1][k3+k1]/4.0;
            if (jq == 0) {
               if (iq == 0)
                  bp->array[m][i0][0][0 ] = bp1->array[m][i3][0][0 ];
               else
                  bp->array[m][i0][0][k2] = bp1->array[m][i3][0][k2];
               for (k = 1; k <= z_block_half; k++)
                  bp->array[m][i0][0][k+k1] = (bp1->array[m][i3][0][2*k-1] +
                                               bp1->array[m][i3][0][2*k  ]);
            }
            for (j = 1; j <= y_block_half; j++) {
               bp1->array[m][i2][2*j-1][0] =
               bp1->array[m][i2][2*j  ][0] = bp->array[m][i1][j+j1][k1]/4.0;
               if (iq == 0)
                  bp->array[m][i0][j+j1][0 ] = (bp1->array[m][i3][2*j-1][0 ] +
                                                bp1->array[m][i3][2*j  ][0 ]);
               else
                  bp->array[m][i0][j+j1][k2] = (bp1->array[m][i3][2*j-1][k2] +
                                                bp1->array[m][i3][2*j  ][k2]);
               for (k = 1; k <= z_block_half; k++) {
                  bp1->array[m][i2][2*j-1][2*k-1] =
                  bp1->array[m][i2][2*j-1][2*k  ] =
                  bp1->array[m][i2][2*j  ][2*k-1] =
                  bp1->array[m][i2][2*j  ][2*k  ] =
                                             bp->array[m][i1][j+j1][k+k1]/4.0;
                  bp->array[m][i0][j+j1][k+k1] =
                                             bp1->array[m][i3][2*j-1][2*k-1] +
                                             bp1->array[m][i3][2*j-1][2*k  ] +
                                             bp1->array[m][i3][2*j  ][2*k-1] +
                                             bp1->array[m][i3][2*j  ][2*k  ];
               }
               bp1->array[m][i2][2*j-1][k2] =
               bp1->array[m][i2][2*j  ][k2] = bp->array[m][i1][j+j1][k3+k1]/4.0;
            }
            bp1->array[m][i2][j2][0] = bp->array[m][i1][j3+j1][k1]/4.0;
            for (k = 1; k <= z_block_half; k++)
               bp1->array[m][i2][j2][2*k-1] =
               bp1->array[m][i2][j2][2*k  ] = bp->array[m][i1][j3+j1][k+k1]/4.0;
            bp1->array[m][i2][j2][k2] = bp->array[m][i1][j3+j1][k3+k1]/4.0;
            if (jq == 1) {
               if (iq == 0)
                  bp->array[m][i0][j2][0 ] = bp1->array[m][i3][j2][0 ];
               else
                  bp->array[m][i0][j2][k2] = bp1->array[m][i3][j2][k2];
               for (k = 1; k <= z_block_half; k++)
                  bp->array[m][i0][j2][k+k1] = (bp1->array[m][i3][j2][2*k-1] +
                                                bp1->array[m][i3][j2][2*k  ]);
            }
         }
      } else if ((l/2) == 1) {
         if (l == 2) {             /* South */
            j0 = 0;
            j1 = 1;
            j2 = y_block_size + 1;
            j3 = y_block_size;
         } else {                  /* North */
            j0 = y_block_size + 1;
            j1 = y_block_size;
            j2 = 0;
            j3 = 1;
         }
         i1 = jq*x_block_half;
         k1 = iq*z_block_half;
         i2 = x_block_size + 1;
         i3 = x_block_half + 1;
         k2 = z_block_size + 1;
         k3 = z_block_half + 1;
         for (m = start; m < start+num_comm; m++) {
            bp1->array[m][0][j2][0 ] = bp->array[m][i1][j1][k1]/4.0;
            for (k = 1; k <= z_block_half; k++)
               bp1->array[m][0][j2][2*k-1] =
               bp1->array[m][0][j2][2*k  ] = bp->array[m][i1][j1][k+k1]/4.0;
            bp1->array[m][0][j2][k2] = bp->array[m][i1][j1][k3+k1]/4.0;
            if (jq == 0) {
               if (iq == 0)
                  bp->array[m][0][j0][0 ] = bp1->array[m][0][j3][0 ];
               else
                  bp->array[m][0][j0][k2] = bp1->array[m][0][j3][k2];
               for (k = 1; k <= z_block_half; k++)
                  bp->array[m][0][j0][k+k1] = (bp1->array[m][0][j3][2*k-1] +
                                               bp1->array[m][0][j3][2*k  ]);
            }
            for (i = 1; i <= x_block_half; i++) {
               bp1->array[m][2*i-1][j2][0] =
               bp1->array[m][2*i  ][j2][0] = bp->array[m][i+i1][j1][k1]/4.0;
               if (iq == 0)
                  bp->array[m][i+i1][j0][0 ] = (bp1->array[m][2*i-1][j3][0 ] +
                                                bp1->array[m][2*i  ][j3][0 ]);
               else
                  bp->array[m][i+i1][j0][k2] = (bp1->array[m][2*i-1][j3][k2] +
                                                bp1->array[m][2*i  ][j3][k2]);
               for (k = 1; k <= z_block_half; k++) {
                  bp1->array[m][2*i-1][j2][2*k-1] =
                  bp1->array[m][2*i-1][j2][2*k  ] =
                  bp1->array[m][2*i  ][j2][2*k-1] =
                  bp1->array[m][2*i  ][j2][2*k  ] =
                                             bp->array[m][i+i1][j1][k+k1]/4.0;
                  bp->array[m][i+i1][j0][k+k1] =
                                             bp1->array[m][2*i-1][j3][2*k-1] +
                                             bp1->array[m][2*i-1][j3][2*k  ] +
                                             bp1->array[m][2*i  ][j3][2*k-1] +
                                             bp1->array[m][2*i  ][j3][2*k  ];
               }
               bp1->array[m][2*i-1][j2][k2] =
               bp1->array[m][2*i  ][j2][k2] = bp->array[m][i+i1][j1][k3+k1]/4.0;
            }
            bp1->array[m][i2][j2][0 ] = bp->array[m][i3+i1][j1][k1]/4.0;
            for (k = 1; k <= z_block_half; k++)
               bp1->array[m][i2][j2][2*k-1] =
               bp1->array[m][i2][j2][2*k  ] = bp->array[m][i3+i1][j1][k+k1]/4.0;
            bp1->array[m][i2][j2][k2] = bp->array[m][i3+i1][j1][k3+k1]/4.0;
            if (jq == 1) {
               if (iq == 0)
                  bp->array[m][i2][j0][0 ] = bp1->array[m][i2][j3][0 ];
               else
                  bp->array[m][i2][j0][k2] = bp1->array[m][i2][j3][k2];
               for (k = 1; k <= z_block_half; k++)
                  bp->array[m][i2][j0][k+k1] = (bp1->array[m][i2][j3][2*k-1] +
                                                bp1->array[m][i2][j3][2*k  ]);
            }
         }
      } else if ((l/2) == 2) {
         if (l == 4) {             /* Down */
            k0 = 0;
            k1 = 1;
            k2 = z_block_size + 1;
            k3 = z_block_size;
         } else {                  /* Up */
            k0 = z_block_size + 1;
            k1 = z_block_size;
            k2 = 0;
            k3 = 1;
         }
         i1 = jq*x_block_half;
         j1 = iq*y_block_half;
         i2 = x_block_size + 1;
         i3 = x_block_half + 1;
         j2 = y_block_size + 1;
         j3 = y_block_half + 1;
         for (m = start; m < start+num_comm; m++) {
            bp1->array[m][0][0 ][k2] = bp->array[m][i1][j1][k1]/4.0;
            for (j = 1; j <= y_block_half; j++)
               bp1->array[m][0][2*j-1][k2] =
               bp1->array[m][0][2*j  ][k2] = bp->array[m][i1][j+j1][k1]/4.0;
            bp1->array[m][0][j2][k2] = bp->array[m][i1][j3+j1][k1]/4.0;
            if (jq == 0) {
               if (iq == 0)
                  bp->array[m][0][0 ][k0] = bp1->array[m][0][0 ][k3];
               else
                  bp->array[m][0][j2][k0] = bp1->array[m][0][j2][k3];
               for (j = 1; j <= y_block_half; j++)
                  bp->array[m][0][j+j1][k0] = (bp1->array[m][0][2*j-1][k3] +
                                               bp1->array[m][0][2*j  ][k3]);
            }
            for (i = 1; i <= x_block_half; i++) {
               bp1->array[m][2*i-1][0][k2] =
               bp1->array[m][2*i  ][0][k2] = bp->array[m][i+i1][j1][k1]/4.0;
               if (iq == 0)
                  bp->array[m][i+i1][0][k0] = (bp1->array[m][2*i-1][0][k3] +
                                               bp1->array[m][2*i  ][0][k3]);
               else
                  bp->array[m][i+i1][j2][k0] = (bp1->array[m][2*i-1][j2][k3] +
                                                bp1->array[m][2*i  ][j2][k3]);
               for (j = 1; j <= y_block_half; j++) {
                  bp1->array[m][2*i-1][2*j-1][k2] =
                  bp1->array[m][2*i-1][2*j  ][k2] =
                  bp1->array[m][2*i  ][2*j-1][k2] =
                  bp1->array[m][2*i  ][2*j  ][k2] =
                                              bp->array[m][i+i1][j+j1][k1]/4.0;
                  bp->array[m][i+i1][j+j1][k0] =
                                              bp1->array[m][2*i-1][2*j-1][k3] +
                                              bp1->array[m][2*i-1][2*j  ][k3] +
                                              bp1->array[m][2*i  ][2*j-1][k3] +
                                              bp1->array[m][2*i  ][2*j  ][k3];
               }
               bp1->array[m][2*i-1][j2][k2] =
               bp1->array[m][2*i  ][j2][k2] = bp->array[m][i+i1][j3+j1][k1]/4.0;
            }
            bp1->array[m][i2][0 ][k2] = bp->array[m][i3+i1][j1][k1]/4.0;
            for (j = 1; j <= y_block_half; j++)
               bp1->array[m][i2][2*j-1][k2] =
               bp1->array[m][i2][2*j  ][k2] = bp->array[m][i3+i1][j+j1][k1]/4.0;
            bp1->array[m][i2][j2][k2] = bp->array[m][i3+i1][j3+j1][k1]/4.0;
            if (jq == 1) {
               if (iq == 0)
                  bp->array[m][i2][0 ][k0] = bp1->array[m][i2][0 ][k3];
               else
                  bp->array[m][i2][j2][k0] = bp1->array[m][i2][j2][k3];
               for (j = 1; j <= y_block_half; j++)
                  bp->array[m][i2][j+j1][k0] = (bp1->array[m][i2][2*j-1][k3] +
                                                bp1->array[m][i2][2*j  ][k3]);
            }
         }
      }
   }
}

// Apply reflective boundary conditions to a face of a block.
void apply_bc(int l, block *bp, int start, int num_comm)
{
   int var, i, j, k, f, t;

   t = 0;
   f = 1;
   if (!code && stencil == 7)
      switch (l) {
         case 1: t = x_block_size + 1;
                 f = x_block_size;
                 /* FALLTHROUGH */
         case 0: for (var = start; var < start+num_comm; var++)
                    for (j = 1; j <= y_block_size; j++)
                       for (k = 1; k <= z_block_size; k++)
                          bp->array[var][t][j][k] = bp->array[var][f][j][k];
                 break;
         case 3: t = y_block_size + 1;
                 f = y_block_size;
                 /* FALLTHROUGH */
         case 2: for (var = start; var < start+num_comm; var++)
                    for (i = 1; i <= x_block_size; i++)
                       for (k = 1; k <= z_block_size; k++)
                          bp->array[var][i][t][k] = bp->array[var][i][f][k];
                 break;
         case 5: t = z_block_size + 1;
                 f = z_block_size;
                 /* FALLTHROUGH */
         case 4: for (var = start; var < start+num_comm; var++)
                    for (i = 1; i <= x_block_size; i++)
                       for (j = 1; j <= y_block_size; j++)
                          bp->array[var][i][j][t] = bp->array[var][i][j][f];
                 break;
      }
   else
      switch (l) {
         case 1: t = x_block_size + 1;
                 f = x_block_size;
                 /* FALLTHROUGH */
         case 0: for (var = start; var < start+num_comm; var++)
                    for (j = 0; j <= y_block_size+1; j++)
                       for (k = 0; k <= z_block_size+1; k++)
                          bp->array[var][t][j][k] = bp->array[var][f][j][k];
                 break;
         case 3: t = y_block_size + 1;
                 f = y_block_size;
                 /* FALLTHROUGH */
         case 2: for (var = start; var < start+num_comm; var++)
                    for (i = 0; i <= x_block_size+1; i++)
                       for (k = 0; k <= z_block_size+1; k++)
                          bp->array[var][i][t][k] = bp->array[var][i][f][k];
                 break;
         case 5: t = z_block_size + 1;
                 f = z_block_size;
                 /* FALLTHROUGH */
         case 4: for (var = start; var < start+num_comm; var++)
                    for (i = 0; i <= x_block_size+1; i++)
                       for (j = 0; j <= y_block_size+1; j++)
                          bp->array[var][i][j][t] = bp->array[var][i][j][f];
                 break;
      }
}
