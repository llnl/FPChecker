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


#ifndef HYPRE_PARCSR_LS_HEADER
#define HYPRE_PARCSR_LS_HEADER

#include "HYPRE.h"
#include "HYPRE_utilities.h"
#include "HYPRE_seq_mv.h"
#include "HYPRE_parcsr_mv.h"
#include "HYPRE_IJ_mv.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/**
 * @name ParCSR Solvers
 *
 * These solvers use matrix/vector storage schemes that are taylored
 * for general sparse matrix systems.
 *
 * @memo Linear solvers for sparse matrix systems
 **/
/*@{*/

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/**
 * @name ParCSR Solvers
 **/
/*@{*/

struct hypre_Solver_struct;
/**
 * The solver object.
 **/

#ifndef HYPRE_SOLVER_STRUCT
#define HYPRE_SOLVER_STRUCT
struct hypre_Solver_struct;
typedef struct hypre_Solver_struct *HYPRE_Solver;
#endif

typedef HYPRE_Int (*HYPRE_PtrToParSolverFcn)(HYPRE_Solver,
                                       HYPRE_ParCSRMatrix,
                                       HYPRE_ParVector,
                                       HYPRE_ParVector);

#ifndef HYPRE_MODIFYPC
#define HYPRE_MODIFYPC
typedef HYPRE_Int (*HYPRE_PtrToModifyPCFcn)(HYPRE_Solver,
                                      HYPRE_Int,
                                      HYPRE_Real);
#endif

/*@}*/

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/**
 * @name ParCSR BoomerAMG Solver and Preconditioner
 * 
 * Parallel unstructured algebraic multigrid solver and preconditioner
 **/
/*@{*/

/**
 * Create a solver object.
 **/
HYPRE_Int HYPRE_BoomerAMGCreate(HYPRE_Solver *solver);

/**
 * Destroy a solver object.
 **/
HYPRE_Int HYPRE_BoomerAMGDestroy(HYPRE_Solver solver);

/**
 * Set up the BoomerAMG solver or preconditioner.  
 * If used as a preconditioner, this function should be passed
 * to the iterative solver {\tt SetPrecond} function.
 *
 * @param solver [IN] object to be set up.
 * @param A [IN] ParCSR matrix used to construct the solver/preconditioner.
 * @param b Ignored by this function.
 * @param x Ignored by this function.
 **/
HYPRE_Int HYPRE_BoomerAMGSetup(HYPRE_Solver       solver,
                               HYPRE_ParCSRMatrix A,
                               HYPRE_ParVector    b,
                               HYPRE_ParVector    x);

/**
 * Solve the system or apply AMG as a preconditioner.
 * If used as a preconditioner, this function should be passed
 * to the iterative solver {\tt SetPrecond} function.
 *
 * @param solver [IN] solver or preconditioner object to be applied.
 * @param A [IN] ParCSR matrix, matrix of the linear system to be solved
 * @param b [IN] right hand side of the linear system to be solved
 * @param x [OUT] approximated solution of the linear system to be solved
 **/
HYPRE_Int HYPRE_BoomerAMGSolve(HYPRE_Solver       solver,
                               HYPRE_ParCSRMatrix A,
                               HYPRE_ParVector    b,
                               HYPRE_ParVector    x);

/**
 * Solve the transpose system $A^T x = b$ or apply AMG as a preconditioner
 * to the transpose system . Note that this function should only be used
 * when preconditioning CGNR with BoomerAMG. It can only be used with
 * Jacobi smoothing (relax_type 0 or 7) and without CF smoothing,
 * i.e relax_order needs to be set to 0.
 * If used as a preconditioner, this function should be passed
 * to the iterative solver {\tt SetPrecond} function.
 *
 * @param solver [IN] solver or preconditioner object to be applied.
 * @param A [IN] ParCSR matrix 
 * @param b [IN] right hand side of the linear system to be solved
 * @param x [OUT] approximated solution of the linear system to be solved
 **/
HYPRE_Int HYPRE_BoomerAMGSolveT(HYPRE_Solver       solver,
                                HYPRE_ParCSRMatrix A,
                                HYPRE_ParVector    b,
                                HYPRE_ParVector    x);

/**
 * Recovers old default for coarsening and interpolation, i.e Falgout 
 * coarsening and untruncated modified classical interpolation.
 * This option might be preferred for 2 dimensional problems.
 **/
HYPRE_Int HYPRE_BoomerAMGSetOldDefault(HYPRE_Solver       solver);

/**
 * Returns the residual.
 **/
HYPRE_Int HYPRE_BoomerAMGGetResidual(HYPRE_Solver     solver,
                                     HYPRE_ParVector *residual);

/**
 * Returns the number of iterations taken.
 **/
HYPRE_Int HYPRE_BoomerAMGGetNumIterations(HYPRE_Solver  solver,
                                          HYPRE_Int          *num_iterations);

/**
 * Returns the cumulative nnz of A*P (used for the FOM computation).
 * Upstream declares this only in the internal header _hypre_parcsr_ls.h,
 * but test/amg.c uses it while including only public headers.  gcc treated
 * the implicit declaration as a warning; clang 16+ makes it an error.
 **/
HYPRE_Int HYPRE_BoomerAMGGetCumNnzAP(HYPRE_Solver  solver,
                                     HYPRE_Real   *cum_nnz_AP);

/**
 * Returns the norm of the final relative residual.
 **/
HYPRE_Int HYPRE_BoomerAMGGetFinalRelativeResidualNorm(HYPRE_Solver  solver,
                                                      HYPRE_Real   *rel_resid_norm);

/**
 * (Optional) Sets the size of the system of PDEs, if using the systems version.
 * The default is 1, i.e. a scalar system.
 **/
HYPRE_Int HYPRE_BoomerAMGSetNumFunctions(HYPRE_Solver solver,
                                         HYPRE_Int          num_functions);

/**
 * (Optional) Sets the mapping that assigns the function to each variable, 
 * if using the systems version. If no assignment is made and the number of
 * functions is k > 1, the mapping generated is (0,1,...,k-1,0,1,...,k-1,...).
 **/
HYPRE_Int HYPRE_BoomerAMGSetDofFunc(HYPRE_Solver  solver,
                                    HYPRE_Int    *dof_func);

/**
 * (Optional) Set the convergence tolerance, if BoomerAMG is used
 * as a solver. If it is used as a preconditioner, it should be set to 0.
 * The default is 1.e-7.
 **/
HYPRE_Int HYPRE_BoomerAMGSetTol(HYPRE_Solver solver,
                                HYPRE_Real   tol);

/**
 * (Optional) Sets maximum number of iterations, if BoomerAMG is used
 * as a solver. If it is used as a preconditioner, it should be set to 1.
 * The default is 20.
 **/
HYPRE_Int HYPRE_BoomerAMGSetMaxIter(HYPRE_Solver solver,
                                    HYPRE_Int          max_iter);

/**
 * (Optional)
 **/
HYPRE_Int HYPRE_BoomerAMGSetMinIter(HYPRE_Solver solver,
                                    HYPRE_Int    min_iter);

/**
 * (Optional) Sets maximum size of coarsest grid.
 * The default is 9.
 **/
