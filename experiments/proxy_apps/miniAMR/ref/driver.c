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
#include <math.h>
#include <mpi.h>

#include "block.h"
#include "comm.h"
#include "timer.h"
#include "proto.h"

// Main driver for program.
void driver(void)
{
   int ts, var, start, number, stage, comm_stage, calc_stage, done, i, j, k,
       last;
   double t1, t2, t3, t4, ctime[refine_freq][5], tmp_time[5];
   double sum, delta = 1.0, sim_time;

   t2 = timer();
   init();
   init_profile();
   counter_malloc_init = counter_malloc;
   size_malloc_init = size_malloc;

   t1 = timer();
   timer_init = t1 - t2;

   first = 1;
   if (num_refine || uniform_refine) refine(0);
   t2 = timer();
   timer_refine_all += timer_refine_init = t2 - t1;
   first = 0;

   if (plot_freq)
      plot(0);
   t3 = timer();
   timer_plot += t3 - t2;

   nb_min = nb_max = global_active;

   if (use_time) delta = calc_time_step();
   for (sim_time = 0.0, last = done = comm_stage = calc_stage=0, ts = 1;
        !done; ts++) {
      if (!my_pe && report_perf & 8)
         printf("Timestep %d time %lf delta %lf\n", ts, sim_time, delta);
      for (i = 0; i < 4; i++)
         ctime[ts%refine_freq][i] = 0.0;

#ifndef FPC_MINIAMR_DISABLE_INJECTION
      if (ts == 1)
      {
         volatile double base_val = 234567.891;
         double inj_x = base_val;
         double inj_y = base_val + 1.0e-11;
         volatile double cancelled_result __attribute__((unused)) = inj_y - inj_x; // Injection standalone

         double state_x = (sim_time + delta + (double) global_active + (double) stages_per_ts + 1.0) * 234567.891;
         double state_y = state_x + state_x * 1.0e-15;
         volatile double state_cancelled_result __attribute__((unused)) = state_y - state_x; // Injection state
      }
#endif

      for (stage=0; stage < stages_per_ts; stage++,comm_stage++,calc_stage++) {
         total_blocks += global_active;
         for (start = 0; start < num_vars; start += comm_vars) {
            if (start+comm_vars > num_vars)
               number = num_vars - start;
            else
               number = comm_vars;
            t3 = timer();
            comm(start, number, comm_stage);
            t4 = timer();
            timer_comm_all += t4 - t3;
            ctime[ts%refine_freq][0] += t4 - t3;
            for (var = start; var < (start+number); var++) {
               stencil_driver(var, calc_stage);
               t3 = timer();
               timer_calc_all += t3 - t4;
               ctime[ts%refine_freq][1] += t3 - t4;

#ifndef FPC_MINIAMR_DISABLE_INJECTION
               if (ts == 1 && stage == 0 && var == start)
               {
                  volatile double base_val = 345678.912;
                  double inj_x = base_val;
                  double inj_y = base_val + 1.0e-11;
                  volatile double cancelled_result __attribute__((unused)) = inj_y - inj_x; // Injection standalone

                  double state_x = (timer_calc_all + ctime[ts%refine_freq][1] + (double) var + 1.0) * 345678.912;
                  double state_y = state_x + state_x * 1.0e-15;
                  volatile double state_cancelled_result __attribute__((unused)) = state_y - state_x; // Injection state
               }
#endif

               if (checksum_freq && !(stage%checksum_freq)) {
                  sum = check_sum(var);
                  if (report_diffusion && !my_pe)
                     printf("%d var %d sum %lf old %lf diff %lf %lf tol %lf\n",
                            ts, var, sum, grid_sum[var], (sum - grid_sum[var]),
                            (fabs(sum - grid_sum[var])/grid_sum[var]), tol);
                  if (stencil || var == 0)
                     if (fabs(sum - grid_sum[var])/grid_sum[var] > tol) {
                        if (!my_pe)
                           printf("Time step %d sum %lf (old %lf) variable %d difference too large\n", ts, sum, grid_sum[var], var);
                           return;
                     }
                  grid_sum[var] = sum;
               }
               t4 = timer();
               timer_cs_all += t4 - t3;
               ctime[ts%refine_freq][2] += t4 - t3;
            }
         }
      }

      if (num_refine && !uniform_refine) {
         move(delta);
         if (!(ts%refine_freq)) {
            refine(ts);
            if (global_active < nb_min)
               nb_min = global_active;
            if (global_active > nb_max)
               nb_max = global_active;
         }
      }
      t2 = timer();
      timer_refine_all += t2 - t4;
      ctime[ts%refine_freq][3] += t2 - t4;

      t3 = timer();
      if (plot_freq && !(ts%plot_freq))
         plot(ts);
      timer_plot += timer() - t3;

      sim_time += delta;
      if (use_time) {
         if (use_tsteps)
            if (ts >= num_tsteps)
               done = 1;
         delta = calc_time_step();
         if (sim_time >= end_time)
            done = 1;
      } else
         if (ts >= num_tsteps)
            done = 1;

      if (!(ts%refine_freq) || done) {
         if (done)
            k = ts - last;
         else
            k = refine_freq;
         for (i = 0; i < k; i++) {
            ctime[i][4] = ctime[i][0] + ctime[i][1];
            MPI_Allreduce(ctime[i], tmp_time, 5, MPI_DOUBLE, MPI_MAX,
                          MPI_COMM_WORLD);
            for (j = 0; j < 5; j++)
               tmax[j] += tmp_time[j];
            MPI_Allreduce(ctime[i], tmp_time, 5, MPI_DOUBLE, MPI_MIN,
                          MPI_COMM_WORLD);
            for (j = 0; j < 5; j++)
               tmin[j] += tmp_time[j];
         }
         last = ts;
      }
   }

   end_time = sim_time;
   num_tsteps = ts - 1;
   timer_all = timer() - t1;
}

// Calculate time step increment.  If an object intersects the unit cube
// then look at the rate of movement and size increase compared to the size
// of the smallest cell.  This does not exclude all cases where an object
// would not influence refinement.
double calc_time_step(void)
{
   int o, dir, done;
   double delta, tmp, inv_cell_size[3];
   object *op;

   if (use_tsteps) {
      // want end_time in num_tsteps timesteps
      delta = end_time/(double) num_tsteps;
   } else {
      // allow object boundary to move no more than one of the smallest cells
      inv_cell_size[0] = (double) (mesh_size[0]*x_block_size);
      inv_cell_size[1] = (double) (mesh_size[1]*y_block_size);
      inv_cell_size[2] = (double) (mesh_size[2]*z_block_size);
      delta = 0.0;
      for (o = 0; o < num_objects; o++) {
         op = &objects[o];
         if (op->size[0] < 0.0 || op->size[1] < 0.0 || op->size[2] < 0)
            break;
         for (done = dir = 0; dir < 3; dir++)
            if (op->cen[dir] < 0.0) {
               if (op->cen[dir] + op->size[dir] < 0.0)
                  done = 1;
            } else if (op->cen[dir] > 1.0) {
               if (op->cen[dir] - op->size[dir] > 1.0)
                  done = 1;
            }
         if (done)
            break;
         for (dir = 0; dir < 3; dir++) {
            tmp = (fabs(op->move[dir]) + fabs(op->inc[dir]))*inv_cell_size[dir];
            if (tmp > delta)
               delta = tmp;
         }
      }

      if (delta > 0.0)
         delta = 1.0/delta;
      else
         delta = 1.0;
   }

   return delta;
}
