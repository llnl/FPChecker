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

#ifdef MA_MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN double average[142];
EXTERN double stddev[139];
EXTERN double minimum[139];
EXTERN double maximum[139];

EXTERN double timer_all;

EXTERN double timer_comm_all;
EXTERN double timer_comm_dir[3];
EXTERN double timer_comm_recv[3];
EXTERN double timer_comm_pack[3];
EXTERN double timer_comm_send[3];
EXTERN double timer_comm_same[3];
EXTERN double timer_comm_diff[3];
EXTERN double timer_comm_bc[3];
EXTERN double timer_comm_wait[3];
EXTERN double timer_comm_unpack[3];

EXTERN double timer_calc_all;

EXTERN double timer_cs_all;
EXTERN double timer_cs_red;
EXTERN double timer_cs_calc;

EXTERN double timer_refine_all;
EXTERN double timer_refine_co;
EXTERN double timer_refine_mr;
EXTERN double timer_refine_cc;
EXTERN double timer_refine_sb;
EXTERN double timer_refine_c1;
EXTERN double timer_refine_c2;
EXTERN double timer_refine_sy;
EXTERN double timer_refine_init;
EXTERN double timer_cb_all;
EXTERN double timer_cb_cb;
EXTERN double timer_cb_pa;
EXTERN double timer_cb_mv;
EXTERN double timer_cb_un;
EXTERN double timer_lb_all;
EXTERN double timer_lb_sort;
EXTERN double timer_lb_pa;
EXTERN double timer_lb_mv;
EXTERN double timer_lb_un;
EXTERN double timer_lb_misc;
EXTERN double timer_lb_mb;
EXTERN double timer_lb_ma;
EXTERN double timer_rs_all;
EXTERN double timer_rs_ca;
EXTERN double timer_rs_pa;
EXTERN double timer_rs_mv;
EXTERN double timer_rs_un;
EXTERN double timer_group;

EXTERN double timer_main;
EXTERN double timer_init;
EXTERN double timer_plot;

EXTERN long long total_blocks;
EXTERN num_sz nb_min;
EXTERN num_sz nb_max;
EXTERN int nrrs;
EXTERN int nrs;
EXTERN int nps;
EXTERN int nlbs;
EXTERN int num_refined;
EXTERN int num_reformed;
EXTERN int num_moved_all;
EXTERN int num_moved_lb;
EXTERN int num_moved_rs;
EXTERN int num_moved_coarsen;
EXTERN int num_comm_x;
EXTERN int num_comm_y;
EXTERN int num_comm_z;
EXTERN int num_comm_tot;
EXTERN int num_comm_uniq;
EXTERN int num_comm_x_min;
EXTERN int num_comm_y_min;
EXTERN int num_comm_z_min;
EXTERN int num_comm_t_min;
EXTERN int num_comm_u_min;
EXTERN int num_comm_x_max;
EXTERN int num_comm_y_max;
EXTERN int num_comm_z_max;
EXTERN int num_comm_t_max;
EXTERN int num_comm_u_max;
EXTERN int counter_halo_recv[3];
EXTERN int counter_halo_send[3];
EXTERN double size_mesg_recv[3];
EXTERN double size_mesg_send[3];
EXTERN int counter_face_recv[3];
EXTERN int counter_face_send[3];
EXTERN int counter_bc[3];
EXTERN int counter_same[3];
EXTERN int counter_diff[3];
EXTERN int counter_malloc;
EXTERN double size_malloc;
EXTERN int counter_malloc_init;
EXTERN double size_malloc_init;
EXTERN int total_red;
EXTERN int num_over;
EXTERN int tot_over;
EXTERN int max_groups;
EXTERN double tmax[5], tmin[5];