HYPRE_Int HYPRE_BoomerAMGSetMaxCoarseSize(HYPRE_Solver solver,
                                          HYPRE_Int    max_coarse_size);

/**
 * (Optional) Sets minimum size of coarsest grid.
 * The default is 1.
 **/
HYPRE_Int HYPRE_BoomerAMGSetMinCoarseSize(HYPRE_Solver solver,
                                          HYPRE_Int    min_coarse_size);

/**
 * (Optional) Sets maximum number of multigrid levels.
 * The default is 25.
 **/
HYPRE_Int HYPRE_BoomerAMGSetMaxLevels(HYPRE_Solver solver,
                                      HYPRE_Int    max_levels);

/**
 * (Optional) Sets AMG strength threshold. The default is 0.25.
 * For 2d Laplace operators, 0.25 is a good value, for 3d Laplace
 * operators, 0.5 or 0.6 is a better value. For elasticity problems,
 * a large strength threshold, such as 0.9, is often better.
 **/
HYPRE_Int HYPRE_BoomerAMGSetStrongThreshold(HYPRE_Solver solver,
                                            HYPRE_Real   strong_threshold);

/**
 * (Optional) Defines the largest strength threshold for which 
 * the strength matrix S uses the communication package of the operator A.
 * If the strength threshold is larger than this values,
 * a communication package is generated for S. This can save
 * memory and decrease the amount of data that needs to be communicated,
 * if S is substantially sparser than A.
 * The default is 1.0.
 **/
HYPRE_Int HYPRE_BoomerAMGSetSCommPkgSwitch(HYPRE_Solver solver,
                                           HYPRE_Real   S_commpkg_switch);

/**
 * (Optional) Sets a parameter to modify the definition of strength for
 * diagonal dominant portions of the matrix. The default is 0.9.
 * If max\_row\_sum is 1, no checking for diagonally dominant rows is
 * performed.
 **/
HYPRE_Int HYPRE_BoomerAMGSetMaxRowSum(HYPRE_Solver solver,
                                      HYPRE_Real    max_row_sum);

/**
 * (Optional) Defines which parallel coarsening algorithm is used.
 * There are the following options for coarsen\_type: 
 * 
 * \begin{tabular}{|c|l|} \hline
 * 0 & CLJP-coarsening (a parallel coarsening algorithm using independent sets. \\
 * 1 & classical Ruge-Stueben coarsening on each processor, no boundary treatment (not recommended!) \\
 * 3 & classical Ruge-Stueben coarsening on each processor, followed by a third pass, which adds coarse \\
 * & points on the boundaries \\
 * 6 & Falgout coarsening (uses 1 first, followed by CLJP using the interior coarse points \\
 * & generated by 1 as its first independent set) \\
 * 7 & CLJP-coarsening (using a fixed random vector, for debugging purposes only) \\
 * 8 & PMIS-coarsening (a parallel coarsening algorithm using independent sets, generating \\
 * & lower complexities than CLJP, might also lead to slower convergence) \\
 * 9 & PMIS-coarsening (using a fixed random vector, for debugging purposes only) \\
 * 10 & HMIS-coarsening (uses one pass Ruge-Stueben on each processor independently, followed \\
 * & by PMIS using the interior C-points generated as its first independent set) \\
 * 11 & one-pass Ruge-Stueben coarsening on each processor, no boundary treatment (not recommended!) \\
 * 21 & CGC coarsening by M. Griebel, B. Metsch and A. Schweitzer \\
 * 22 & CGC-E coarsening by M. Griebel, B. Metsch and A.Schweitzer \\
 * \hline
 * \end{tabular}
 * 
 * The default is 6. 
 **/
HYPRE_Int HYPRE_BoomerAMGSetCoarsenType(HYPRE_Solver solver,
                                        HYPRE_Int    coarsen_type);

/**
 * (Optional) Defines the non-Galerkin drop-tolerance
 * for sparsifying coarse grid operators and thus reducing communication. 
 * Value specified here is set on all levels.
 * This routine should be used before HYPRE_BoomerAMGSetLevelNonGalerkinTol, which 
 * then can be used to change individual levels if desired
 **/
HYPRE_Int HYPRE_BoomerAMGSetNonGalerkinTol (HYPRE_Solver solver,
                                          HYPRE_Real  nongalerkin_tol);

/**
 * (Optional) Defines the level specific non-Galerkin drop-tolerances
 * for sparsifying coarse grid operators and thus reducing communication. 
 * A drop-tolerance of 0.0 means to skip doing non-Galerkin on that 
 * level.  The maximum drop tolerance for a level is 1.0, although 
 * much smaller values such as 0.03 or 0.01 are recommended.
 *
 * Note that if the user wants to set a  specific tolerance on all levels,
 * HYPRE_BooemrAMGSetNonGalerkinTol should be used. Individual levels
 * can then be changed using this routine.
 *
 * In general, it is safer to drop more aggressively on coarser levels.  
 * For instance, one could use 0.0 on the finest level, 0.01 on the second level and
 * then using 0.05 on all remaining levels. The best way to achieve this is
 * to set 0.05 on all levels with HYPRE_BoomerAMGSetNonGalerkinTol and then
 * change the tolerance on level 0 to 0.0 and the tolerance on level 1 to 0.01
 * with HYPRE_BoomerAMGSetLevelNonGalerkinTol.
 * Like many AMG parameters, these drop tolerances can be tuned.  It is also common 
 * to delay the start of the non-Galerkin process further to a later level than
 * level 1.
 *
 * @param solver [IN] solver or preconditioner object to be applied.
 * @param nongalerkin_tol [IN] level specific drop tolerance
 * @param level [IN] level on which drop tolerance is used
 **/
HYPRE_Int HYPRE_BoomerAMGSetLevelNonGalerkinTol (HYPRE_Solver solver,
                                          HYPRE_Real   nongalerkin_tol,
                                          HYPRE_Int  level);
/*
 * (Optional) Defines the non-Galerkin drop-tolerance (old version)
 **/
HYPRE_Int HYPRE_BoomerAMGSetNonGalerkTol (HYPRE_Solver solver,
                                          HYPRE_Int    nongalerk_num_tol,
                                          HYPRE_Real  *nongalerk_tol);

/**
 * (Optional) Defines whether local or global measures are used.
 **/
HYPRE_Int HYPRE_BoomerAMGSetMeasureType(HYPRE_Solver solver,
                                        HYPRE_Int    measure_type);

/**
 * (Optional) Defines the number of levels of aggressive coarsening.
 * The default is 0, i.e. no aggressive coarsening.
 **/
HYPRE_Int HYPRE_BoomerAMGSetAggNumLevels(HYPRE_Solver solver,
                                         HYPRE_Int    agg_num_levels);

/**
 * (Optional) Defines the degree of aggressive coarsening.
 * The default is 1. Larger numbers lead to less aggressive 
 * coarsening.
 **/
HYPRE_Int HYPRE_BoomerAMGSetNumPaths(HYPRE_Solver solver,
                                     HYPRE_Int    num_paths);

/**
 * (optional) Defines the number of pathes for CGC-coarsening.
 **/
