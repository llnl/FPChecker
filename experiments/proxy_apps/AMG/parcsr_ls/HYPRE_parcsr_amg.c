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

#include "_hypre_parcsr_ls.h"

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGCreate
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGCreate( HYPRE_Solver *solver)
{
   if (!solver)
   {
      hypre_error_in_arg(1);
      return hypre_error_flag;
   }
   *solver = (HYPRE_Solver) hypre_BoomerAMGCreate( ) ;

   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGDestroy
 *--------------------------------------------------------------------------*/

HYPRE_Int 
HYPRE_BoomerAMGDestroy( HYPRE_Solver solver )
{
   return( hypre_BoomerAMGDestroy( (void *) solver ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetup
 *--------------------------------------------------------------------------*/

HYPRE_Int 
HYPRE_BoomerAMGSetup( HYPRE_Solver solver,
                      HYPRE_ParCSRMatrix A,
                      HYPRE_ParVector b,
                      HYPRE_ParVector x      )
{
   return( hypre_BoomerAMGSetup( (void *) solver,
                                 (hypre_ParCSRMatrix *) A,
                                 (hypre_ParVector *) b,
                                 (hypre_ParVector *) x ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSolve
 *--------------------------------------------------------------------------*/

HYPRE_Int 
HYPRE_BoomerAMGSolve( HYPRE_Solver solver,
                      HYPRE_ParCSRMatrix A,
                      HYPRE_ParVector b,
                      HYPRE_ParVector x      )
{


   return( hypre_BoomerAMGSolve( (void *) solver,
                                 (hypre_ParCSRMatrix *) A,
                                 (hypre_ParVector *) b,
                                 (hypre_ParVector *) x ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetRestriction
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetRestriction( HYPRE_Solver solver,
                               HYPRE_Int          restr_par  )
{
   return( hypre_BoomerAMGSetRestriction( (void *) solver, restr_par ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetMaxLevels, HYPRE_BoomerAMGGetMaxLevels
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetMaxLevels( HYPRE_Solver solver,
                             HYPRE_Int          max_levels  )
{
   return( hypre_BoomerAMGSetMaxLevels( (void *) solver, max_levels ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetMaxLevels( HYPRE_Solver solver,
                             HYPRE_Int        * max_levels  )
{
   return( hypre_BoomerAMGGetMaxLevels( (void *) solver, max_levels ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetMaxCoarseSize, HYPRE_BoomerAMGGetMaxCoarseSize
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetMaxCoarseSize( HYPRE_Solver solver,
                                 HYPRE_Int          max_coarse_size  )
{
   return( hypre_BoomerAMGSetMaxCoarseSize( (void *) solver, max_coarse_size ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetMaxCoarseSize( HYPRE_Solver solver,
                                 HYPRE_Int        * max_coarse_size  )
{
   return( hypre_BoomerAMGGetMaxCoarseSize( (void *) solver, max_coarse_size ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetMinCoarseSize, HYPRE_BoomerAMGGetMinCoarseSize
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetMinCoarseSize( HYPRE_Solver solver,
                                 HYPRE_Int          min_coarse_size  )
{
   return( hypre_BoomerAMGSetMinCoarseSize( (void *) solver, min_coarse_size ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetMinCoarseSize( HYPRE_Solver solver,
                                 HYPRE_Int        * min_coarse_size  )
{
   return( hypre_BoomerAMGGetMinCoarseSize( (void *) solver, min_coarse_size ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetSeqThreshold, HYPRE_BoomerAMGGetSeqThreshold
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetSeqThreshold( HYPRE_Solver solver,
                                HYPRE_Int          seq_threshold  )
{
   return( hypre_BoomerAMGSetSeqThreshold( (void *) solver, seq_threshold ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetSeqThreshold( HYPRE_Solver solver,
                                HYPRE_Int        * seq_threshold  )
{
   return( hypre_BoomerAMGGetSeqThreshold( (void *) solver, seq_threshold ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetRedundant, HYPRE_BoomerAMGGetRedundant
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetRedundant( HYPRE_Solver solver,
                                HYPRE_Int          redundant  )
{
   return( hypre_BoomerAMGSetRedundant( (void *) solver, redundant ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetRedundant( HYPRE_Solver solver,
                                HYPRE_Int        * redundant  )
{
   return( hypre_BoomerAMGGetRedundant( (void *) solver, redundant ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetStrongThreshold, HYPRE_BoomerAMGGetStrongThreshold
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetStrongThreshold( HYPRE_Solver solver,
                                   HYPRE_Real   strong_threshold  )
{
   return( hypre_BoomerAMGSetStrongThreshold( (void *) solver,
                                              strong_threshold ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetStrongThreshold( HYPRE_Solver solver,
                                   HYPRE_Real * strong_threshold  )
{
   return( hypre_BoomerAMGGetStrongThreshold( (void *) solver,
                                              strong_threshold ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetMaxRowSum, HYPRE_BoomerAMGGetMaxRowSum
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetMaxRowSum( HYPRE_Solver solver,
                             HYPRE_Real   max_row_sum  )
{
   return( hypre_BoomerAMGSetMaxRowSum( (void *) solver,
                                        max_row_sum ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetMaxRowSum( HYPRE_Solver solver,
                             HYPRE_Real * max_row_sum  )
{
   return( hypre_BoomerAMGGetMaxRowSum( (void *) solver,
                                        max_row_sum ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetTruncFactor, HYPRE_BoomerAMGGetTruncFactor
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetTruncFactor( HYPRE_Solver solver,
                               HYPRE_Real   trunc_factor  )
{
   return( hypre_BoomerAMGSetTruncFactor( (void *) solver,
                                          trunc_factor ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetTruncFactor( HYPRE_Solver solver,
                               HYPRE_Real * trunc_factor  )
{
   return( hypre_BoomerAMGGetTruncFactor( (void *) solver,
                                          trunc_factor ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetPMaxElmts, HYPRE_BoomerAMGGetPMaxElmts
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetPMaxElmts( HYPRE_Solver solver,
                             HYPRE_Int   P_max_elmts  )
{
   return( hypre_BoomerAMGSetPMaxElmts( (void *) solver,
                                        P_max_elmts ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetPMaxElmts( HYPRE_Solver solver,
                             HYPRE_Int   * P_max_elmts  )
{
   return( hypre_BoomerAMGGetPMaxElmts( (void *) solver,
                                        P_max_elmts ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetJacobiTruncThreshold, HYPRE_BoomerAMGGetJacobiTruncThreshold
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetJacobiTruncThreshold( HYPRE_Solver solver,
                                        HYPRE_Real   jacobi_trunc_threshold  )
{
   return( hypre_BoomerAMGSetJacobiTruncThreshold( (void *) solver,
                                                   jacobi_trunc_threshold ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetJacobiTruncThreshold( HYPRE_Solver solver,
                                        HYPRE_Real * jacobi_trunc_threshold  )
{
   return( hypre_BoomerAMGGetJacobiTruncThreshold( (void *) solver,
                                                   jacobi_trunc_threshold ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetPostInterpType, HYPRE_BoomerAMGGetPostInterpType
 *  If >0, specifies something to do to improve a computed interpolation matrix.
 * defaults to 0, for nothing.
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetPostInterpType( HYPRE_Solver solver,
                                  HYPRE_Int       post_interp_type  )
{
   return( hypre_BoomerAMGSetPostInterpType( (void *) solver,
                                             post_interp_type ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetPostInterpType( HYPRE_Solver solver,
                                  HYPRE_Int     * post_interp_type  )
{
   return( hypre_BoomerAMGGetPostInterpType( (void *) solver,
                                             post_interp_type ) );
}


/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetSCommPkgSwitch
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetSCommPkgSwitch( HYPRE_Solver solver,
                                  HYPRE_Real   S_commpkg_switch  )
{
   return( hypre_BoomerAMGSetSCommPkgSwitch( (void *) solver,
                                             S_commpkg_switch ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetInterpType
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetInterpType( HYPRE_Solver solver,
                              HYPRE_Int          interp_type  )
{
   return( hypre_BoomerAMGSetInterpType( (void *) solver, interp_type ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetSepWeight
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetSepWeight( HYPRE_Solver solver,
                             HYPRE_Int          sep_weight  )
{
   return( hypre_BoomerAMGSetSepWeight( (void *) solver, sep_weight ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetMinIter
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetMinIter( HYPRE_Solver solver,
                           HYPRE_Int          min_iter  )
{
   return( hypre_BoomerAMGSetMinIter( (void *) solver, min_iter ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetMaxIter
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetMaxIter( HYPRE_Solver solver,
                           HYPRE_Int          max_iter  )
{
   return( hypre_BoomerAMGSetMaxIter( (void *) solver, max_iter ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetMaxIter( HYPRE_Solver solver,
                           HYPRE_Int        * max_iter  )
{
   return( hypre_BoomerAMGGetMaxIter( (void *) solver, max_iter ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetCoarsenType, HYPRE_BoomerAMGGetCoarsenType
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetCoarsenType( HYPRE_Solver solver,
                               HYPRE_Int          coarsen_type  )
{
   return( hypre_BoomerAMGSetCoarsenType( (void *) solver, coarsen_type ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetCoarsenType( HYPRE_Solver solver,
                               HYPRE_Int        * coarsen_type  )
{
   return( hypre_BoomerAMGGetCoarsenType( (void *) solver, coarsen_type ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetMeasureType, HYPRE_BoomerAMGGetMeasureType
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetMeasureType( HYPRE_Solver solver,
                               HYPRE_Int          measure_type  )
{
   return( hypre_BoomerAMGSetMeasureType( (void *) solver, measure_type ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetMeasureType( HYPRE_Solver solver,
                               HYPRE_Int        * measure_type  )
{
   return( hypre_BoomerAMGGetMeasureType( (void *) solver, measure_type ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetOldDefault
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetOldDefault( HYPRE_Solver solver)
{
   HYPRE_BoomerAMGSetCoarsenType( solver, 6 );
   HYPRE_BoomerAMGSetInterpType( solver, 0 );
   HYPRE_BoomerAMGSetPMaxElmts( solver, 0 );
   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetSetupType
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetSetupType( HYPRE_Solver solver,
                             HYPRE_Int          setup_type  )
{
   return( hypre_BoomerAMGSetSetupType( (void *) solver, setup_type ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetCycleType, HYPRE_BoomerAMGGetCycleType
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetCycleType( HYPRE_Solver solver,
                             HYPRE_Int          cycle_type  )
{
   return( hypre_BoomerAMGSetCycleType( (void *) solver, cycle_type ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetCycleType( HYPRE_Solver solver,
                             HYPRE_Int        * cycle_type  )
{
   return( hypre_BoomerAMGGetCycleType( (void *) solver, cycle_type ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetTol, HYPRE_BoomerAMGGetTol
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetTol( HYPRE_Solver solver,
                       HYPRE_Real   tol    )
{
   return( hypre_BoomerAMGSetTol( (void *) solver, tol ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetTol( HYPRE_Solver solver,
                       HYPRE_Real * tol    )
{
   return( hypre_BoomerAMGGetTol( (void *) solver, tol ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetNumGridSweeps
 * DEPRECATED.  There are memory management problems associated with the
 * use of a user-supplied array (who releases it?).
 * Use SetNumSweeps and SetCycleNumSweeps instead.
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetNumGridSweeps( HYPRE_Solver  solver,
                                 HYPRE_Int          *num_grid_sweeps  )
{
   return( hypre_BoomerAMGSetNumGridSweeps( (void *) solver, num_grid_sweeps ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetNumSweeps
 * There is no corresponding Get function.  Use GetCycleNumSweeps.
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetNumSweeps( HYPRE_Solver  solver,
                             HYPRE_Int          num_sweeps  )
{
   return( hypre_BoomerAMGSetNumSweeps( (void *) solver, num_sweeps ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetCycleNumSweeps, HYPRE_BoomerAMGGetCycleNumSweeps
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetCycleNumSweeps( HYPRE_Solver  solver,
                                  HYPRE_Int          num_sweeps, HYPRE_Int k  )
{
   return( hypre_BoomerAMGSetCycleNumSweeps( (void *) solver, num_sweeps, k ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetCycleNumSweeps( HYPRE_Solver  solver,
                                  HYPRE_Int        * num_sweeps, HYPRE_Int k  )
{
   return( hypre_BoomerAMGGetCycleNumSweeps( (void *) solver, num_sweeps, k ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGInitGridRelaxation
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGInitGridRelaxation( HYPRE_Int     **num_grid_sweeps_ptr,
                                   HYPRE_Int     **grid_relax_type_ptr,
                                   HYPRE_Int    ***grid_relax_points_ptr,
                                   HYPRE_Int       coarsen_type,
                                   HYPRE_Real  **relax_weights_ptr,
                                   HYPRE_Int       max_levels         )
{  HYPRE_Int i;
   HYPRE_Int *num_grid_sweeps;
   HYPRE_Int *grid_relax_type;
   HYPRE_Int **grid_relax_points;
   HYPRE_Real *relax_weights;

   *num_grid_sweeps_ptr   = hypre_CTAlloc(HYPRE_Int, 4);
   *grid_relax_type_ptr   = hypre_CTAlloc(HYPRE_Int, 4);
   *grid_relax_points_ptr = hypre_CTAlloc(HYPRE_Int*, 4);
   *relax_weights_ptr     = hypre_CTAlloc(HYPRE_Real, max_levels);

   num_grid_sweeps   = *num_grid_sweeps_ptr;
   grid_relax_type   = *grid_relax_type_ptr;
   grid_relax_points = *grid_relax_points_ptr;
   relax_weights     = *relax_weights_ptr;

   if (coarsen_type == 5)
   {
      /* fine grid */
      num_grid_sweeps[0] = 3;
      grid_relax_type[0] = 3;
      grid_relax_points[0] = hypre_CTAlloc(HYPRE_Int, 4);
      grid_relax_points[0][0] = -2;
      grid_relax_points[0][1] = -1;
      grid_relax_points[0][2] = 1;

      /* down cycle */
      num_grid_sweeps[1] = 4;
      grid_relax_type[1] = 3;
      grid_relax_points[1] = hypre_CTAlloc(HYPRE_Int, 4);
      grid_relax_points[1][0] = -1;
      grid_relax_points[1][1] = 1;
      grid_relax_points[1][2] = -2;
      grid_relax_points[1][3] = -2;

      /* up cycle */
      num_grid_sweeps[2] = 4;
      grid_relax_type[2] = 3;
      grid_relax_points[2] = hypre_CTAlloc(HYPRE_Int, 4);
      grid_relax_points[2][0] = -2;
      grid_relax_points[2][1] = -2;
      grid_relax_points[2][2] = 1;
      grid_relax_points[2][3] = -1;
   }
   else
   {  
      /* fine grid */
      num_grid_sweeps[0] = 2;
      grid_relax_type[0] = 3;
      grid_relax_points[0] = hypre_CTAlloc(HYPRE_Int, 2);
      grid_relax_points[0][0] = 1;
      grid_relax_points[0][1] = -1;
 
      /* down cycle */
      num_grid_sweeps[1] = 2;
      grid_relax_type[1] = 3;
      grid_relax_points[1] = hypre_CTAlloc(HYPRE_Int, 2);
      grid_relax_points[1][0] = 1;
      grid_relax_points[1][1] = -1;
  
      /* up cycle */
      num_grid_sweeps[2] = 2;
      grid_relax_type[2] = 3;
      grid_relax_points[2] = hypre_CTAlloc(HYPRE_Int, 2);
      grid_relax_points[2][0] = -1;
      grid_relax_points[2][1] = 1;
   }
   /* coarsest grid */
   num_grid_sweeps[3] = 1;
   grid_relax_type[3] = 3;
   grid_relax_points[3] = hypre_CTAlloc(HYPRE_Int, 1);
   grid_relax_points[3][0] = 0;

   for (i = 0; i < max_levels; i++)
      relax_weights[i] = 1.;

   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetGridRelaxType
 * DEPRECATED.  There are memory management problems associated with the
 * use of a user-supplied array (who releases it?).
 * Use SetRelaxType and SetCycleRelaxType instead.
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetGridRelaxType( HYPRE_Solver  solver,
                                 HYPRE_Int          *grid_relax_type  )
{
   return( hypre_BoomerAMGSetGridRelaxType( (void *) solver, grid_relax_type ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetRelaxType
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetRelaxType( HYPRE_Solver  solver,
                             HYPRE_Int          relax_type  )
{
   return( hypre_BoomerAMGSetRelaxType( (void *) solver, relax_type ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetCycleRelaxType, HYPRE_BoomerAMGetCycleRelaxType
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetCycleRelaxType( HYPRE_Solver  solver,
                                  HYPRE_Int          relax_type, HYPRE_Int k  )
{
   return( hypre_BoomerAMGSetCycleRelaxType( (void *) solver, relax_type, k ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetCycleRelaxType( HYPRE_Solver  solver,
                                  HYPRE_Int        * relax_type, HYPRE_Int k  )
{
   return( hypre_BoomerAMGGetCycleRelaxType( (void *) solver, relax_type, k ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetRelaxOrder
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetRelaxOrder( HYPRE_Solver  solver,
                              HYPRE_Int           relax_order)
{
   return( hypre_BoomerAMGSetRelaxOrder( (void *) solver, relax_order ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetGridRelaxPoints
 * DEPRECATED.  There are memory management problems associated with the
 * use of a user-supplied array (who releases it?).
 * Ulrike Yang suspects that nobody uses this function.
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetGridRelaxPoints( HYPRE_Solver   solver,
                                   HYPRE_Int          **grid_relax_points  )
{
   return( hypre_BoomerAMGSetGridRelaxPoints( (void *) solver, grid_relax_points ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetRelaxWeight
 * DEPRECATED.  There are memory management problems associated with the
 * use of a user-supplied array (who releases it?).
 * Use SetRelaxWt and SetLevelRelaxWt instead.
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetRelaxWeight( HYPRE_Solver  solver,
                               HYPRE_Real   *relax_weight  )
{
   return( hypre_BoomerAMGSetRelaxWeight( (void *) solver, relax_weight ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetRelaxWt
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetRelaxWt( HYPRE_Solver  solver,
                           HYPRE_Real    relax_wt  )
{
   return( hypre_BoomerAMGSetRelaxWt( (void *) solver, relax_wt ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetLevelRelaxWt
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetLevelRelaxWt( HYPRE_Solver  solver,
                                HYPRE_Real    relax_wt, 
				HYPRE_Int 	      level  )
{
   return( hypre_BoomerAMGSetLevelRelaxWt( (void *) solver, relax_wt, level ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetOmega
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetOmega( HYPRE_Solver  solver,
                         HYPRE_Real   *omega  )
{
   return( hypre_BoomerAMGSetOmega( (void *) solver, omega ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetOuterWt
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetOuterWt( HYPRE_Solver  solver,
                           HYPRE_Real    outer_wt  )
{
   return( hypre_BoomerAMGSetOuterWt( (void *) solver, outer_wt ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetLevelOuterWt
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetLevelOuterWt( HYPRE_Solver  solver,
                                HYPRE_Real    outer_wt, 
				HYPRE_Int 	      level  )
{
   return( hypre_BoomerAMGSetLevelOuterWt( (void *) solver, outer_wt, level ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetLogging, HYPRE_BoomerAMGGetLogging
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetLogging( HYPRE_Solver solver,
                           HYPRE_Int          logging  )
{
   /* This function should be called before Setup.  Logging changes
      may require allocation or freeing of arrays, which is presently
      only done there.
      It may be possible to support logging changes at other times,
      but there is little need.
   */
   return( hypre_BoomerAMGSetLogging( (void *) solver, logging ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetLogging( HYPRE_Solver solver,
                           HYPRE_Int        * logging  )
{
   return( hypre_BoomerAMGGetLogging( (void *) solver, logging ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetPrintLevel, HYPRE_BoomerAMGGetPrintLevel
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetPrintLevel( HYPRE_Solver solver,
                              HYPRE_Int        print_level  )
{
   return( hypre_BoomerAMGSetPrintLevel( (void *) solver, print_level ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetPrintLevel( HYPRE_Solver solver,
                              HYPRE_Int      * print_level  )
{
   return( hypre_BoomerAMGGetPrintLevel( (void *) solver, print_level ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetPrintFileName
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetPrintFileName( HYPRE_Solver  solver,
                                 const char   *print_file_name  )
{
   return( hypre_BoomerAMGSetPrintFileName( (void *) solver, print_file_name ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetDebugFlag, HYPRE_BoomerAMGGetDebugFlag
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetDebugFlag( HYPRE_Solver solver,
                             HYPRE_Int          debug_flag  )
{
   return( hypre_BoomerAMGSetDebugFlag( (void *) solver, debug_flag ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetDebugFlag( HYPRE_Solver solver,
                             HYPRE_Int        * debug_flag  )
{
   return( hypre_BoomerAMGGetDebugFlag( (void *) solver, debug_flag ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGGetNumIterations
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGGetNumIterations( HYPRE_Solver  solver,
                                 HYPRE_Int          *num_iterations  )
{
   return( hypre_BoomerAMGGetNumIterations( (void *) solver, num_iterations ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGGetCumNumIterations
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGGetCumNumIterations( HYPRE_Solver  solver,
                                    HYPRE_Int          *cum_num_iterations  )
{
   return( hypre_BoomerAMGGetCumNumIterations( (void *) solver, cum_num_iterations ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGGetCumNnzAP
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGGetCumNnzAP( HYPRE_Solver  solver,
                                 HYPRE_Real          *cum_nnz_AP )
{
   return( hypre_BoomerAMGGetCumNnzAP( (void *) solver, cum_nnz_AP ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGGetResidual
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGGetResidual( HYPRE_Solver solver, HYPRE_ParVector * residual )
{
   return hypre_BoomerAMGGetResidual( (void *) solver,
                                      (hypre_ParVector **) residual );
}
                            

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGGetFinalRelativeResidualNorm
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGGetFinalRelativeResidualNorm( HYPRE_Solver  solver,
                                             HYPRE_Real   *rel_resid_norm  )
{
   return( hypre_BoomerAMGGetRelResidualNorm( (void *) solver, rel_resid_norm ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetNumFunctions, HYPRE_BoomerAMGGetNumFunctions
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetNumFunctions( HYPRE_Solver  solver,
                                HYPRE_Int          num_functions  )
{
   return( hypre_BoomerAMGSetNumFunctions( (void *) solver, num_functions ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetNumFunctions( HYPRE_Solver  solver,
                                HYPRE_Int        * num_functions  )
{
   return( hypre_BoomerAMGGetNumFunctions( (void *) solver, num_functions ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetNodal
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetNodal( HYPRE_Solver  solver,
                         HYPRE_Int          nodal  )
{
   return( hypre_BoomerAMGSetNodal( (void *) solver, nodal ) );
}
/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetNodalLevels
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetNodalLevels( HYPRE_Solver  solver,
                               HYPRE_Int          nodal_levels  )
{
   return( hypre_BoomerAMGSetNodalLevels( (void *) solver, nodal_levels ) );
}


/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetNodalDiag
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetNodalDiag( HYPRE_Solver  solver,
                             HYPRE_Int          nodal  )
{
   return( hypre_BoomerAMGSetNodalDiag( (void *) solver, nodal ) );
}
/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetDofFunc
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetDofFunc( HYPRE_Solver  solver,
                           HYPRE_Int          *dof_func  )
/* Warning about a possible memory problem: When the BoomerAMG object is destroyed
   in hypre_BoomerAMGDestroy, dof_func aka DofFunc will be destroyed (currently
   line 246 of par_amg.c).  Normally this is what we want.  But if the user provided
   dof_func by calling HYPRE_BoomerAMGSetDofFunc, this could be an unwanted surprise.
   As hypre is currently commonly used, this situation is likely to be rare. */
{
   return( hypre_BoomerAMGSetDofFunc( (void *) solver, dof_func ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetNumPaths
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetNumPaths( HYPRE_Solver  solver,
                            HYPRE_Int          num_paths  )
{
   return( hypre_BoomerAMGSetNumPaths( (void *) solver, num_paths ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAggNumLevels
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAggNumLevels( HYPRE_Solver  solver,
                                HYPRE_Int          agg_num_levels  )
{
   return( hypre_BoomerAMGSetAggNumLevels( (void *) solver, agg_num_levels ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAggInterpType
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAggInterpType( HYPRE_Solver  solver,
                                 HYPRE_Int          agg_interp_type  )
{
   return( hypre_BoomerAMGSetAggInterpType( (void *) solver, agg_interp_type ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAggTruncFactor
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAggTruncFactor( HYPRE_Solver  solver,
                                  HYPRE_Real    agg_trunc_factor  )
{
   return( hypre_BoomerAMGSetAggTruncFactor( (void *) solver, agg_trunc_factor ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAddTruncFactor
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAddTruncFactor( HYPRE_Solver  solver,
                                  HYPRE_Real        add_trunc_factor  )
{
   return( hypre_BoomerAMGSetMultAddTruncFactor( (void *) solver, add_trunc_factor ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetMultAddTruncFactor
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetMultAddTruncFactor( HYPRE_Solver  solver,
                                  HYPRE_Real        add_trunc_factor  )
{
   return( hypre_BoomerAMGSetMultAddTruncFactor( (void *) solver, add_trunc_factor ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAddRelaxWt
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAddRelaxWt( HYPRE_Solver  solver,
                                  HYPRE_Real        add_rlx_wt  )
{
   return( hypre_BoomerAMGSetAddRelaxWt( (void *) solver, add_rlx_wt ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAddRelaxType
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAddRelaxType( HYPRE_Solver  solver,
                                  HYPRE_Int        add_rlx_type  )
{
   return( hypre_BoomerAMGSetAddRelaxType( (void *) solver, add_rlx_type ) );
}
/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAggP12TruncFactor
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAggP12TruncFactor( HYPRE_Solver  solver,
                                     HYPRE_Real    agg_P12_trunc_factor  )
{
   return( hypre_BoomerAMGSetAggP12TruncFactor( (void *) solver, agg_P12_trunc_factor ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAggPMaxElmts
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAggPMaxElmts( HYPRE_Solver  solver,
                                HYPRE_Int          agg_P_max_elmts  )
{
   return( hypre_BoomerAMGSetAggPMaxElmts( (void *) solver, agg_P_max_elmts ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAddPMaxElmts
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAddPMaxElmts( HYPRE_Solver  solver,
                                HYPRE_Int          add_P_max_elmts  )
{
   return( hypre_BoomerAMGSetMultAddPMaxElmts( (void *) solver, add_P_max_elmts ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetMultAddPMaxElmts
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetMultAddPMaxElmts( HYPRE_Solver  solver,
                                HYPRE_Int          add_P_max_elmts  )
{
   return( hypre_BoomerAMGSetMultAddPMaxElmts( (void *) solver, add_P_max_elmts ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAggP12MaxElmts
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAggP12MaxElmts( HYPRE_Solver  solver,
                                  HYPRE_Int          agg_P12_max_elmts  )
{
   return( hypre_BoomerAMGSetAggP12MaxElmts( (void *) solver, agg_P12_max_elmts ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetChebyOrder
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetChebyOrder( HYPRE_Solver  solver,
                              HYPRE_Int        order )
{
   return( hypre_BoomerAMGSetChebyOrder( (void *) solver, order ) );
}
/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetChebyFraction
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetChebyFraction( HYPRE_Solver  solver,
                                 HYPRE_Real     ratio )
{
   return( hypre_BoomerAMGSetChebyFraction( (void *) solver, ratio ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetChebyScale
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetChebyScale( HYPRE_Solver  solver,
                                 HYPRE_Int     scale )
{
   return( hypre_BoomerAMGSetChebyScale( (void *) solver, scale ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetChebyVariant
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetChebyVariant( HYPRE_Solver  solver,
                                 HYPRE_Int     variant )
{
   return( hypre_BoomerAMGSetChebyVariant( (void *) solver, variant ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetChebyEigEst
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetChebyEigEst( HYPRE_Solver  solver,
                                 HYPRE_Int     eig_est )
{
   return( hypre_BoomerAMGSetChebyEigEst( (void *) solver, eig_est ) );
}
/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAdditive, HYPRE_BoomerAMGGetAdditive
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAdditive( HYPRE_Solver solver,
                          HYPRE_Int          additive  )
{
   return( hypre_BoomerAMGSetAdditive( (void *) solver, additive ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetAdditive( HYPRE_Solver solver,
                          HYPRE_Int        * additive  )
{
   return( hypre_BoomerAMGGetAdditive( (void *) solver, additive ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetMultAdditive, HYPRE_BoomerAMGGetMultAdditive
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetMultAdditive( HYPRE_Solver solver,
                          HYPRE_Int          mult_additive  )
{
   return( hypre_BoomerAMGSetMultAdditive( (void *) solver, mult_additive ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetMultAdditive( HYPRE_Solver solver,
                          HYPRE_Int        * mult_additive  )
{
   return( hypre_BoomerAMGGetMultAdditive( (void *) solver, mult_additive ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetSimple, HYPRE_BoomerAMGGetSimple
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetSimple( HYPRE_Solver solver,
                          HYPRE_Int          simple  )
{
   return( hypre_BoomerAMGSetSimple( (void *) solver, simple ) );
}

HYPRE_Int
HYPRE_BoomerAMGGetSimple( HYPRE_Solver solver,
                          HYPRE_Int        * simple  )
{
   return( hypre_BoomerAMGGetSimple( (void *) solver, simple ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetAddLastLvl
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetAddLastLvl( HYPRE_Solver solver,
                          HYPRE_Int          add_last_lvl  )
{
   return( hypre_BoomerAMGSetAddLastLvl( (void *) solver, add_last_lvl ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetNonGalerkinTol
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetNonGalerkinTol (HYPRE_Solver solver,
                                HYPRE_Real  nongalerkin_tol)
{
   return (hypre_BoomerAMGSetNonGalerkinTol ( (void *) solver, nongalerkin_tol ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetLevelNonGalerkinTol
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetLevelNonGalerkinTol (HYPRE_Solver solver,
                                HYPRE_Real  nongalerkin_tol,
                                HYPRE_Int   level)
{
   return (hypre_BoomerAMGSetLevelNonGalerkinTol ( (void *) solver, nongalerkin_tol , level ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetNonGalerkTol
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetNonGalerkTol (HYPRE_Solver solver,
                                HYPRE_Int    nongalerk_num_tol,
                                HYPRE_Real  *nongalerk_tol)
{
   return (hypre_BoomerAMGSetNonGalerkTol ( (void *) solver, nongalerk_num_tol , nongalerk_tol ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetRAP2
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetRAP2 (HYPRE_Solver solver,
                                HYPRE_Int    rap2)
{
   return (hypre_BoomerAMGSetRAP2 ( (void *) solver, rap2 ) );
}

/*--------------------------------------------------------------------------
 * HYPRE_BoomerAMGSetKeepTranspose
 *--------------------------------------------------------------------------*/

HYPRE_Int
HYPRE_BoomerAMGSetKeepTranspose (HYPRE_Solver solver,
                                HYPRE_Int    keepTranspose)
{
   return (hypre_BoomerAMGSetKeepTranspose ( (void *) solver, keepTranspose ) );
}

