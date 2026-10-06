/*BHEADER**********************************************************************
 * Copyright (c) 2017,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Ulrike Yang (yang11@llnl.gov) et al. CODE-LLNL-738-322.
 * This file is part of AMG.  See files README and COPYRIGHT for details.
 *
 * AMG is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTIBILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the
 * GNU General Public License for more details.
 *
 ***********************************************************************EHEADER*/

/*--------------------------------------------------------------------------
 * Test driver for unstructured matrix interface (IJ_matrix interface).
 * Do `driver -help' for usage info.
 * This driver started from the driver for parcsr_linear_solvers, and it
 * works by first building a parcsr matrix as before and then "copying"
 * that matrix row-by-row into the IJMatrix interface. AJC 7/99.
 *--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "_hypre_utilities.h"
#include "HYPRE.h"
#include "HYPRE_parcsr_mv.h"

#include "HYPRE_IJ_mv.h"
#include "HYPRE_parcsr_ls.h"
#include "_hypre_parcsr_mv.h"
#include "HYPRE_krylov.h"

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

HYPRE_Int BuildIJLaplacian27pt (HYPRE_Int argc , char *argv [], HYPRE_Real *size , HYPRE_IJMatrix *A_ptr );
HYPRE_Int AddOrRestoreAIJ( HYPRE_IJMatrix  ij_A, HYPRE_Real eps, HYPRE_Int action  );
HYPRE_Int hypre_map27( HYPRE_Int  ix, HYPRE_Int  iy, HYPRE_Int  iz,
      HYPRE_Int  px, HYPRE_Int  py, HYPRE_Int  pz,
      HYPRE_Int  Cx, HYPRE_Int  Cy, HYPRE_Int  Cz, HYPRE_Int nx, HYPRE_Int nxy);

#ifdef __cplusplus
}
#endif
#define SECOND_TIME 0
 
hypre_int
main( hypre_int argc,
      char *argv[] )
{
   HYPRE_Int           arg_index;
   HYPRE_Int           print_usage;
   HYPRE_Int           build_rhs_type;
   HYPRE_Int           build_rhs_arg_index;
   HYPRE_Int           build_src_type;
   HYPRE_Int           build_src_arg_index;
   HYPRE_Int           solver_id;
   HYPRE_Int           problem_id;
   HYPRE_Int           ioutdat;
   HYPRE_Int           poutdat;
   HYPRE_Int           debug_flag;
   HYPRE_Int           ierr = 0;
   HYPRE_Int           i, j; 
   HYPRE_Int           max_levels = 25;
   HYPRE_Int           num_iterations;
   HYPRE_Int           max_iter = 1000;
   HYPRE_Int           mg_max_iter = 100;
   HYPRE_Int 	       cum_num_its=0;
   HYPRE_Int           nodal = 0;
   HYPRE_Real          final_res_norm;
   void               *object;

   HYPRE_IJMatrix      ij_A; 
   HYPRE_IJVector      ij_b;
   HYPRE_IJVector      ij_x;

   HYPRE_ParCSRMatrix  parcsr_A;
   HYPRE_ParVector     b;
   HYPRE_ParVector     x;

   HYPRE_Solver        amg_solver;
   HYPRE_Solver        pcg_solver;
   HYPRE_Solver        pcg_precond=NULL, pcg_precond_gotten;

   HYPRE_Int           myid;
   HYPRE_Int          *dof_func;
   HYPRE_Int	       num_functions = 1;
   HYPRE_Int	       num_paths = 1;
   HYPRE_Int	       agg_num_levels = 1;
   HYPRE_Int	       ns_coarse = 1;

   HYPRE_Int	       time_index;
   MPI_Comm            comm = hypre_MPI_COMM_WORLD;
   HYPRE_Int first_local_row, last_local_row, local_num_rows;
   HYPRE_Int first_local_col, last_local_col, local_num_cols;

   HYPRE_Int time_steps = 6;

   /* parameters for BoomerAMG */
   HYPRE_Int    P_max_elmts = 8;
   HYPRE_Int    cycle_type;
   HYPRE_Int    coarsen_type = 8;
   HYPRE_Int    measure_type = 0;
   HYPRE_Int    num_sweeps = 2;  
   HYPRE_Int    relax_type = 18;   
   HYPRE_Int    rap2=1;
   HYPRE_Int    keepTranspose = 0;
   HYPRE_Real   tol = 1.e-8, pc_tol = 0.;
   HYPRE_Real   atol = 0.0;

   HYPRE_Real   wall_time;
   HYPRE_Real   system_size;
   HYPRE_Real   cum_nnz_AP = 0;
   HYPRE_Real   nnz_AP;
   HYPRE_Real   FOM1, FOM2;

   /* parameters for GMRES */
   HYPRE_Int	    k_dim = 20;
   /* interpolation */
   HYPRE_Int      interp_type  = 6; /* default value */
   /* aggressive coarsening */
   HYPRE_Int      agg_interp_type  = 4; /* default value */

   HYPRE_Int      print_system = 0;
   HYPRE_Int      print_stats = 0;

   HYPRE_Int rel_change = 0;

   HYPRE_Real *values;

   /*-----------------------------------------------------------
    * Initialize some stuff
    *-----------------------------------------------------------*/

   /* Initialize MPI */
   hypre_MPI_Init(&argc, &argv);

   hypre_MPI_Comm_rank(hypre_MPI_COMM_WORLD, &myid );

   /*-----------------------------------------------------------
    * Set defaults
    *-----------------------------------------------------------*/
 
   build_rhs_type = 3;
   build_rhs_arg_index = argc;
   build_src_type = -1;
   build_src_arg_index = argc;
   debug_flag = 0;

   solver_id = 1;
   problem_id = 1;

   ioutdat = 0;
   poutdat = 0;

   /*-----------------------------------------------------------
    * Parse command line
    *-----------------------------------------------------------*/
 
   print_usage = 0;
   arg_index = 1;

   while ( (arg_index < argc) && (!print_usage) )
   {
      if ( strcmp(argv[arg_index], "-help") == 0 )
      {
         print_usage = 1;
      }
      else
      {
         arg_index++;
      }
   }

   /* defaults for GMRES */

   k_dim = 20;

   arg_index = 0;
   while (arg_index < argc)
   {
      if ( strcmp(argv[arg_index], "-problem") == 0 )
      {
         arg_index++;
         problem_id = atoi(argv[arg_index++]);
         if (problem_id == 2) 
	 {
	    solver_id = 3;
	 }
         
      }
      else if ( strcmp(argv[arg_index], "-printstats") == 0 )
      {
         arg_index++;
         ioutdat  = 1;
         poutdat  = 1;
         print_stats  = 1;
      }
      else if ( strcmp(argv[arg_index], "-printallstats") == 0 )
      {
         arg_index++;
         ioutdat  = 3;
         poutdat  = 1;
         print_stats  = 1;
      }
      else if ( strcmp(argv[arg_index], "-print") == 0 )
      {
         arg_index++;
         print_system = 1;
      }
      else if ( strcmp(argv[arg_index], "-keepT") == 0 )
      {
         arg_index++;
         keepTranspose = 1;
         rap2 = 0;
      }
      else
      {
         arg_index++;
      }
   }

   /*-----------------------------------------------------------
    * Print usage info
    *-----------------------------------------------------------*/
 
   if ( (print_usage) && (myid == 0) )
   {
      hypre_printf("\n");
      hypre_printf("Usage: %s [<options>]\n", argv[0]);
      hypre_printf("\n");
      hypre_printf("  -problem <ID>: problem ID\n");
      hypre_printf("       1 = solves 1 large problem with AMG-PCG (default) \n");
      hypre_printf("       2 = simulates a time-dependent loop with AMG-GMRES\n");
      hypre_printf("\n");
      hypre_printf("  -n <nx> <ny> <nz>: problem size per MPI process (default: nx=ny=nz=10)\n");
      hypre_printf("\n");
      hypre_printf("  -P <px> <py> <pz>: processor topology (default: px=py=pz=1)\n");
      hypre_printf("\n");
      hypre_printf("  -print       : prints the system\n");
      hypre_printf("  -printstats  : prints preconditioning and convergence stats\n");
      hypre_printf("  -printallstats  : prints preconditioning and convergence stats\n");
      hypre_printf("                    including residual norms for each iteration\n");
      hypre_printf("\n"); 
      exit(1);
   }

   /*-----------------------------------------------------------
    * Print driver parameters
    *-----------------------------------------------------------*/
 
   if (myid == 0)
   {
      hypre_printf("Running with these driver parameters:\n");
      hypre_printf("  solver ID    = %d\n\n", solver_id);
   }

   /*-----------------------------------------------------------
    * Set up matrix
    *-----------------------------------------------------------*/

   time_index = hypre_InitializeTiming("Spatial Operator");
   hypre_BeginTiming(time_index);

   BuildIJLaplacian27pt(argc, argv, &system_size, &ij_A);
   HYPRE_IJMatrixGetObject(ij_A, &object);
   parcsr_A = (HYPRE_ParCSRMatrix) object;


   hypre_EndTiming(time_index);
   hypre_PrintTiming("Generate Matrix", &wall_time, hypre_MPI_COMM_WORLD);
   hypre_FinalizeTiming(time_index);
   hypre_ClearTiming();

   /*-----------------------------------------------------------
    * Set up the RHS and initial guess
    *-----------------------------------------------------------*/

   time_index = hypre_InitializeTiming("RHS and Initial Guess");
   hypre_BeginTiming(time_index);

   ierr = HYPRE_ParCSRMatrixGetLocalRange( parcsr_A,
                                              &first_local_row, &last_local_row ,
                                              &first_local_col, &last_local_col );

   local_num_rows = last_local_row - first_local_row + 1;
   local_num_cols = last_local_col - first_local_col + 1;

   if (myid == 0)
   {
      hypre_printf("  RHS vector has unit components\n");
      hypre_printf("  Initial guess is 0\n");
   }

   /* RHS */
   HYPRE_IJVectorCreate(hypre_MPI_COMM_WORLD, first_local_row, last_local_row, &ij_b);
   HYPRE_IJVectorSetObjectType(ij_b, HYPRE_PARCSR);
   HYPRE_IJVectorInitialize(ij_b);

   /* Initial guess */
   HYPRE_IJVectorCreate(hypre_MPI_COMM_WORLD, first_local_col, last_local_col, &ij_x);
   HYPRE_IJVectorSetObjectType(ij_x, HYPRE_PARCSR);
   HYPRE_IJVectorInitialize(ij_x);

   values = hypre_CTAlloc(HYPRE_Real, local_num_rows);

   HYPRE_IJVectorSetValues(ij_x, local_num_rows, NULL, values);

   for (i = 0; i < local_num_rows; i++)
      values[i] = 1.0;

   HYPRE_IJVectorSetValues(ij_b, local_num_rows, NULL, values);
   hypre_TFree(values);

   ierr = HYPRE_IJVectorGetObject( ij_b, &object );
   b = (HYPRE_ParVector) object;

   ierr = HYPRE_IJVectorGetObject( ij_x, &object );
   x = (HYPRE_ParVector) object;

   hypre_EndTiming(time_index);
   hypre_PrintTiming("IJ Vector Setup", &wall_time, hypre_MPI_COMM_WORLD);
   hypre_FinalizeTiming(time_index);
   hypre_ClearTiming();
   
   /*-----------------------------------------------------------
    * Print out the system and initial guess
    *-----------------------------------------------------------*/

   if (print_system)
   {
      HYPRE_IJMatrixPrint(ij_A, "IJ.out.A");
      HYPRE_IJVectorPrint(ij_b, "IJ.out.b");
      HYPRE_IJVectorPrint(ij_x, "IJ.out.x0");

   }

   /*-----------------------------------------------------------
    * Problem 1: Solve one large problem with AMG-PCG
    *-----------------------------------------------------------*/


   if (problem_id == 1 )
   {
      time_index = hypre_InitializeTiming("PCG Setup");
      hypre_MPI_Barrier(hypre_MPI_COMM_WORLD);
      hypre_BeginTiming(time_index);
 
      HYPRE_ParCSRPCGCreate(hypre_MPI_COMM_WORLD, &pcg_solver);
      HYPRE_PCGSetMaxIter(pcg_solver, max_iter);
      HYPRE_PCGSetTol(pcg_solver, tol);
      HYPRE_PCGSetTwoNorm(pcg_solver, 1);
      HYPRE_PCGSetRelChange(pcg_solver, rel_change);
      HYPRE_PCGSetPrintLevel(pcg_solver, ioutdat);
      HYPRE_PCGSetAbsoluteTol(pcg_solver, atol);

      /* use BoomerAMG as preconditioner */
      if (myid == 0 && print_stats) hypre_printf("Solver: AMG-PCG\n");
      HYPRE_BoomerAMGCreate(&pcg_precond); 
      HYPRE_BoomerAMGSetTol(pcg_precond, pc_tol);
      HYPRE_BoomerAMGSetCoarsenType(pcg_precond, coarsen_type);
      HYPRE_BoomerAMGSetPMaxElmts(pcg_precond, P_max_elmts);
      HYPRE_BoomerAMGSetPrintLevel(pcg_precond, poutdat);
      HYPRE_BoomerAMGSetMaxIter(pcg_precond, 1);
      HYPRE_BoomerAMGSetNumSweeps(pcg_precond, num_sweeps);
      if (relax_type > -1) HYPRE_BoomerAMGSetRelaxType(pcg_precond, relax_type);
      HYPRE_BoomerAMGSetDebugFlag(pcg_precond, debug_flag);
      HYPRE_BoomerAMGSetAggNumLevels(pcg_precond, agg_num_levels);
      HYPRE_BoomerAMGSetRAP2(pcg_precond, rap2);
      HYPRE_BoomerAMGSetKeepTranspose(pcg_precond, keepTranspose);
      HYPRE_PCGSetMaxIter(pcg_solver, mg_max_iter);
      HYPRE_PCGSetPrecond(pcg_solver,
                             (HYPRE_PtrToSolverFcn) HYPRE_BoomerAMGSolve,
                             (HYPRE_PtrToSolverFcn) HYPRE_BoomerAMGSetup,
                             pcg_precond);
 
      HYPRE_PCGGetPrecond(pcg_solver, &pcg_precond_gotten);
      if (pcg_precond_gotten !=  pcg_precond)
      {
         hypre_printf("HYPRE_ParCSRPCGGetPrecond got bad precond\n");
         return(-1);
      }
      else 
         if (myid == 0 && print_stats)
            hypre_printf("HYPRE_ParCSRPCGGetPrecond got good precond\n");

      HYPRE_PCGSetup(pcg_solver, (HYPRE_Matrix)parcsr_A, 
                     (HYPRE_Vector)b, (HYPRE_Vector)x);

      hypre_MPI_Barrier(hypre_MPI_COMM_WORLD);
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Problem 1: AMG Setup Time", &wall_time, hypre_MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();
      fflush(NULL);

      HYPRE_BoomerAMGGetCumNnzAP(pcg_precond, &cum_nnz_AP);

      FOM1 = cum_nnz_AP/ wall_time;

      if (myid == 0)
            printf ("\nFOM_Setup: nnz_AP / Setup Phase Time: %e\n\n", FOM1);
   
      time_index = hypre_InitializeTiming("PCG Solve");
      hypre_MPI_Barrier(hypre_MPI_COMM_WORLD);
      hypre_BeginTiming(time_index);
 
      HYPRE_PCGSolve(pcg_solver, (HYPRE_Matrix)parcsr_A, 
                     (HYPRE_Vector)b, (HYPRE_Vector)x);
 
      hypre_MPI_Barrier(hypre_MPI_COMM_WORLD);
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Problem 1: AMG-PCG Solve Time", &wall_time, hypre_MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();
      fflush(NULL);
 
      HYPRE_PCGGetNumIterations(pcg_solver, &num_iterations);
      HYPRE_PCGGetFinalRelativeResidualNorm(pcg_solver, &final_res_norm);

   
      HYPRE_BoomerAMGDestroy(pcg_precond);

      HYPRE_ParCSRPCGDestroy(pcg_solver);

      FOM2 = cum_nnz_AP*(HYPRE_Real)num_iterations/ wall_time;

      if (myid == 0)
      {
         hypre_printf("\n");
         hypre_printf("Iterations = %d\n", num_iterations);
         hypre_printf("Final Relative Residual Norm = %e\n", final_res_norm);
         hypre_printf("\n");
         printf ("\nFOM_Solve: nnz_AP * Iterations / Solve Phase Time: %e\n\n", FOM2);
         FOM1 += 3.0*FOM2;
         FOM1 /= 4.0;
         printf ("\n\nFigure of Merit (FOM_1): %e\n\n", FOM1);
      }
 
   }

   /*-----------------------------------------------------------
    * Problem 2: simulate time-dependent problem AMG-GMRES
    *-----------------------------------------------------------*/

   if (problem_id == 2)
   {
      HYPRE_Real eps;
      HYPRE_Real diagonal = 26.5;
      AddOrRestoreAIJ(ij_A, diagonal, 0);
      time_index = hypre_InitializeTiming("GMRES Solve");
      hypre_MPI_Barrier(hypre_MPI_COMM_WORLD);
      hypre_BeginTiming(time_index);
      for (j=0; j < time_steps; j++)
      {
 
         HYPRE_ParCSRGMRESCreate(hypre_MPI_COMM_WORLD, &pcg_solver);
         HYPRE_GMRESSetKDim(pcg_solver, k_dim);
         HYPRE_GMRESSetMaxIter(pcg_solver, max_iter);
         HYPRE_GMRESSetTol(pcg_solver, tol);
         HYPRE_GMRESSetAbsoluteTol(pcg_solver, atol);
         HYPRE_GMRESSetLogging(pcg_solver, 1);
         HYPRE_GMRESSetPrintLevel(pcg_solver, ioutdat);
         HYPRE_GMRESSetRelChange(pcg_solver, rel_change);
 
         /* use BoomerAMG as preconditioner */
         if (myid == 0 && print_stats) hypre_printf("Solver: AMG-GMRES\n");

         HYPRE_BoomerAMGCreate(&pcg_precond); 
         HYPRE_BoomerAMGSetTol(pcg_precond, pc_tol);
         HYPRE_BoomerAMGSetCoarsenType(pcg_precond, coarsen_type);
         HYPRE_BoomerAMGSetPMaxElmts(pcg_precond, P_max_elmts);
         HYPRE_BoomerAMGSetPrintLevel(pcg_precond, poutdat);
         HYPRE_BoomerAMGSetMaxIter(pcg_precond, 1);
         HYPRE_BoomerAMGSetNumSweeps(pcg_precond, num_sweeps);
         if (relax_type > -1) HYPRE_BoomerAMGSetRelaxType(pcg_precond, relax_type);
         HYPRE_BoomerAMGSetDebugFlag(pcg_precond, debug_flag);
         HYPRE_BoomerAMGSetAggNumLevels(pcg_precond, agg_num_levels);
         HYPRE_BoomerAMGSetRAP2(pcg_precond, rap2);
         HYPRE_BoomerAMGSetKeepTranspose(pcg_precond, keepTranspose);
         HYPRE_GMRESSetMaxIter(pcg_solver, mg_max_iter);
         HYPRE_GMRESSetPrecond(pcg_solver,
                               (HYPRE_PtrToSolverFcn) HYPRE_BoomerAMGSolve,
                               (HYPRE_PtrToSolverFcn) HYPRE_BoomerAMGSetup,
                               pcg_precond);

         HYPRE_GMRESGetPrecond(pcg_solver, &pcg_precond_gotten);
         if (pcg_precond_gotten != pcg_precond)
         {
            hypre_printf("HYPRE_GMRESGetPrecond got bad precond\n");
            return(-1);
         }
         else
            if (myid == 0 && print_stats)
               hypre_printf("HYPRE_GMRESGetPrecond got good precond\n");
         HYPRE_GMRESSetup (pcg_solver, (HYPRE_Matrix)parcsr_A, (HYPRE_Vector)b, (HYPRE_Vector)x);
         HYPRE_BoomerAMGGetCumNnzAP(pcg_precond, &nnz_AP);
         cum_nnz_AP += nnz_AP;
 
         HYPRE_GMRESSolve (pcg_solver, (HYPRE_Matrix)parcsr_A, (HYPRE_Vector)b, (HYPRE_Vector)x);
 
         HYPRE_GMRESGetNumIterations(pcg_solver, &num_iterations);
         HYPRE_GMRESGetFinalRelativeResidualNorm(pcg_solver,&final_res_norm);
         if (myid == 0 && print_stats)
         {
            hypre_printf("\n");
            hypre_printf("GMRES Iterations = %d\n", num_iterations);
            hypre_printf("Final GMRES Relative Residual Norm = %e\n", final_res_norm);
            hypre_printf("\n");
         }
         cum_num_its += num_iterations;
      
         for (i=0; i < 4; i++)
         {
            HYPRE_Int gmres_iter = 6-i;
            eps = 0.01;
            AddOrRestoreAIJ(ij_A, eps, 1);
            HYPRE_ParVectorAxpy(eps,x,b);
            HYPRE_GMRESSetMaxIter(pcg_solver, gmres_iter);
            HYPRE_GMRESSetTol(pcg_solver, 0.0);
            HYPRE_GMRESSolve(pcg_solver, (HYPRE_Matrix)parcsr_A, (HYPRE_Vector)b, (HYPRE_Vector)x);
            HYPRE_GMRESGetNumIterations(pcg_solver, &num_iterations);
            HYPRE_GMRESGetFinalRelativeResidualNorm(pcg_solver,&final_res_norm);
            if (myid == 0 && print_stats)
            {
               hypre_printf("\n");
               hypre_printf("GMRES Iterations = %d\n", num_iterations);
               hypre_printf("Final GMRES Relative Residual Norm = %e\n", final_res_norm);
               hypre_printf("\n");
            }
            cum_num_its += num_iterations;
         }
 
         HYPRE_BoomerAMGDestroy(pcg_precond);

         HYPRE_ParCSRGMRESDestroy(pcg_solver);
 
         if (j < time_steps) 
         {
            diagonal -= 0.1 ;
            AddOrRestoreAIJ(ij_A, diagonal, 0);
            HYPRE_ParVectorSetConstantValues(x,0.0);
            HYPRE_ParVectorSetConstantValues(b,1.0);
         }
      }
      hypre_MPI_Barrier(hypre_MPI_COMM_WORLD);
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Problem 2: Cumulative AMG-GMRES Solve Time", &wall_time, hypre_MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();
      if (myid == 0)
      {
         hypre_printf("\n");
         hypre_printf("No. of Time Steps = %d\n", time_steps);
         hypre_printf("Cum. No. of Iterations = %d\n", cum_num_its);
         hypre_printf("Final Relative Residual Norm = %e\n", final_res_norm);
         hypre_printf("\n");
         cum_nnz_AP /= (HYPRE_Real)time_steps;
         printf ("\nnnz AP * (Iterations + time_steps) / Total Time: \n");
         printf ("\nFigure of Merit (FOM_2): %e\n\n", (cum_nnz_AP*(HYPRE_Real)(cum_num_its +time_steps)/ wall_time));
         hypre_printf("\n");
      }
   }
 

   /*-----------------------------------------------------------
    * Print the solution
    *-----------------------------------------------------------*/

   if (print_system)
   {
      HYPRE_IJVectorPrint(ij_x, "IJ.out.x");
   }

   /*-----------------------------------------------------------
    * Finalize things
    *-----------------------------------------------------------*/

   HYPRE_IJMatrixDestroy(ij_A);

   HYPRE_IJVectorDestroy(ij_b);

   HYPRE_IJVectorDestroy(ij_x);