HYPRE_Int HYPRE_BoomerAMGSetCGCIts (HYPRE_Solver solver,
                                    HYPRE_Int    its);

/**
 * (Optional) Sets whether to use the nodal systems coarsening.
 * Should be used for linear systems generated from systems of PDEs.
 * The default is 0 (unknown-based coarsening, 
 *                   only coarsens within same function).
 * For the remaining options a nodal matrix is generated by
 * applying a norm to the nodal blocks and applying the coarsening
 * algorithm to this matrix.
 * \begin{tabular}{|c|l|} \hline
 * 1  & Frobenius norm \\
 * 2  & sum of absolute values of elements in each block \\
 * 3  & largest element in each block (not absolute value)\\
 * 4  & row-sum norm \\
 * 6  & sum of all values in each block \\
 * \hline
 * \end{tabular}
 **/
HYPRE_Int HYPRE_BoomerAMGSetNodal(HYPRE_Solver solver,
                                  HYPRE_Int    nodal);
/**
 * (Optional) Sets whether to give special treatment to diagonal elements in 
 * the nodal systems version.
 * The default is 0.
 * If set to 1, the diagonal entry is set to the negative sum of all off
 * diagonal entries.
 * If set to 2, the signs of all diagonal entries are inverted.
 */
HYPRE_Int HYPRE_BoomerAMGSetNodalDiag(HYPRE_Solver solver,
                                      HYPRE_Int    nodal_diag);


/**
 * (Optional) Defines which parallel interpolation operator is used.
 * There are the following options for interp\_type: 
 * 
 * \begin{tabular}{|c|l|} \hline
 * 0  & classical modified interpolation \\
 * 1  & LS interpolation (for use with GSMG) \\
 * 2  & classical modified interpolation for hyperbolic PDEs \\
 * 3  & direct interpolation (with separation of weights) \\
 * 4  & multipass interpolation \\
 * 5  & multipass interpolation (with separation of weights) \\
 * 6  & extended+i interpolation \\
 * 7  & extended+i (if no common C neighbor) interpolation \\
 * 8  & standard interpolation \\
 * 9  & standard interpolation (with separation of weights) \\
 * 10 & classical block interpolation (for use with nodal systems version only) \\
 * 11 & classical block interpolation (for use with nodal systems version only) \\
 *    & with diagonalized diagonal blocks \\
 * 12 & FF interpolation \\
 * 13 & FF1 interpolation \\
 * 14 & extended interpolation \\
 * \hline
 * \end{tabular}
 * 
 * The default is 0. 
 **/
HYPRE_Int HYPRE_BoomerAMGSetInterpType(HYPRE_Solver solver,
                                       HYPRE_Int    interp_type);

/**
 * (Optional) Defines a truncation factor for the interpolation.
 * The default is 0.
 **/
HYPRE_Int HYPRE_BoomerAMGSetTruncFactor(HYPRE_Solver solver,
                                        HYPRE_Real   trunc_factor);

/**
 * (Optional) Defines the maximal number of elements per row for the interpolation.
 * The default is 0.
 **/
HYPRE_Int HYPRE_BoomerAMGSetPMaxElmts(HYPRE_Solver solver,
                                      HYPRE_Int    P_max_elmts);

/**
 * (Optional) Defines whether separation of weights is used
 * when defining strength for standard interpolation or
 * multipass interpolation.
 * Default: 0, i.e. no separation of weights used.
 **/
HYPRE_Int HYPRE_BoomerAMGSetSepWeight(HYPRE_Solver solver,
                                      HYPRE_Int    sep_weight);

/**
 * (Optional) Defines the interpolation used on levels of aggressive coarsening
 * The default is 4, i.e. multipass interpolation.
 * The following options exist:
 * 
 * \begin{tabular}{|c|l|} \hline
 * 1 & 2-stage extended+i interpolation \\
 * 2 & 2-stage standard interpolation \\
 * 3 & 2-stage extended interpolation \\
 * 4 & multipass interpolation \\
 * \hline
 * \end{tabular}
 **/
HYPRE_Int HYPRE_BoomerAMGSetAggInterpType(HYPRE_Solver solver,
                                          HYPRE_Int    agg_interp_type);

/**
 * (Optional) Defines the truncation factor for the 
 * interpolation used for aggressive coarsening.
 * The default is 0.
 **/
HYPRE_Int HYPRE_BoomerAMGSetAggTruncFactor(HYPRE_Solver solver,
                                           HYPRE_Real   agg_trunc_factor);

/**
 * (Optional) Defines the truncation factor for the 
 * matrices P1 and P2 which are used to build 2-stage interpolation.
 * The default is 0.
 **/
HYPRE_Int HYPRE_BoomerAMGSetAggP12TruncFactor(HYPRE_Solver solver,
                                              HYPRE_Real   agg_P12_trunc_factor);

/**
 * (Optional) Defines the maximal number of elements per row for the 
 * interpolation used for aggressive coarsening.
 * The default is 0.
 **/
HYPRE_Int HYPRE_BoomerAMGSetAggPMaxElmts(HYPRE_Solver solver,
                                         HYPRE_Int    agg_P_max_elmts);

/**
 * (Optional) Defines the maximal number of elements per row for the 
 * matrices P1 and P2 which are used to build 2-stage interpolation.
 * The default is 0.
 **/
HYPRE_Int HYPRE_BoomerAMGSetAggP12MaxElmts(HYPRE_Solver solver,
                                           HYPRE_Int    agg_P12_max_elmts);

/**
 * (Optional) Allows the user to incorporate additional vectors 
 * into the interpolation for systems AMG, e.g. rigid body modes for 
 * linear elasticity problems.
 * This can only be used in context with nodal coarsening and still
 * requires the user to choose an interpolation. 
 **/
HYPRE_Int HYPRE_BoomerAMGSetInterpVectors (HYPRE_Solver     solver , 
                                           HYPRE_Int        num_vectors ,
                                           HYPRE_ParVector *interp_vectors );

/**
 * (Optional) Defines the interpolation variant used for 
 * HYPRE_BoomerAMGSetInterpVectors:
 * \begin{tabular}{|c|l|} \hline
 * 1 & GM approach 1 \\
 * 2 & GM approach 2  (to be preferred over 1) \\
 * 3 & LN approach \\
 * \hline
 * \end{tabular}
 **/
HYPRE_Int HYPRE_BoomerAMGSetInterpVecVariant (HYPRE_Solver solver, 
                                              HYPRE_Int    var );

/**
 * (Optional) Defines the maximal elements per row for Q, the additional
 * columns added to the original interpolation matrix P, to reduce complexity.
 * The default is no truncation.
 **/
HYPRE_Int HYPRE_BoomerAMGSetInterpVecQMax (HYPRE_Solver solver, 
                                           HYPRE_Int    q_max );

/**
 * (Optional) Defines a truncation factor for Q, the additional
 * columns added to the original interpolation matrix P, to reduce complexity.
 * The default is no truncation.
 **/
HYPRE_Int HYPRE_BoomerAMGSetInterpVecAbsQTrunc (HYPRE_Solver solver, 
                                                HYPRE_Real   q_trunc );

