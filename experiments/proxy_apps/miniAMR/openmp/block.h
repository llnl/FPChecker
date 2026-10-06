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

//typedef int num_sz;
typedef long long num_sz;

typedef struct {
   num_sz number;
   num_sz num_prime;
   int level;
   int refine;
   int new_proc;
   int b_type;
   num_sz parent;       // if original block -1,
                     // else if on node, number in structure
                     // else (-2 - parent->number)
   int parent_node;
   int child_number;
   int nei_refine[6];
   int nei_level[6];  /* 0 to 5 = W, E, S, N, D, U; use -2 for boundary */
   int nei[6][2][2];  /* negative if off processor (-1 - proc) */
   int cen[3];
   double ****array;
} block;
EXTERN block *blocks;

typedef struct {
   num_sz number;
   num_sz num_prime;
   int level;
   int b_type;
   num_sz parent;      // -1 if original block
   int parent_node;
   int child_number;
   int refine;
   num_sz child[8];    // n if on node, number if not
                    // if negative, then onnode child is a parent (-1 - n)
   int child_node[8];
   int cen[3];
} parent;
EXTERN parent *parents;

typedef struct {
   num_sz number;     // number of block
   int n;          // position in block array
} sorted_block;
EXTERN sorted_block *sorted_list;
EXTERN int *sorted_index;

EXTERN int my_pe;
EXTERN int num_pes;

EXTERN int max_num_blocks;
EXTERN int num_refine;
EXTERN int uniform_refine;
EXTERN int x_block_size, y_block_size, z_block_size;
EXTERN int num_vars;
EXTERN int comm_vars;
EXTERN int init_block_x, init_block_y, init_block_z;
EXTERN int reorder;
EXTERN int npx, npy, npz;
EXTERN int inbalance;
EXTERN int refine_freq;
EXTERN int report_diffusion;
EXTERN int error_tol;
EXTERN int use_tsteps;
EXTERN int num_tsteps;
EXTERN int use_time;
EXTERN double end_time;
EXTERN int stages_per_ts;
EXTERN int checksum_freq;
EXTERN int stencil;
EXTERN int report_perf;
EXTERN int plot_freq;
EXTERN int num_objects;
EXTERN int lb_opt;
EXTERN int block_change;
EXTERN int code;
EXTERN int permute;
EXTERN int nonblocking;
EXTERN int refine_ghost;
EXTERN int change_dir;
EXTERN int group_blocks;
EXTERN int limit_move;
EXTERN int send_faces;
EXTERN int lb_method;

EXTERN int init_x, init_y, init_z;
EXTERN int first;
EXTERN int *dirs;
EXTERN int num_cells;
EXTERN int mat;
EXTERN int max_num_parents;
EXTERN int num_parents;
EXTERN int max_active_parent;
EXTERN int cur_max_level;
EXTERN num_sz *num_blocks;
EXTERN num_sz *local_num_blocks;
EXTERN num_sz *block_start;
EXTERN int num_active;
EXTERN int max_active_block;
EXTERN num_sz global_active;
EXTERN int x_block_half, y_block_half, z_block_half;
EXTERN double tol;
EXTERN double *grid_sum;
EXTERN num_sz *p8;
EXTERN int *p2;
EXTERN int mesh_size[3];
EXTERN int max_mesh_size;
EXTERN int *from, *to;
EXTERN int msg_len[3][4];
EXTERN int local_max_b;
EXTERN int global_max_b;
EXTERN double *a0, a1;
EXTERN double total_fp_divs;
EXTERN double total_fp_adds;
EXTERN double total_fp_muls;

typedef struct {
   int type;
   int bounce;
   double cen[3];
   double orig_cen[3];
   double move[3];
   double orig_move[3];
   double size[3];
   double orig_size[3];
   double inc[3];
} object;
EXTERN object *objects;

EXTERN int num_dots;
EXTERN int max_num_dots;
EXTERN int max_active_dot;
EXTERN int max_dots_used;
EXTERN num_sz total_dots_used;

typedef struct {
   num_sz number;
   int n;
   int proc;
   int new_proc;
   int cen[3];
} dot;
EXTERN dot *dots;

typedef struct {
   num_sz number;
   num_sz num_prime;
   int n;
   int proc;
   int new_proc;
} spot;
EXTERN spot *spots;