/*
  hypre_FinalizeMemoryDebug();
*/
   hypre_MPI_Finalize();

   return (0);
}

/*----------------------------------------------------------------------
 * Build 27-point laplacian in 3D,
 * Parameters given in command line.
 *----------------------------------------------------------------------*/

HYPRE_Int
BuildIJLaplacian27pt( HYPRE_Int         argc,
                       char            *argv[],
                       HYPRE_Real      *system_size_ptr,
                       HYPRE_IJMatrix  *ij_A_ptr     )
{
   MPI_Comm            comm = hypre_MPI_COMM_WORLD;
   HYPRE_Int                 nx, ny, nz;
   HYPRE_Int                 P, Q, R;

   HYPRE_IJMatrix  ij_A;

   HYPRE_Int                 num_procs, myid;
   HYPRE_Int                 px, py, pz;
   HYPRE_Real         *value;
   HYPRE_Int    *diag_i;
   HYPRE_Int    *offd_i;
   HYPRE_Int    *row_nums;
   HYPRE_Int    *col_nums;
   HYPRE_Int    *num_cols;
   HYPRE_Real *data;

   HYPRE_Int row_index;
   HYPRE_Int i;
   HYPRE_Int local_size;
   HYPRE_Real global_size;
   HYPRE_Int first_local_row;
   HYPRE_Int last_local_row;
   HYPRE_Int first_local_col;
   HYPRE_Int last_local_col;

   HYPRE_Int nxy;
   HYPRE_Int nx_global, ny_global, nz_global;
   HYPRE_Int all_threads;
   HYPRE_Int *nnz;
   HYPRE_Int Cx, Cy, Cz;
   HYPRE_Int arg_index;

   /*-----------------------------------------------------------
    * Initialize some stuff
    *-----------------------------------------------------------*/

   hypre_MPI_Comm_size(comm, &num_procs );
   hypre_MPI_Comm_rank(comm, &myid );
   all_threads = hypre_NumThreads();
   nnz = hypre_CTAlloc(HYPRE_Int, all_threads);

   /*-----------------------------------------------------------
    * Set defaults
    *-----------------------------------------------------------*/
 
   nx = 10;
   ny = 10;
   nz = 10;

   P  = 1;
   Q  = num_procs;
   R  = 1;

   /*-----------------------------------------------------------
    * Parse command line
    *-----------------------------------------------------------*/
   arg_index = 0;
   while (arg_index < argc)
   {
      if ( strcmp(argv[arg_index], "-n") == 0 )
      {
         arg_index++;
         nx = atoi(argv[arg_index++]);
         ny = atoi(argv[arg_index++]);
         nz = atoi(argv[arg_index++]);
      }
      else if ( strcmp(argv[arg_index], "-P") == 0 )
      {
         arg_index++;
         P  = atoi(argv[arg_index++]);
         Q  = atoi(argv[arg_index++]);
         R  = atoi(argv[arg_index++]);
      }
      else
      {
         arg_index++;
      }
   }

   /*-----------------------------------------------------------
    * Check a few things
    *-----------------------------------------------------------*/

   if ((P*Q*R) != num_procs)
   {
      hypre_printf("Error: Invalid number of processors or processor topology \n");
      exit(1);
   }

   /*-----------------------------------------------------------
    * Print driver parameters
    *-----------------------------------------------------------*/

   nx_global = P*nx; 
   ny_global = Q*ny; 
   nz_global = R*nz; 
   global_size = (HYPRE_Real)(nx_global*ny_global*nz_global);
   if (myid == 0)

   {
      hypre_printf("  Laplacian_27pt:\n");
      hypre_printf("    (Nx, Ny, Nz) = (%d, %d, %d)\n", nx_global, ny_global, nz_global);
      hypre_printf("    (Px, Py, Pz) = (%d, %d, %d)\n\n", P,  Q,  R);
   }

   /*-----------------------------------------------------------
    * Set up the grid structure
    *-----------------------------------------------------------*/

   /* compute px,py,pz from P,Q,R and myid */
   px = myid % P;
   py = (( myid - px)/P) % Q;
   pz = ( myid - px - P*py)/( P*Q );

   /*-----------------------------------------------------------
    * Generate the matrix 
    *-----------------------------------------------------------*/
 
   value = hypre_CTAlloc(HYPRE_Real, 2);

   value[0] = 26.0;
   if (nx == 1 || ny == 1 || nz == 1)
      value[0] = 8.0;
   if (nx*ny == 1 || nx*nz == 1 || ny*nz == 1)
      value[0] = 2.0;
   value[1] = -1.;

   local_size = nx*ny*nz;

   row_nums = hypre_CTAlloc(HYPRE_Int, local_size);
   num_cols = hypre_CTAlloc(HYPRE_Int, local_size);
   row_index = myid*local_size;

   HYPRE_IJMatrixCreate( comm, row_index, row_index+local_size-1,
                               row_index, row_index+local_size-1,
                               &ij_A );

   HYPRE_IJMatrixSetObjectType( ij_A, HYPRE_PARCSR );

   nxy = nx*ny;

   diag_i = hypre_CTAlloc(HYPRE_Int, local_size);
   offd_i = hypre_CTAlloc(HYPRE_Int, local_size);

   Cx = nx*(ny*nz-1);
   Cy = nxy*(P*nz-1);
   Cz = local_size*(P*Q-1);
#ifdef HYPRE_USING_OPENMP
#pragma omp parallel
#endif
   {
    HYPRE_Int ix, iy, iz;
    HYPRE_Int cnt, o_cnt;
    HYPRE_Int ix_start, ix_end;
    HYPRE_Int iy_start, iy_end;
    HYPRE_Int iz_start, iz_end;
    HYPRE_Int num_threads, my_thread;
    HYPRE_Int all_nnz=0;
    HYPRE_Int size, rest;
    HYPRE_Int new_row_index;
    num_threads = hypre_NumActiveThreads();
    my_thread = hypre_GetThreadNum();
    size = nz/num_threads;
    rest = nz - size*num_threads;
    ix_start = nx*px;
    ix_end = ix_start+nx;
    iy_start = ny*py;
    iy_end = iy_start+ny;
    if (my_thread < rest)
    {
       iz_start = nz*pz + my_thread*size+my_thread;
       iz_end = nz*pz + (my_thread+1)*size+my_thread+1;
       cnt = (my_thread*size+my_thread)*nxy-1;
    }
    else
    {
       iz_start = nz*pz + my_thread*size+rest;
       iz_end = nz*pz + (my_thread+1)*size+rest;
       cnt = (my_thread*size+rest)*nxy-1;
    }
    o_cnt = cnt;

    for (iz = iz_start;  iz < iz_end; iz++)
    {
      for (iy = iy_start;  iy < iy_end; iy++)
      {
         for (ix = ix_start; ix < ix_end; ix++)
         {
            cnt++;
            o_cnt++;
            diag_i[cnt]++;
            if (iz > nz*pz) 
            {
               diag_i[cnt]++;
               if (iy > ny*py) 
               {
                  diag_i[cnt]++;
      	          if (ix > nx*px)
      	          {
      	             diag_i[cnt]++;
      	          }
      	          else
      	          {
      	             if (ix) 
      		        offd_i[o_cnt]++;
      	          }
      	          if (ix < nx*(px+1)-1)
      	          {
      	             diag_i[cnt]++;
      	          }
      	          else
      	          {
      	             if (ix+1 < nx_global) 
      		        offd_i[o_cnt]++;
      	          }
               }
               else
               {
                  if (iy) 
                  {
                     offd_i[o_cnt]++;
      	             if (ix > nx*px)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else if (ix)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             if (ix < nx*(px+1)-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else if (ix < nx_global-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
                  }
               }
               if (ix > nx*px) 
                  diag_i[cnt]++;
               else
               {
                  if (ix) 
                  {
                     offd_i[o_cnt]++;
                  }
               }
               if (ix+1 < nx*(px+1)) 
                  diag_i[cnt]++;
               else
               {
                  if (ix+1 < nx_global) 
                  {
                     offd_i[o_cnt]++; 
                  }
               }
               if (iy+1 < ny*(py+1)) 
               {
                  diag_i[cnt]++;
      	          if (ix > nx*px)
      	          {
      	             diag_i[cnt]++;
      	          }
      	          else
      	          {
      	             if (ix) 
      		        offd_i[o_cnt]++;
      	          }
      	          if (ix < nx*(px+1)-1)
      	          {
      	             diag_i[cnt]++;
      	          }
      	          else
      	          {
      	             if (ix+1 < nx_global) 
      		        offd_i[o_cnt]++;
      	          }
               }
               else
               {
                  if (iy+1 < ny_global) 
                  {
                     offd_i[o_cnt]++;
      	             if (ix > nx*px)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else if (ix)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             if (ix < nx*(px+1)-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else if (ix < nx_global-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
                  }
               }
            }
            else
            {
               if (iz)
	       {
		  offd_i[o_cnt]++;
                  if (iy > ny*py) 
                  {
                     offd_i[o_cnt]++;
      	             if (ix > nx*px)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else
      	             {
      	                if (ix) 
      	   	        offd_i[o_cnt]++;
      	             }
      	             if (ix < nx*(px+1)-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else
      	             {
      	                if (ix+1 < nx_global) 
      	   	        offd_i[o_cnt]++;
      	             }
                  }
                  else
                  {
                     if (iy) 
                     {
                        offd_i[o_cnt]++;
      	                if (ix > nx*px)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                else if (ix)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                if (ix < nx*(px+1)-1)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                else if (ix < nx_global-1)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
                     }
                  }
                  if (ix > nx*px) 
                     offd_i[o_cnt]++;
                  else
                  {
                     if (ix) 
                     {
                        offd_i[o_cnt]++; 
                     }
                  }
                  if (ix+1 < nx*(px+1)) 
                     offd_i[o_cnt]++;
                  else
                  {
                     if (ix+1 < nx_global) 
                     {
                        offd_i[o_cnt]++; 
                     }
                  }
                  if (iy+1 < ny*(py+1)) 
                  {
                     offd_i[o_cnt]++;
      	             if (ix > nx*px)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else
      	             {
      	                if (ix) 
      	   	           offd_i[o_cnt]++;
      	             }
      	             if (ix < nx*(px+1)-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else
      	             {
      	                if (ix+1 < nx_global) 
      	   	           offd_i[o_cnt]++;
      	             }
                  }
                  else
                  {
                     if (iy+1 < ny_global) 
                     {
                        offd_i[o_cnt]++;
      	                if (ix > nx*px)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                else if (ix)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                if (ix < nx*(px+1)-1)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                else if (ix < nx_global-1)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
                     }
                  }
               }
            }
            if (iy > ny*py) 
            {
               diag_i[cnt]++;
   	       if (ix > nx*px)
   	       {
   	          diag_i[cnt]++;
   	       }
   	       else
   	       {
   	          if (ix) 
   		     offd_i[o_cnt]++;
   	       }
   	       if (ix < nx*(px+1)-1)
   	       {
   	          diag_i[cnt]++;
   	       }
   	       else
   	       {
   	          if (ix+1 < nx_global) 
   		     offd_i[o_cnt]++;
   	       }
            }
            else
            {
               if (iy) 
               {
                  offd_i[o_cnt]++;
   	          if (ix > nx*px)
   	          {
   	             offd_i[o_cnt]++;
   	          }
   	          else if (ix)
   	          {
   	             offd_i[o_cnt]++;
   	          }
   	          if (ix < nx*(px+1)-1)
   	          {
   	             offd_i[o_cnt]++;
   	          }
   	          else if (ix < nx_global-1)
   	          {
   	             offd_i[o_cnt]++;
   	          }
               }
            }
            if (ix > nx*px) 
               diag_i[cnt]++;
            else
            {
               if (ix) 
               {
                  offd_i[o_cnt]++; 
               }
            }
            if (ix+1 < nx*(px+1)) 
               diag_i[cnt]++;
            else
            {
               if (ix+1 < nx_global) 
               {
                  offd_i[o_cnt]++; 
               }
            }
            if (iy+1 < ny*(py+1)) 
            {
               diag_i[cnt]++;
   	       if (ix > nx*px)
   	       {
   	          diag_i[cnt]++;
   	       }
   	       else
   	       {
   	          if (ix) 
   		     offd_i[o_cnt]++;
   	       }
   	       if (ix < nx*(px+1)-1)
   	       {
   	          diag_i[cnt]++;
   	       }
   	       else
   	       {
   	          if (ix+1 < nx_global) 
   		     offd_i[o_cnt]++;
   	       }
            }
            else
            {
               if (iy+1 < ny_global) 
               {
                  offd_i[o_cnt]++;
   	          if (ix > nx*px)
   	          {
   	             offd_i[o_cnt]++;
   	          }
   	          else if (ix)
   	          {
   	             offd_i[o_cnt]++;
   	          }
   	          if (ix < nx*(px+1)-1)
   	          {
   	             offd_i[o_cnt]++;
   	          }
   	          else if (ix < nx_global-1)
   	          {
   	             offd_i[o_cnt]++;
   	          }
               }
            }
            if (iz+1 < nz*(pz+1)) 
            {
               diag_i[cnt]++;
               if (iy > ny*py) 
               {
                  diag_i[cnt]++;
      	          if (ix > nx*px)
      	          {
      	             diag_i[cnt]++;
      	          }
      	          else
      	          {
      	             if (ix) 
      		        offd_i[o_cnt]++;
      	          }
      	          if (ix < nx*(px+1)-1)
      	          {
      	             diag_i[cnt]++;
      	          }
      	          else
      	          {
      	             if (ix+1 < nx_global) 
      		        offd_i[o_cnt]++;
      	          }
               }
               else
               {
                  if (iy) 
                  {
                     offd_i[o_cnt]++;
      	             if (ix > nx*px)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else if (ix)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             if (ix < nx*(px+1)-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else if (ix < nx_global-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
                  }
               }
               if (ix > nx*px) 
                  diag_i[cnt]++;
               else
               {
                  if (ix) 
                  {
                     offd_i[o_cnt]++; 
                  }
               }
               if (ix+1 < nx*(px+1)) 
                  diag_i[cnt]++;
               else
               {
                  if (ix+1 < nx_global) 
                  {
                     offd_i[o_cnt]++; 
                  }
               }
               if (iy+1 < ny*(py+1)) 
               {
                  diag_i[cnt]++;
      	          if (ix > nx*px)
      	          {
      	             diag_i[cnt]++;
      	          }
      	          else
      	          {
      	             if (ix) 
      		        offd_i[o_cnt]++;
      	          }
      	          if (ix < nx*(px+1)-1)
      	          {
      	             diag_i[cnt]++;
      	          }
      	          else
      	          {
      	             if (ix+1 < nx_global) 
      		        offd_i[o_cnt]++;
      	          }
               }
               else
               {
                  if (iy+1 < ny_global) 
                  {
                     offd_i[o_cnt]++;
      	             if (ix > nx*px)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else if (ix)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             if (ix < nx*(px+1)-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else if (ix < nx_global-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
                  }
               }
            }
            else
            {
               if (iz+1 < nz_global)
	       {
		  offd_i[o_cnt]++;
                  if (iy > ny*py) 
                  {
                     offd_i[o_cnt]++;
      	             if (ix > nx*px)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else
      	             {
      	                if (ix) 
      	   	        offd_i[o_cnt]++;
      	             }
      	             if (ix < nx*(px+1)-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else
      	             {
      	                if (ix+1 < nx_global) 
      	   	        offd_i[o_cnt]++;
      	             }
                  }
                  else
                  {
                     if (iy) 
                     {
                        offd_i[o_cnt]++;
      	                if (ix > nx*px)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                else if (ix)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                if (ix < nx*(px+1)-1)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                else if (ix < nx_global-1)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
                     }
                  }
                  if (ix > nx*px) 
                     offd_i[o_cnt]++;
                  else
                  {
                     if (ix) 
                     {
                        offd_i[o_cnt]++; 
                     }
                  }
                  if (ix+1 < nx*(px+1)) 
                     offd_i[o_cnt]++;
                  else
                  {
                     if (ix+1 < nx_global) 
                     {
                        offd_i[o_cnt]++; 
                     }
                  }
                  if (iy+1 < ny*(py+1)) 
                  {
                     offd_i[o_cnt]++;
      	             if (ix > nx*px)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else
      	             {
      	                if (ix) 
      	   	           offd_i[o_cnt]++;
      	             }
      	             if (ix < nx*(px+1)-1)
      	             {
      	                offd_i[o_cnt]++;
      	             }
      	             else
      	             {
      	                if (ix+1 < nx_global) 
      	   	           offd_i[o_cnt]++;
      	             }
                  }
                  else
                  {
                     if (iy+1 < ny_global) 
                     {
                        offd_i[o_cnt]++;
      	                if (ix > nx*px)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                else if (ix)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                if (ix < nx*(px+1)-1)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
      	                else if (ix < nx_global-1)
      	                {
      	                   offd_i[o_cnt]++;
      	                }
                     }
                  }
               }
            }
            nnz[my_thread] += diag_i[cnt]+offd_i[o_cnt];
            row_nums[cnt] = row_index+cnt;
            num_cols[cnt] = diag_i[cnt]+offd_i[o_cnt];
         }
      }
   }

#ifdef HYPRE_USING_OPENMP
#pragma omp barrier
#endif

   if (my_thread == 0)
   {
      for (i=1; i< num_threads; i++)
         nnz[i]+= nnz[i-1];

      all_nnz = nnz[num_threads-1];
      col_nums = hypre_CTAlloc(HYPRE_Int, all_nnz);
      data = hypre_CTAlloc(HYPRE_Real, all_nnz);

      HYPRE_IJMatrixSetDiagOffdSizes( ij_A, diag_i, offd_i);
   }

#ifdef HYPRE_USING_OPENMP
#pragma omp barrier
#endif

   if (my_thread) 
   {
      cnt = nnz[my_thread-1];
      new_row_index = row_index+(iz_start-nz*pz)*nxy;
   }
   else
   {
      cnt = 0;
      new_row_index = row_index;
   }
   for (iz = iz_start;  iz < iz_end; iz++)
   {
      for (iy = iy_start;  iy < iy_end; iy++)
      {
         for (ix = ix_start; ix < ix_end; ix++)
         {
            col_nums[cnt] = new_row_index;
            data[cnt++] = value[0];
            if (iz > nz*pz) 
            {
               if (iy > ny*py) 
               {
      	          if (ix > nx*px)
      	          {
      	             col_nums[cnt] = new_row_index-nxy-nx-1;
      	             data[cnt++] = value[1];
      	          }
      	          else
      	          {
      	             if (ix) 
      	             {
      		        col_nums[cnt] = hypre_map27(ix-1,iy-1,iz-1,px-1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	          }
      	          col_nums[cnt] = new_row_index-nxy-nx;
      	          data[cnt++] = value[1];
      	          if (ix < nx*(px+1)-1)
      	          {
      	             col_nums[cnt] = new_row_index-nxy-nx+1;
      	             data[cnt++] = value[1];
      	          }
      	          else
      	          {
      	             if (ix+1 < nx_global) 
      	             {
      		        col_nums[cnt] = hypre_map27(ix+1,iy-1,iz-1,px+1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	          }
               }
               else
               {
                  if (iy) 
                  {
      	             if (ix > nx*px)
      	             {
      		        col_nums[cnt] = hypre_map27(ix-1,iy-1,iz-1,px,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else if (ix)
      	             {
      		        col_nums[cnt] = hypre_map27(ix-1,iy-1,iz-1,px-1,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      		     col_nums[cnt] = hypre_map27(ix,iy-1,iz-1,px,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
      	             if (ix < nx*(px+1)-1)
      	             {
      		        col_nums[cnt] = hypre_map27(ix+1,iy-1,iz-1,px,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else if (ix < nx_global-1)
      	             {
      		        col_nums[cnt] = hypre_map27(ix+1,iy-1,iz-1,px+1,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
                  }
               }
               if (ix > nx*px) 
      	       {   
      	          col_nums[cnt] = new_row_index-nxy-1;
      	          data[cnt++] = value[1];
      	       }   
               else
               {
                  if (ix) 
                  {
      		     col_nums[cnt] = hypre_map27(ix-1,iy,iz-1,px-1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
                  }
               }
      	       col_nums[cnt] = new_row_index-nxy;
      	       data[cnt++] = value[1];
               if (ix+1 < nx*(px+1)) 
      	       {   
      	          col_nums[cnt] = new_row_index-nxy+1;
      	          data[cnt++] = value[1];
      	       }   
               else
               {
                  if (ix+1 < nx_global) 
                  {
      		     col_nums[cnt] = hypre_map27(ix+1,iy,iz-1,px+1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
                  }
               }
               if (iy+1 < ny*(py+1)) 
               {
      	          if (ix > nx*px)
      	          {
      	             col_nums[cnt] = new_row_index-nxy+nx-1;
      	             data[cnt++] = value[1];
      	          }
      	          else
      	          {
      	             if (ix) 
                     {
      		        col_nums[cnt] = hypre_map27(ix-1,iy+1,iz-1,px-1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
                     }
      	          }
      	          col_nums[cnt] = new_row_index-nxy+nx;
      	          data[cnt++] = value[1];
      	          if (ix < nx*(px+1)-1)
      	          {
      	             col_nums[cnt] = new_row_index-nxy+nx+1;
      	             data[cnt++] = value[1];
      	          }
      	          else
      	          {
      	             if (ix+1 < nx_global) 
                     {
      		        col_nums[cnt] = hypre_map27(ix+1,iy+1,iz-1,px+1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
                     }
      	          }
               }
               else
               {
                  if (iy+1 < ny_global) 
                  {
      	             if (ix > nx*px)
      	             {
      		        col_nums[cnt] = hypre_map27(ix-1,iy+1,iz-1,px,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else if (ix)
      	             {
      		        col_nums[cnt] = hypre_map27(ix-1,iy+1,iz-1,px-1,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      		     col_nums[cnt] = hypre_map27(ix,iy+1,iz-1,px,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
      	             if (ix < nx*(px+1)-1)
      	             {
      		        col_nums[cnt] = hypre_map27(ix+1,iy+1,iz-1,px,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else if (ix < nx_global-1)
      	             {
      		        col_nums[cnt] = hypre_map27(ix+1,iy+1,iz-1,px+1,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
                  }
               }
            }
            else
            {
               if (iz)
	       {
                  if (iy > ny*py) 
                  {
      	             if (ix > nx*px)
      	             {
      		        col_nums[cnt] = hypre_map27(ix-1,iy-1,iz-1,px,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else
      	             {
      	                if (ix) 
      	                {
      		           col_nums[cnt] = hypre_map27(ix-1,iy-1,iz-1,px-1,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	             }
      		     col_nums[cnt] = hypre_map27(ix,iy-1,iz-1,px,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
      	             if (ix < nx*(px+1)-1)
      	             {
      		        col_nums[cnt] = hypre_map27(ix+1,iy-1,iz-1,px,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else
      	             {
      	                if (ix+1 < nx_global) 
      	                {
      		           col_nums[cnt] = hypre_map27(ix+1,iy-1,iz-1,px+1,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	             }
                  }
                  else
                  {
                     if (iy) 
                     {
      	                if (ix > nx*px)
      	                {
      		           col_nums[cnt] = hypre_map27(ix-1,iy-1,iz-1,px,py-1,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	                else if (ix)
      	                {
      		           col_nums[cnt] =hypre_map27(ix-1,iy-1,iz-1,px-1,py-1,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      		        col_nums[cnt] = hypre_map27(ix,iy-1,iz-1,px,py-1,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	                if (ix < nx*(px+1)-1)
      	                {
      		           col_nums[cnt] = hypre_map27(ix+1,iy-1,iz-1,px,py-1,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	                else if (ix < nx_global-1)
      	                {
      		           col_nums[cnt] =hypre_map27(ix+1,iy-1,iz-1,px+1,py-1,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
                     }
                  }
                  if (ix > nx*px) 
                  {
      		     col_nums[cnt] =hypre_map27(ix-1,iy,iz-1,px,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
                  }
                  else
                  {
                     if (ix) 
                     {
      		        col_nums[cnt] =hypre_map27(ix-1,iy,iz-1,px-1,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
                     }
                  }
      		  col_nums[cnt] =hypre_map27(ix,iy,iz-1,px,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		  data[cnt++] = value[1];
                  if (ix+1 < nx*(px+1)) 
                  {
      		     col_nums[cnt] =hypre_map27(ix+1,iy,iz-1,px,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
                  }
                  else
                  {
                     if (ix+1 < nx_global) 
                     {
      		        col_nums[cnt] =hypre_map27(ix+1,iy,iz-1,px+1,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
                     }
                  }
                  if (iy+1 < ny*(py+1)) 
                  {
      	             if (ix > nx*px)
      	             {
      		        col_nums[cnt] =hypre_map27(ix-1,iy+1,iz-1,px,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else
      	             {
      	                if (ix) 
      	                {
      		           col_nums[cnt] =hypre_map27(ix-1,iy+1,iz-1,px-1,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	             }
      		     col_nums[cnt] =hypre_map27(ix,iy+1,iz-1,px,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
      	             if (ix < nx*(px+1)-1)
      	             {
      		        col_nums[cnt] =hypre_map27(ix+1,iy+1,iz-1,px,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else
      	             {
      	                if (ix+1 < nx_global) 
      	                {
      		           col_nums[cnt] =hypre_map27(ix+1,iy+1,iz-1,px+1,py,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	             }
                  }
                  else
                  {
                     if (iy+1 < ny_global) 
                     {
      	                if (ix > nx*px)
      	                {
      		           col_nums[cnt] =hypre_map27(ix-1,iy+1,iz-1,px,py+1,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	                else if (ix)
      	                {
      		           col_nums[cnt] =hypre_map27(ix-1,iy+1,iz-1,px-1,py+1,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      		        col_nums[cnt] =hypre_map27(ix,iy+1,iz-1,px,py+1,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	                if (ix < nx*(px+1)-1)
      	                {
      		           col_nums[cnt] =hypre_map27(ix+1,iy+1,iz-1,px,py+1,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	                else if (ix < nx_global-1)
      	                {
      		           col_nums[cnt] =hypre_map27(ix+1,iy+1,iz-1,px+1,py+1,pz-1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
                     }
                  }
               }
            }
            if (iy > ny*py) 
            {
   	       if (ix > nx*px)
   	       {
   	          col_nums[cnt] = new_row_index-nx-1;
   	          data[cnt++] = value[1];
   	       }
   	       else
   	       {
   	          if (ix) 
   	          {
      		     col_nums[cnt] =hypre_map27(ix-1,iy-1,iz,px-1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
   	          }
   	       }
   	       col_nums[cnt] = new_row_index-nx;
   	       data[cnt++] = value[1];
   	       if (ix < nx*(px+1)-1)
   	       {
   	          col_nums[cnt] = new_row_index-nx+1;
   	          data[cnt++] = value[1];
   	       }
   	       else
   	       {
   	          if (ix+1 < nx_global) 
   	          {
      		     col_nums[cnt] =hypre_map27(ix+1,iy-1,iz,px+1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
   	          }
   	       }
            }
            else
            {
               if (iy) 
               {
   	          if (ix > nx*px)
   	          {
      		     col_nums[cnt] =hypre_map27(ix-1,iy-1,iz,px,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
   	          }
   	          else if (ix)
   	          {
      		     col_nums[cnt] =hypre_map27(ix-1,iy-1,iz,px-1,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
   	          }
      		  col_nums[cnt] =hypre_map27(ix,iy-1,iz,px,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		  data[cnt++] = value[1];
   	          if (ix < nx*(px+1)-1)
   	          {
      		     col_nums[cnt] =hypre_map27(ix+1,iy-1,iz,px,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
   	          }
   	          else if (ix < nx_global-1)
   	          {
      		     col_nums[cnt] =hypre_map27(ix+1,iy-1,iz,px+1,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
   	          }
               }
            }
            if (ix > nx*px) 
            {
               col_nums[cnt] = new_row_index-1;
               data[cnt++] = value[1];
            }
            else
            {
               if (ix) 
               {
      		  col_nums[cnt] =hypre_map27(ix-1,iy,iz,px-1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		  data[cnt++] = value[1];
               }
            }
            if (ix+1 < nx*(px+1)) 
            {
               col_nums[cnt] = new_row_index+1;
               data[cnt++] = value[1];
            }
            else
            {
               if (ix+1 < nx_global) 
               {
      		  col_nums[cnt] =hypre_map27(ix+1,iy,iz,px+1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		  data[cnt++] = value[1];
               }
            }
            if (iy+1 < ny*(py+1)) 
            {
   	       if (ix > nx*px)
   	       {
                  col_nums[cnt] = new_row_index+nx-1;
                  data[cnt++] = value[1];
   	       }
   	       else
   	       {
   	          if (ix) 
                  {
      		     col_nums[cnt] =hypre_map27(ix-1,iy+1,iz,px-1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
                  }
   	       }
               col_nums[cnt] = new_row_index+nx;
               data[cnt++] = value[1];
   	       if (ix < nx*(px+1)-1)
   	       {
                  col_nums[cnt] = new_row_index+nx+1;
                  data[cnt++] = value[1];
   	       }
   	       else
   	       {
   	          if (ix+1 < nx_global) 
                  {
      		     col_nums[cnt] =hypre_map27(ix+1,iy+1,iz,px+1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
                  }
   	       }
            }
            else
            {
               if (iy+1 < ny_global) 
               {
   	          if (ix > nx*px)
   	          {
      		     col_nums[cnt] =hypre_map27(ix-1,iy+1,iz,px,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
   	          }
   	          else if (ix)
   	          {
      		     col_nums[cnt] =hypre_map27(ix-1,iy+1,iz,px-1,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
   	          }
      		  col_nums[cnt] =hypre_map27(ix,iy+1,iz,px,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		  data[cnt++] = value[1];
   	          if (ix < nx*(px+1)-1)
   	          {
      		     col_nums[cnt] =hypre_map27(ix+1,iy+1,iz,px,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
   	          }
   	          else if (ix < nx_global-1)
   	          {
      		     col_nums[cnt] =hypre_map27(ix+1,iy+1,iz,px+1,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
   	          }
               }
            }
            if (iz+1 < nz*(pz+1)) 
            {
               if (iy > ny*py) 
               {
      	          if (ix > nx*px)
      	          {
      	             col_nums[cnt] = new_row_index+nxy-nx-1;
      	             data[cnt++] = value[1];
      	          }
      	          else
      	          {
      	             if (ix) 
   	             {
      		        col_nums[cnt] =hypre_map27(ix-1,iy-1,iz+1,px-1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
   	             }
      	          }
      	          col_nums[cnt] = new_row_index+nxy-nx;
      	          data[cnt++] = value[1];
      	          if (ix < nx*(px+1)-1)
      	          {
      	             col_nums[cnt] = new_row_index+nxy-nx+1;
      	             data[cnt++] = value[1];
      	          }
      	          else
      	          {
      	             if (ix+1 < nx_global) 
   	             {
      		        col_nums[cnt] =hypre_map27(ix+1,iy-1,iz+1,px+1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
   	             }
      	          }
               }
               else
               {
                  if (iy) 
                  {
      	             if (ix > nx*px)
      	             {
      		        col_nums[cnt] =hypre_map27(ix-1,iy-1,iz+1,px,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else if (ix)
      	             {
      		        col_nums[cnt] =hypre_map27(ix-1,iy-1,iz+1,px-1,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      		     col_nums[cnt] =hypre_map27(ix,iy-1,iz+1,px,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
      	             if (ix < nx*(px+1)-1)
      	             {
      		        col_nums[cnt] =hypre_map27(ix+1,iy-1,iz+1,px,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else if (ix < nx_global-1)
      	             {
      		        col_nums[cnt] =hypre_map27(ix+1,iy-1,iz+1,px+1,py-1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
                  }
               }
               if (ix > nx*px) 
               {
                  col_nums[cnt] = new_row_index+nxy-1;
                  data[cnt++] = value[1];
               }
               else
               {
                  if (ix) 
                  {
      		     col_nums[cnt] =hypre_map27(ix-1,iy,iz+1,px-1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
                  }
               }
               col_nums[cnt] = new_row_index+nxy;
               data[cnt++] = value[1];
               if (ix+1 < nx*(px+1)) 
               {
                  col_nums[cnt] = new_row_index+nxy+1;
                  data[cnt++] = value[1];
               }
               else
               {
                  if (ix+1 < nx_global) 
                  {
      		     col_nums[cnt] =hypre_map27(ix+1,iy,iz+1,px+1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
                  }
               }
               if (iy+1 < ny*(py+1)) 
               {
      	          if (ix > nx*px)
      	          {
                     col_nums[cnt] = new_row_index+nxy+nx-1;
                     data[cnt++] = value[1];
      	          }
      	          else
      	          {
      	             if (ix) 
                     {
      		        col_nums[cnt] =hypre_map27(ix-1,iy+1,iz+1,px-1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
                     }
      	          }
                  col_nums[cnt] = new_row_index+nxy+nx;
                  data[cnt++] = value[1];
      	          if (ix < nx*(px+1)-1)
      	          {
                     col_nums[cnt] = new_row_index+nxy+nx+1;
                     data[cnt++] = value[1];
      	          }
      	          else
      	          {
      	             if (ix+1 < nx_global) 
                     {
      		        col_nums[cnt] =hypre_map27(ix+1,iy+1,iz+1,px+1,py,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
                     }
      	          }
               }
               else
               {
                  if (iy+1 < ny_global) 
                  {
      	             if (ix > nx*px)
      	             {
      		        col_nums[cnt] =hypre_map27(ix-1,iy+1,iz+1,px,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else if (ix)
      	             {
      		        col_nums[cnt] =hypre_map27(ix-1,iy+1,iz+1,px-1,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      		     col_nums[cnt] =hypre_map27(ix,iy+1,iz+1,px,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
      	             if (ix < nx*(px+1)-1)
      	             {
      		        col_nums[cnt] =hypre_map27(ix+1,iy+1,iz+1,px,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else if (ix < nx_global-1)
      	             {
      		        col_nums[cnt] =hypre_map27(ix+1,iy+1,iz+1,px+1,py+1,pz,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
                  }
               }
            }
            else
            {
               if (iz+1 < nz_global)
	       {
                  if (iy > ny*py) 
                  {
      	             if (ix > nx*px)
      	             {
      		        col_nums[cnt] =hypre_map27(ix-1,iy-1,iz+1,px,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else
      	             {
      	                if (ix) 
      	                {
      		           col_nums[cnt] =hypre_map27(ix-1,iy-1,iz+1,px-1,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	             }
      		     col_nums[cnt] =hypre_map27(ix,iy-1,iz+1,px,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
      	             if (ix < nx*(px+1)-1)
      	             {
      		        col_nums[cnt] =hypre_map27(ix+1,iy-1,iz+1,px,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else
      	             {
      	                if (ix+1 < nx_global) 
      	                {
      		           col_nums[cnt] =hypre_map27(ix+1,iy-1,iz+1,px+1,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	             }
                  }
                  else
                  {
                     if (iy) 
                     {
      	                if (ix > nx*px)
      	                {
      		           col_nums[cnt] =hypre_map27(ix-1,iy-1,iz+1,px,py-1,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	                else if (ix)
      	                {
      		           col_nums[cnt] =hypre_map27(ix-1,iy-1,iz+1,px-1,py-1,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      		        col_nums[cnt] =hypre_map27(ix,iy-1,iz+1,px,py-1,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	                if (ix < nx*(px+1)-1)
      	                {
      		           col_nums[cnt] =hypre_map27(ix+1,iy-1,iz+1,px,py-1,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	                else if (ix < nx_global-1)
      	                {
      		           col_nums[cnt] =hypre_map27(ix+1,iy-1,iz+1,px+1,py-1,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
                     }
                  }
                  if (ix > nx*px) 
                  {
      		     col_nums[cnt] =hypre_map27(ix-1,iy,iz+1,px,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
                  }
                  else
                  {
                     if (ix) 
                     {
      		        col_nums[cnt] =hypre_map27(ix-1,iy,iz+1,px-1,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
                     }
                  }
      		  col_nums[cnt] =hypre_map27(ix,iy,iz+1,px,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		  data[cnt++] = value[1];
                  if (ix+1 < nx*(px+1)) 
                  {
      		     col_nums[cnt] =hypre_map27(ix+1,iy,iz+1,px,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
                  }
                  else
                  {
                     if (ix+1 < nx_global) 
                     {
      		        col_nums[cnt] =hypre_map27(ix+1,iy,iz+1,px+1,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
                     }
                  }
                  if (iy+1 < ny*(py+1)) 
                  {
      	             if (ix > nx*px)
      	             {
      		        col_nums[cnt] =hypre_map27(ix-1,iy+1,iz+1,px,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else
      	             {
      	                if (ix) 
      	                {
      		           col_nums[cnt] =hypre_map27(ix-1,iy+1,iz+1,px-1,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	             }
      		     col_nums[cnt] =hypre_map27(ix,iy+1,iz+1,px,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		     data[cnt++] = value[1];
      	             if (ix < nx*(px+1)-1)
      	             {
      		        col_nums[cnt] =hypre_map27(ix+1,iy+1,iz+1,px,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	             }
      	             else
      	             {
      	                if (ix+1 < nx_global) 
      	                {
      		           col_nums[cnt] =hypre_map27(ix+1,iy+1,iz+1,px+1,py,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	             }
                  }
                  else
                  {
                     if (iy+1 < ny_global) 
                     {
      	                if (ix > nx*px)
      	                {
      		           col_nums[cnt] =hypre_map27(ix-1,iy+1,iz+1,px,py+1,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	                else if (ix)
      	                {
      		           col_nums[cnt] =hypre_map27(ix-1,iy+1,iz+1,px-1,py+1,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      		        col_nums[cnt] =hypre_map27(ix,iy+1,iz+1,px,py+1,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		        data[cnt++] = value[1];
      	                if (ix < nx*(px+1)-1)
      	                {
      		           col_nums[cnt] =hypre_map27(ix+1,iy+1,iz+1,px,py+1,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
      	                else if (ix < nx_global-1)
      	                {
      		           col_nums[cnt] =hypre_map27(ix+1,iy+1,iz+1,px+1,py+1,pz+1,
					Cx,Cy,Cz,nx,nxy);
      		           data[cnt++] = value[1];
      	                }
                     }
                  }
               }
            }
            new_row_index++;
         }
      }
    }
   } /*end parallel loop */

   HYPRE_IJMatrixInitialize(ij_A);

   HYPRE_IJMatrixSetOMPFlag(ij_A, 1);

   HYPRE_IJMatrixSetValues(ij_A, local_size, num_cols, row_nums,
			col_nums, data);

   HYPRE_IJMatrixAssemble(ij_A);

   /*A = (HYPRE_ParCSRMatrix) GenerateLaplacian27pt(hypre_MPI_COMM_WORLD,
                                                  nx, ny, nz, P, Q, R, px, py, r, values);*/

   hypre_TFree(diag_i);
   hypre_TFree(offd_i);
   hypre_TFree(num_cols);
   hypre_TFree(col_nums);
   hypre_TFree(row_nums);
   hypre_TFree(data);
   hypre_TFree(value);
   hypre_TFree(nnz);

   *system_size_ptr = global_size;
   *ij_A_ptr = ij_A;

   return (0);
}

/*----------------------------------------------------------------------
 * Add epsilon to diagonal of A
 *----------------------------------------------------------------------*/

HYPRE_Int
AddOrRestoreAIJ( HYPRE_IJMatrix  ij_A, HYPRE_Real eps, HYPRE_Int action  )
{
   HYPRE_Int first_row, last_row, i;
   HYPRE_Int first_col, last_col, local_size;
   HYPRE_Int *num_cols;
   HYPRE_Int *row_nums;
   HYPRE_Int *col_nums;
   HYPRE_Real *data;

   HYPRE_IJMatrixGetLocalRange(ij_A, &first_row, &last_row, &first_col, &last_col);

   local_size = last_row-first_row+1;
   num_cols = hypre_CTAlloc(HYPRE_Int, local_size);
   row_nums = hypre_CTAlloc(HYPRE_Int, local_size);
   col_nums = hypre_CTAlloc(HYPRE_Int, local_size);
   data = hypre_CTAlloc(HYPRE_Real, local_size);

#ifdef HYPRE_USING_OPENMP
#pragma omp parallel for private(i) HYPRE_SMP_SCHEDULE
#endif
   for (i=0; i < local_size; i++)
   {
      num_cols[i] = 1;
      row_nums[i] = first_row+i;
      col_nums[i] = first_row+i;
      data[i] = eps;
   }

   if (action) 
      HYPRE_IJMatrixAddToValues(ij_A, local_size, num_cols, row_nums,
			col_nums, data);
   else
      HYPRE_IJMatrixSetValues(ij_A, local_size, num_cols, row_nums,
			col_nums, data);

   HYPRE_IJMatrixAssemble(ij_A);
   return(0);
}

HYPRE_Int
hypre_map27( HYPRE_Int  ix,
      HYPRE_Int  iy,
      HYPRE_Int  iz,
      HYPRE_Int  px,
      HYPRE_Int  py,
      HYPRE_Int  pz,
      HYPRE_Int  Cx,
      HYPRE_Int  Cy,
      HYPRE_Int  Cz,
      HYPRE_Int nx,
      HYPRE_Int nxy)
{
   HYPRE_Int global_index = pz*Cz + py*Cy +px*Cx + iz*nxy + iy*nx + ix;

   return global_index;
}