/**
 * (Optional) Specifies the use of GSMG - geometrically smooth 
 * coarsening and interpolation. Currently any nonzero value for
 * gsmg will lead to the use of GSMG.
 * The default is 0, i.e. (GSMG is not used)
 **/
HYPRE_Int HYPRE_BoomerAMGSetGSMG(HYPRE_Solver solver,
                                 HYPRE_Int    gsmg);

/**
 * (Optional) Defines the number of sample vectors used in GSMG
 * or LS interpolation.
 **/
HYPRE_Int HYPRE_BoomerAMGSetNumSamples(HYPRE_Solver solver,
                                       HYPRE_Int    num_samples);
/**
 * (Optional) Defines the type of cycle.
 * For a V-cycle, set cycle\_type to 1, for a W-cycle
 *  set cycle\_type to 2. The default is 1.
 **/
HYPRE_Int HYPRE_BoomerAMGSetCycleType(HYPRE_Solver solver,
                                      HYPRE_Int    cycle_type);

/**
 * (Optional) Defines use of an additive V(1,1)-cycle using the
 * classical additive method starting at level 'addlvl'.
 * The multiplicative approach is used on levels 0, ...'addlvl+1'.
 * 'addlvl' needs to be > -1 for this to have an effect.
 * Can only be used with weighted Jacobi and l1-Jacobi(default).
 * 
 * Can only be used when AMG is used as a preconditioner !!!
 **/
HYPRE_Int HYPRE_BoomerAMGSetAdditive(HYPRE_Solver solver,
                                     HYPRE_Int    addlvl);

/**
 * (Optional) Defines use of an additive V(1,1)-cycle using the
 * mult-additive method starting at level 'addlvl'.
 * The multiplicative approach is used on levels 0, ...'addlvl+1'.
 * 'addlvl' needs to be > -1 for this to have an effect.
 * Can only be used with weighted Jacobi and l1-Jacobi(default).
 * 
 * Can only be used when AMG is used as a preconditioner !!!
 **/
HYPRE_Int HYPRE_BoomerAMGSetMultAdditive(HYPRE_Solver solver,
                                         HYPRE_Int    addlvl);

/**
 * (Optional) Defines use of an additive V(1,1)-cycle using the
 * simplified mult-additive method starting at level 'addlvl'.
 * The multiplicative approach is used on levels 0, ...'addlvl+1'.
 * 'addlvl' needs to be > -1 for this to have an effect.
 * Can only be used with weighted Jacobi and l1-Jacobi(default).
 * 
 * Can only be used when AMG is used as a preconditioner !!!
 **/
HYPRE_Int HYPRE_BoomerAMGSetSimple(HYPRE_Solver solver,
                                   HYPRE_Int    addlvl);

/**
 * (Optional) Defines last level where additive, mult-additive
 * or simple cycle is used.
 * The multiplicative approach is used on levels > add_last_lvl.
 * 
 * Can only be used when AMG is used as a preconditioner !!!
 **/
HYPRE_Int HYPRE_BoomerAMGSetAddLastLvl(HYPRE_Solver solver,
                                     HYPRE_Int    add_last_lvl);

/**
 * (Optional) Defines the truncation factor for the 
 * smoothed interpolation used for mult-additive or simple method.
 * The default is 0.
 **/
HYPRE_Int HYPRE_BoomerAMGSetMultAddTruncFactor(HYPRE_Solver solver,
                                           HYPRE_Real   add_trunc_factor);

/**
 * (Optional) Defines the maximal number of elements per row for the 
 * smoothed interpolation used for mult-additive or simple method.
 * The default is 0.
 **/
HYPRE_Int HYPRE_BoomerAMGSetMultAddPMaxElmts(HYPRE_Solver solver,
                                         HYPRE_Int    add_P_max_elmts);
/**
 * (Optional) Defines the relaxation type used in the (mult)additive cycle
 * portion (also affects simple method.) 
 * The default is 18 (L1-Jacobi).
 * Currently the only other option allowed is 0 (Jacobi) which should be 
 **/
HYPRE_Int HYPRE_BoomerAMGSetAddRelaxType(HYPRE_Solver solver,
                                         HYPRE_Int    add_rlx_type);

/**
 * (Optional) Defines the relaxation weight used for Jacobi within the 
 * (mult)additive or simple cycle portion.
 * The default is 1. 
 * The weight only affects the Jacobi method, and has no effect on L1-Jacobi
 **/
HYPRE_Int HYPRE_BoomerAMGSetAddRelaxWt(HYPRE_Solver solver,
                                         HYPRE_Real    add_rlx_wt);

/**
 * (Optional) Sets maximal size for agglomeration or redundant coarse grid solve. 
 * When the system is smaller than this threshold, sequential AMG is used 
 * on process 0 or on all remaining active processes (if redundant = 1 ).
 **/
HYPRE_Int HYPRE_BoomerAMGSetSeqThreshold(HYPRE_Solver solver,
                                         HYPRE_Int    seq_threshold);
/**
 * (Optional) operates switch for redundancy. Needs to be used with
 * HYPRE_BoomerAMGSetSeqThreshold. Default is 0, i.e. no redundancy.
 **/
HYPRE_Int HYPRE_BoomerAMGSetRedundant(HYPRE_Solver solver,
                                      HYPRE_Int    redundant);

/*
 * (Optional) Defines the number of sweeps for the fine and coarse grid, 
 * the up and down cycle.
 *
 * Note: This routine will be phased out!!!!
 * Use HYPRE\_BoomerAMGSetNumSweeps or HYPRE\_BoomerAMGSetCycleNumSweeps instead.
 **/
HYPRE_Int HYPRE_BoomerAMGSetNumGridSweeps(HYPRE_Solver  solver,
                                          HYPRE_Int    *num_grid_sweeps);

/**
 * (Optional) Sets the number of sweeps. On the finest level, the up and 
 * the down cycle the number of sweeps are set to num\_sweeps and on the 
 * coarsest level to 1. The default is 1.
 **/
HYPRE_Int HYPRE_BoomerAMGSetNumSweeps(HYPRE_Solver  solver,
                                      HYPRE_Int     num_sweeps);

/**
 * (Optional) Sets the number of sweeps at a specified cycle.
 * There are the following options for k:
 *
 * \begin{tabular}{|l|l|} \hline
 * the down cycle     & if k=1 \\
 * the up cycle       & if k=2 \\
 * the coarsest level & if k=3.\\
 * \hline
 * \end{tabular}
 **/
HYPRE_Int HYPRE_BoomerAMGSetCycleNumSweeps(HYPRE_Solver  solver,
                                           HYPRE_Int     num_sweeps,
                                           HYPRE_Int     k);

/**
 * (Optional) Defines which smoother is used on the fine and coarse grid, 
 * the up and down cycle.
 *
 * Note: This routine will be phased out!!!!
 * Use HYPRE\_BoomerAMGSetRelaxType or HYPRE\_BoomerAMGSetCycleRelaxType instead.
 **/
HYPRE_Int HYPRE_BoomerAMGSetGridRelaxType(HYPRE_Solver  solver,
                                          HYPRE_Int    *grid_relax_type);

/**
 * (Optional) Defines the smoother to be used. It uses the given
 * smoother on the fine grid, the up and 
 * the down cycle and sets the solver on the coarsest level to Gaussian
 * elimination (9). The default is Gauss-Seidel (3).
 *
 * There are the following options for relax\_type:
 *
 * \begin{tabular}{|c|l|} \hline
 * 0 & Jacobi \\
 * 1 & Gauss-Seidel, sequential (very slow!) \\
 * 2 & Gauss-Seidel, interior points in parallel, boundary sequential (slow!) \\
 * 3 & hybrid Gauss-Seidel or SOR, forward solve \\
 * 4 & hybrid Gauss-Seidel or SOR, backward solve \\
 * 5 & hybrid chaotic Gauss-Seidel (works only with OpenMP) \\
 * 6 & hybrid symmetric Gauss-Seidel or SSOR \\
 * 8 & $\ell_1$-scaled hybrid symmetric Gauss-Seidel\\
 * 9 & Gaussian elimination (only on coarsest level) \\
 * 15 & CG (warning - not a fixed smoother - may require FGMRES)\\
 * 16 & Chebyshev\\
 * 17 & FCF-Jacobi\\
 * 18 & $\ell_1$-scaled jacobi\\
 * \hline
 * \end{tabular}
 **/
HYPRE_Int HYPRE_BoomerAMGSetRelaxType(HYPRE_Solver  solver,
                                      HYPRE_Int     relax_type);

/**
 * (Optional) Defines the smoother at a given cycle.
 * For options of relax\_type see
 * description of HYPRE\_BoomerAMGSetRelaxType). Options for k are
 *
 * \begin{tabular}{|l|l|} \hline
 * the down cycle     & if k=1 \\
 * the up cycle       & if k=2 \\
 * the coarsest level & if k=3. \\
 * \hline
 * \end{tabular}
 **/
HYPRE_Int HYPRE_BoomerAMGSetCycleRelaxType(HYPRE_Solver  solver,
                                           HYPRE_Int     relax_type,
                                           HYPRE_Int     k);

/**
 * (Optional) Defines in which order the points are relaxed. There are
 * the following options for
 * relax\_order: 
 *
 * \begin{tabular}{|c|l|} \hline
 * 0 & the points are relaxed in natural or lexicographic
 *                   order on each processor \\
 * 1 &  CF-relaxation is used, i.e on the fine grid and the down
 *                   cycle the coarse points are relaxed first, \\
 * & followed by the fine points; on the up cycle the F-points are relaxed
 * first, followed by the C-points. \\
 * & On the coarsest level, if an iterative scheme is used, 
 * the points are relaxed in lexicographic order. \\
 * \hline
 * \end{tabular}
 *
 * The default is 1 (CF-relaxation).
 **/
HYPRE_Int HYPRE_BoomerAMGSetRelaxOrder(HYPRE_Solver  solver,
                                       HYPRE_Int     relax_order);

/*
 * (Optional) Defines in which order the points are relaxed. 
 *
 * Note: This routine will be phased out!!!!
 * Use HYPRE\_BoomerAMGSetRelaxOrder instead.
 **/
HYPRE_Int HYPRE_BoomerAMGSetGridRelaxPoints(HYPRE_Solver   solver,
                                            HYPRE_Int    **grid_relax_points);

/*
 * (Optional) Defines the relaxation weight for smoothed Jacobi and hybrid SOR.
 *
 * Note: This routine will be phased out!!!!
 * Use HYPRE\_BoomerAMGSetRelaxWt or HYPRE\_BoomerAMGSetLevelRelaxWt instead.
 **/
HYPRE_Int HYPRE_BoomerAMGSetRelaxWeight(HYPRE_Solver  solver,
                                        HYPRE_Real   *relax_weight); 
/**
 * (Optional) Defines the relaxation weight for smoothed Jacobi and hybrid SOR 
 * on all levels. 
 * 
 * \begin{tabular}{|l|l|} \hline
 * relax\_weight > 0 & this assigns the given relaxation weight on all levels \\
 * relax\_weight = 0 &  the weight is determined on each level
 *                       with the estimate $3 \over {4\|D^{-1/2}AD^{-1/2}\|}$,\\
 * & where $D$ is the diagonal matrix of $A$ (this should only be used with Jacobi) \\
 * relax\_weight = -k & the relaxation weight is determined with at most k CG steps
 *                       on each level \\
 * & this should only be used for symmetric positive definite problems) \\
 * \hline
 * \end{tabular} 
 * 
 * The default is 1.
 **/
HYPRE_Int HYPRE_BoomerAMGSetRelaxWt(HYPRE_Solver  solver,
                                    HYPRE_Real    relax_weight);

/**
 * (Optional) Defines the relaxation weight for smoothed Jacobi and hybrid SOR
 * on the user defined level. Note that the finest level is denoted 0, the
 * next coarser level 1, etc. For nonpositive relax\_weight, the parameter is
 * determined on the given level as described for HYPRE\_BoomerAMGSetRelaxWt. 
 * The default is 1.
 **/
HYPRE_Int HYPRE_BoomerAMGSetLevelRelaxWt(HYPRE_Solver  solver,
                                         HYPRE_Real    relax_weight,
                                         HYPRE_Int     level);

/**
 * (Optional) Defines the outer relaxation weight for hybrid SOR.
 * Note: This routine will be phased out!!!!
 * Use HYPRE\_BoomerAMGSetOuterWt or HYPRE\_BoomerAMGSetLevelOuterWt instead.
 **/
HYPRE_Int HYPRE_BoomerAMGSetOmega(HYPRE_Solver  solver,
                                  HYPRE_Real   *omega);

/**
 * (Optional) Defines the outer relaxation weight for hybrid SOR and SSOR
 * on all levels.
 * 
 * \begin{tabular}{|l|l|} \hline 
 * omega > 0 & this assigns the same outer relaxation weight omega on each level\\
 * omega = -k & an outer relaxation weight is determined with at most k CG
 *                steps on each level \\
 * & (this only makes sense for symmetric
 *                positive definite problems and smoothers, e.g. SSOR) \\
 * \hline
 * \end{tabular} 
 * 
 * The default is 1.
 **/
HYPRE_Int HYPRE_BoomerAMGSetOuterWt(HYPRE_Solver  solver,
                                    HYPRE_Real    omega);

/**
 * (Optional) Defines the outer relaxation weight for hybrid SOR or SSOR
 * on the user defined level. Note that the finest level is denoted 0, the
 * next coarser level 1, etc. For nonpositive omega, the parameter is
 * determined on the given level as described for HYPRE\_BoomerAMGSetOuterWt. 
 * The default is 1.
 **/
HYPRE_Int HYPRE_BoomerAMGSetLevelOuterWt(HYPRE_Solver  solver,
                                         HYPRE_Real    omega,
                                         HYPRE_Int     level);


/**
 * (Optional) Defines the Order for Chebyshev smoother.
 *  The default is 2 (valid options are 1-4).
 **/
HYPRE_Int HYPRE_BoomerAMGSetChebyOrder(HYPRE_Solver solver,
                                       HYPRE_Int    order);

/**
 * (Optional) Fraction of the spectrum to use for the Chebyshev smoother.
 *  The default is .3 (i.e., damp on upper 30% of the spectrum).
 **/
HYPRE_Int HYPRE_BoomerAMGSetChebyFraction (HYPRE_Solver solver,
                                           HYPRE_Real   ratio);

/*
 * (Optional) Defines whether matrix should be scaled.
 *  The default is 1 (i.e., scaled).
 **/
HYPRE_Int HYPRE_BoomerAMGSetChebyScale (HYPRE_Solver solver,
                                           HYPRE_Int   scale);
/*
 * (Optional) Defines which polynomial variant should be used.
 *  The default is 0 (i.e., scaled).
 **/
HYPRE_Int HYPRE_BoomerAMGSetChebyVariant (HYPRE_Solver solver,
                                           HYPRE_Int   variant);

/*
 * (Optional) Defines how to estimate eigenvalues.
 *  The default is 10 (i.e., 10 CG iterations are used to find extreme 
 *  eigenvalues.) If eig_est=0, the largest eigenvalue is estimated
 *  using Gershgorin, the smallest is set to 0.
 *  If eig_est is a positive number n, n iterations of CG are used to
 *  determine the smallest and largest eigenvalue.
 **/
HYPRE_Int HYPRE_BoomerAMGSetChebyEigEst (HYPRE_Solver solver,
                                           HYPRE_Int   eig_est);


/**
 * (Optional) Enables the use of more complex smoothers.
 * The following options exist for smooth\_type:
 *
 * \begin{tabular}{|c|l|l|} \hline
 * value & smoother & routines needed to set smoother parameters \\
 * 6 & Schwarz smoothers & HYPRE\_BoomerAMGSetDomainType, HYPRE\_BoomerAMGSetOverlap, \\
 *   &  & HYPRE\_BoomerAMGSetVariant, HYPRE\_BoomerAMGSetSchwarzRlxWeight \\
 * 7 & Pilut & HYPRE\_BoomerAMGSetDropTol, HYPRE\_BoomerAMGSetMaxNzPerRow \\
 * 8 & ParaSails & HYPRE\_BoomerAMGSetSym, HYPRE\_BoomerAMGSetLevel, \\
 *   &  &  HYPRE\_BoomerAMGSetFilter, HYPRE\_BoomerAMGSetThreshold \\
 * 9 & Euclid & HYPRE\_BoomerAMGSetEuclidFile \\
 * \hline
 * \end{tabular}
 *
 * The default is 6. Also, if no smoother parameters are set via the routines mentioned in the table above,
 * default values are used.
 **/
HYPRE_Int HYPRE_BoomerAMGSetSmoothType(HYPRE_Solver solver,
                                       HYPRE_Int    smooth_type);

/**
 * (Optional) Sets the number of levels for more complex smoothers.
 * The smoothers, 
 * as defined by HYPRE\_BoomerAMGSetSmoothType, will be used
 * on level 0 (the finest level) through level smooth\_num\_levels-1. 
 * The default is 0, i.e. no complex smoothers are used.
 **/
HYPRE_Int HYPRE_BoomerAMGSetSmoothNumLevels(HYPRE_Solver solver,
                                            HYPRE_Int    smooth_num_levels);

/**
 * (Optional) Sets the number of sweeps for more complex smoothers.
 * The default is 1.
 **/
HYPRE_Int HYPRE_BoomerAMGSetSmoothNumSweeps(HYPRE_Solver solver,
                                            HYPRE_Int    smooth_num_sweeps);

/**
 * (Optional) Defines which variant of the Schwarz method is used.
 * The following options exist for variant:
 * 
 * \begin{tabular}{|c|l|} \hline
 * 0 & hybrid multiplicative Schwarz method (no overlap across processor 
 *    boundaries) \\
 * 1 & hybrid additive Schwarz method (no overlap across processor 
 *    boundaries) \\
 * 2 & additive Schwarz method \\
 * 3 & hybrid multiplicative Schwarz method (with overlap across processor 
 *    boundaries) \\
 * \hline
 * \end{tabular}
 *
 * The default is 0.
 **/
HYPRE_Int HYPRE_BoomerAMGSetVariant(HYPRE_Solver solver,
                                    HYPRE_Int    variant);

/**
 * (Optional) Defines the overlap for the Schwarz method.
 * The following options exist for overlap:
 *
 * \begin{tabular}{|c|l|} \hline
 * 0  & no overlap \\
 * 1  & minimal overlap (default) \\
 * 2  & overlap generated by including all neighbors of domain boundaries \\
 * \hline
 * \end{tabular}
 **/
HYPRE_Int HYPRE_BoomerAMGSetOverlap(HYPRE_Solver solver,
                                    HYPRE_Int    overlap);

/**
 * (Optional) Defines the type of domain used for the Schwarz method.
 * The following options exist for domain\_type:
 *
 * \begin{tabular}{|c|l|} \hline
 * 0 &  each point is a domain \\
 * 1 &  each node is a domain (only of interest in "systems" AMG) \\
 * 2 &  each domain is generated by agglomeration (default) \\
 * \hline
 * \end{tabular}
 **/
HYPRE_Int HYPRE_BoomerAMGSetDomainType(HYPRE_Solver solver,
                                       HYPRE_Int    domain_type);

/**
 * (Optional) Defines a smoothing parameter for the additive Schwarz method.
 **/
HYPRE_Int HYPRE_BoomerAMGSetSchwarzRlxWeight(HYPRE_Solver solver,
                                             HYPRE_Real   schwarz_rlx_weight);

/**
 *  (Optional) Indicates that the aggregates may not be SPD for the Schwarz method.
 * The following options exist for use\_nonsymm:
 *
 * \begin{tabular}{|c|l|} \hline
 * 0  & assume SPD (default) \\
 * 1  & assume non-symmetric \\
 * \hline
 * \end{tabular}
**/
HYPRE_Int HYPRE_BoomerAMGSetSchwarzUseNonSymm(HYPRE_Solver solver,
                                              HYPRE_Int    use_nonsymm);

/**
 * (Optional) Defines symmetry for ParaSAILS. 
 * For further explanation see description of ParaSAILS.
 **/
HYPRE_Int HYPRE_BoomerAMGSetSym(HYPRE_Solver solver,
                                HYPRE_Int    sym);

/**
 * (Optional) Defines number of levels for ParaSAILS.
 * For further explanation see description of ParaSAILS.
 **/
HYPRE_Int HYPRE_BoomerAMGSetLevel(HYPRE_Solver solver,
                                  HYPRE_Int    level);

/**
 * (Optional) Defines threshold for ParaSAILS.
 * For further explanation see description of ParaSAILS.
 **/
HYPRE_Int HYPRE_BoomerAMGSetThreshold(HYPRE_Solver solver,
                                      HYPRE_Real   threshold);

/**
 * (Optional) Defines filter for ParaSAILS.
 * For further explanation see description of ParaSAILS.
 **/
HYPRE_Int HYPRE_BoomerAMGSetFilter(HYPRE_Solver solver,
                                   HYPRE_Real   filter);

/**
 * (Optional) Defines drop tolerance for PILUT.
 * For further explanation see description of PILUT.
 **/
HYPRE_Int HYPRE_BoomerAMGSetDropTol(HYPRE_Solver solver,
                                    HYPRE_Real   drop_tol);

/**
 * (Optional) Defines maximal number of nonzeros for PILUT.
 * For further explanation see description of PILUT.
 **/
HYPRE_Int HYPRE_BoomerAMGSetMaxNzPerRow(HYPRE_Solver solver,
                                        HYPRE_Int    max_nz_per_row);

/**
 * (Optional) Defines name of an input file for Euclid parameters.
 * For further explanation see description of Euclid.
 **/
HYPRE_Int HYPRE_BoomerAMGSetEuclidFile(HYPRE_Solver  solver,
                                       char         *euclidfile); 

/**
 * (Optional) Defines number of levels for ILU(k) in Euclid.
 * For further explanation see description of Euclid.
 **/
HYPRE_Int HYPRE_BoomerAMGSetEuLevel(HYPRE_Solver solver,
                                    HYPRE_Int    eu_level);

/**
 * (Optional) Defines filter for ILU(k) for Euclid.
 * For further explanation see description of Euclid.
 **/
HYPRE_Int HYPRE_BoomerAMGSetEuSparseA(HYPRE_Solver solver,
                                      HYPRE_Real   eu_sparse_A);

/**
 * (Optional) Defines use of block jacobi ILUT for Euclid.
 * For further explanation see description of Euclid.
 **/
HYPRE_Int HYPRE_BoomerAMGSetEuBJ(HYPRE_Solver solver,
                                 HYPRE_Int    eu_bj);

/*
 * (Optional)
 **/
HYPRE_Int HYPRE_BoomerAMGSetRestriction(HYPRE_Solver solver,
                                        HYPRE_Int    restr_par);

/*
 * (Optional) Name of file to which BoomerAMG will print;
 * cf HYPRE\_BoomerAMGSetPrintLevel.  (Presently this is ignored).
 **/
HYPRE_Int HYPRE_BoomerAMGSetPrintFileName(HYPRE_Solver  solver,
                                          const char   *print_file_name);

/**
 * (Optional) Requests automatic printing of setup and solve information.
 *
 * \begin{tabular}{|c|l|} \hline
 * 0 & no printout (default) \\
 * 1 & print setup information \\
 * 2 & print solve information \\
 * 3 & print both setup and solve information \\
 * \hline
 * \end{tabular}
 *
 * Note, that if one desires to print information and uses BoomerAMG as a 
 * preconditioner, suggested print$\_$level is 1 to avoid excessive output,
 * and use print$\_$level of solver for solve phase information.
 **/
HYPRE_Int HYPRE_BoomerAMGSetPrintLevel(HYPRE_Solver solver,
                                       HYPRE_Int    print_level);

/**
 * (Optional) Requests additional computations for diagnostic and similar
 * data to be logged by the user. Default to 0 for do nothing.  The latest
 * residual will be available if logging > 1.
 **/
HYPRE_Int HYPRE_BoomerAMGSetLogging(HYPRE_Solver solver,
                                    HYPRE_Int    logging);


/**
 * (Optional)
 **/
HYPRE_Int HYPRE_BoomerAMGSetDebugFlag(HYPRE_Solver solver,
                                      HYPRE_Int    debug_flag);

/**
 * (Optional) This routine will be eliminated in the future.
 **/
HYPRE_Int HYPRE_BoomerAMGInitGridRelaxation(HYPRE_Int    **num_grid_sweeps_ptr,
                                            HYPRE_Int    **grid_relax_type_ptr,
                                            HYPRE_Int   ***grid_relax_points_ptr,
                                            HYPRE_Int      coarsen_type,
                                            HYPRE_Real **relax_weights_ptr,
                                            HYPRE_Int      max_levels);

/**
 * (Optional) If rap2 not equal 0, the triple matrix product RAP is
 * replaced by two matrix products.
 **/
HYPRE_Int HYPRE_BoomerAMGSetRAP2(HYPRE_Solver solver,
                                      HYPRE_Int    rap2);

/**
 * (Optional) If set to 1, the local interpolation transposes will
 * be saved to use more efficient matvecs instead of matvecTs
 **/
HYPRE_Int HYPRE_BoomerAMGSetKeepTranspose(HYPRE_Solver solver,
                                      HYPRE_Int    keepTranspose);

/*@}*/

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/**
 * @name ParCSR PCG Solver
 *
 * These routines should be used in conjunction with the generic interface in
 * \Ref{PCG Solver}.
 **/
/*@{*/

/**
 * Create a solver object.
 **/
HYPRE_Int HYPRE_ParCSRPCGCreate(MPI_Comm      comm,
                                HYPRE_Solver *solver);

/**
 * Destroy a solver object.
 **/
HYPRE_Int HYPRE_ParCSRPCGDestroy(HYPRE_Solver solver);

HYPRE_Int HYPRE_ParCSRPCGSetup(HYPRE_Solver       solver,
                               HYPRE_ParCSRMatrix A,
                               HYPRE_ParVector    b,
                               HYPRE_ParVector    x);
   
HYPRE_Int HYPRE_ParCSRPCGSolve(HYPRE_Solver       solver,
                               HYPRE_ParCSRMatrix A,
                               HYPRE_ParVector    b,
                               HYPRE_ParVector    x);

HYPRE_Int HYPRE_ParCSRPCGSetTol(HYPRE_Solver solver,
                                HYPRE_Real   tol);

HYPRE_Int HYPRE_ParCSRPCGSetAbsoluteTol(HYPRE_Solver solver,
                                  HYPRE_Real   tol);

HYPRE_Int HYPRE_ParCSRPCGSetMaxIter(HYPRE_Solver solver,
                                    HYPRE_Int    max_iter);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_ParCSRPCGSetStopCrit(HYPRE_Solver solver,
                                     HYPRE_Int    stop_crit);

HYPRE_Int HYPRE_ParCSRPCGSetTwoNorm(HYPRE_Solver solver,
                                    HYPRE_Int    two_norm);

HYPRE_Int HYPRE_ParCSRPCGSetRelChange(HYPRE_Solver solver,
                                      HYPRE_Int    rel_change);

HYPRE_Int HYPRE_ParCSRPCGSetPrecond(HYPRE_Solver            solver,
                                    HYPRE_PtrToParSolverFcn precond,
                                    HYPRE_PtrToParSolverFcn precond_setup,
                                    HYPRE_Solver            precond_solver);

HYPRE_Int HYPRE_ParCSRPCGGetPrecond(HYPRE_Solver  solver,
                                    HYPRE_Solver *precond_data);

HYPRE_Int HYPRE_ParCSRPCGSetLogging(HYPRE_Solver solver,
                                    HYPRE_Int    logging);

HYPRE_Int HYPRE_ParCSRPCGSetPrintLevel(HYPRE_Solver solver,
                                       HYPRE_Int    print_level);

HYPRE_Int HYPRE_ParCSRPCGGetNumIterations(HYPRE_Solver  solver,
                                          HYPRE_Int    *num_iterations);

HYPRE_Int HYPRE_ParCSRPCGGetFinalRelativeResidualNorm(HYPRE_Solver  solver,
                                                      HYPRE_Real   *norm);

/**
 * Setup routine for diagonal preconditioning.
 **/
HYPRE_Int HYPRE_ParCSRDiagScaleSetup(HYPRE_Solver       solver,
                                     HYPRE_ParCSRMatrix A,
                                     HYPRE_ParVector    y,
                                     HYPRE_ParVector    x);

/**
 * Solve routine for diagonal preconditioning.
 **/
HYPRE_Int HYPRE_ParCSRDiagScale(HYPRE_Solver       solver,
                                HYPRE_ParCSRMatrix HA,
                                HYPRE_ParVector    Hy,
                                HYPRE_ParVector    Hx);

/*@}*/

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/**
 * @name ParCSR GMRES Solver
 *
 * These routines should be used in conjunction with the generic interface in
 * \Ref{GMRES Solver}.
 **/
/*@{*/

/**
 * Create a solver object.
 **/
HYPRE_Int HYPRE_ParCSRGMRESCreate(MPI_Comm      comm,
                                  HYPRE_Solver *solver);

/**
 * Destroy a solver object.
 **/
HYPRE_Int HYPRE_ParCSRGMRESDestroy(HYPRE_Solver solver);

HYPRE_Int HYPRE_ParCSRGMRESSetup(HYPRE_Solver       solver,
                                 HYPRE_ParCSRMatrix A,
                                 HYPRE_ParVector    b,
                                 HYPRE_ParVector    x);

HYPRE_Int HYPRE_ParCSRGMRESSolve(HYPRE_Solver       solver,
                                 HYPRE_ParCSRMatrix A,
                                 HYPRE_ParVector    b,
                                 HYPRE_ParVector    x);

HYPRE_Int HYPRE_ParCSRGMRESSetKDim(HYPRE_Solver solver,
                                   HYPRE_Int    k_dim);

HYPRE_Int HYPRE_ParCSRGMRESSetTol(HYPRE_Solver solver,
                                  HYPRE_Real   tol);

HYPRE_Int HYPRE_ParCSRGMRESSetAbsoluteTol(HYPRE_Solver solver,
                                          HYPRE_Real   a_tol);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_ParCSRGMRESSetMinIter(HYPRE_Solver solver,
                                      HYPRE_Int    min_iter);

HYPRE_Int HYPRE_ParCSRGMRESSetMaxIter(HYPRE_Solver solver,
                                      HYPRE_Int    max_iter);

/*
 * Obsolete
 **/
HYPRE_Int HYPRE_ParCSRGMRESSetStopCrit(HYPRE_Solver solver,
                                       HYPRE_Int    stop_crit);

HYPRE_Int HYPRE_ParCSRGMRESSetPrecond(HYPRE_Solver             solver,
                                      HYPRE_PtrToParSolverFcn  precond,
                                      HYPRE_PtrToParSolverFcn  precond_setup,
                                      HYPRE_Solver             precond_solver);

HYPRE_Int HYPRE_ParCSRGMRESGetPrecond(HYPRE_Solver  solver,
                                      HYPRE_Solver *precond_data);

HYPRE_Int HYPRE_ParCSRGMRESSetLogging(HYPRE_Solver solver,
                                      HYPRE_Int    logging);

HYPRE_Int HYPRE_ParCSRGMRESSetPrintLevel(HYPRE_Solver solver,
                                         HYPRE_Int    print_level);

HYPRE_Int HYPRE_ParCSRGMRESGetNumIterations(HYPRE_Solver  solver,
                                            HYPRE_Int    *num_iterations);

HYPRE_Int HYPRE_ParCSRGMRESGetFinalRelativeResidualNorm(HYPRE_Solver  solver,
                                                        HYPRE_Real   *norm);

/*@}*/

/*--------------------------------------------------------------------------
 * Miscellaneous: These probably do not belong in the interface.
 *--------------------------------------------------------------------------*/

HYPRE_ParCSRMatrix GenerateLaplacian(MPI_Comm    comm,
                                     HYPRE_Int   nx,
                                     HYPRE_Int   ny,
                                     HYPRE_Int   nz,
                                     HYPRE_Int   P,
                                     HYPRE_Int   Q,
                                     HYPRE_Int   R,
                                     HYPRE_Int   p,
                                     HYPRE_Int   q,
                                     HYPRE_Int   r,
                                     HYPRE_Real *value);

HYPRE_ParCSRMatrix GenerateLaplacian27pt(MPI_Comm    comm,
                                         HYPRE_Int   nx,
                                         HYPRE_Int   ny,
                                         HYPRE_Int   nz,
                                         HYPRE_Int   P,
                                         HYPRE_Int   Q,
                                         HYPRE_Int   R,
                                         HYPRE_Int   p,
                                         HYPRE_Int   q,
                                         HYPRE_Int   r,
                                         HYPRE_Real *value);

HYPRE_ParCSRMatrix GenerateLaplacian9pt(MPI_Comm    comm,
                                        HYPRE_Int   nx,
                                        HYPRE_Int   ny,
                                        HYPRE_Int   P,
                                        HYPRE_Int   Q,
                                        HYPRE_Int   p,
                                        HYPRE_Int   q,
                                        HYPRE_Real *value);

HYPRE_ParCSRMatrix GenerateDifConv(MPI_Comm    comm,
                                   HYPRE_Int   nx,
                                   HYPRE_Int   ny,
                                   HYPRE_Int   nz,
                                   HYPRE_Int   P,
                                   HYPRE_Int   Q,
                                   HYPRE_Int   R,
                                   HYPRE_Int   p,
                                   HYPRE_Int   q,
                                   HYPRE_Int   r,
                                   HYPRE_Real *value);

HYPRE_ParCSRMatrix
GenerateRotate7pt(MPI_Comm   comm,
                  HYPRE_Int  nx,
                  HYPRE_Int  ny,
                  HYPRE_Int  P,
                  HYPRE_Int  Q,
                  HYPRE_Int  p,
                  HYPRE_Int  q,
                  HYPRE_Real alpha,
                  HYPRE_Real eps );
                                                                                
HYPRE_ParCSRMatrix
GenerateVarDifConv(MPI_Comm         comm,
                   HYPRE_Int        nx,
                   HYPRE_Int        ny,
                   HYPRE_Int        nz,
                   HYPRE_Int        P,
                   HYPRE_Int        Q,
                   HYPRE_Int        R,
                   HYPRE_Int        p,
                   HYPRE_Int        q,
                   HYPRE_Int        r,
                   HYPRE_Real       eps,
                   HYPRE_ParVector *rhs_ptr);

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/*
 * (Optional) Switches on use of Jacobi interpolation after computing
 * an original interpolation
 **/
HYPRE_Int HYPRE_BoomerAMGSetPostInterpType(HYPRE_Solver solver,
                                           HYPRE_Int    post_interp_type);

/*
 * (Optional) Sets a truncation threshold for Jacobi interpolation.
 **/
HYPRE_Int HYPRE_BoomerAMGSetJacobiTruncThreshold(HYPRE_Solver solver,
                                                 HYPRE_Real   jacobi_trunc_threshold);



/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/ 
/*@}*/

#ifdef __cplusplus
}
#endif

#endif
