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

#ifndef HYPRE_KRYLOV_HEADER
#define HYPRE_KRYLOV_HEADER

#include "HYPRE_utilities.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/**
 * @name Krylov Solvers
 *
 * These solvers support many of the matrix/vector storage schemes in hypre.
 * They should be used in conjunction with the storage-specific interfaces,
 * particularly the specific Create() and Destroy() functions.
 *
 * @memo A basic interface for Krylov solvers
 **/
/*@{*/

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/**
 * @name Krylov Solvers
 **/
/*@{*/

#ifndef HYPRE_SOLVER_STRUCT
#define HYPRE_SOLVER_STRUCT
struct hypre_Solver_struct;
/**
 * The solver object.
 **/
typedef struct hypre_Solver_struct *HYPRE_Solver;
#endif

#ifndef HYPRE_MATRIX_STRUCT
#define HYPRE_MATRIX_STRUCT
struct hypre_Matrix_struct;
/**
 * The matrix object.
 **/
typedef struct hypre_Matrix_struct *HYPRE_Matrix;
#endif

#ifndef HYPRE_VECTOR_STRUCT
#define HYPRE_VECTOR_STRUCT
struct hypre_Vector_struct;
/**
 * The vector object.
 **/
typedef struct hypre_Vector_struct *HYPRE_Vector;
#endif

typedef HYPRE_Int (*HYPRE_PtrToSolverFcn)(HYPRE_Solver,
                                    HYPRE_Matrix,
                                    HYPRE_Vector,
                                    HYPRE_Vector);

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
 * @name PCG Solver
 **/
/*@{*/

/**
 * Prepare to solve the system.  The coefficient data in {\tt b} and {\tt x} is
 * ignored here, but information about the layout of the data may be used.
 **/
HYPRE_Int HYPRE_PCGSetup(HYPRE_Solver solver,
                   HYPRE_Matrix A,
                   HYPRE_Vector b,
                   HYPRE_Vector x);

/**
 * Solve the system.
 **/
HYPRE_Int HYPRE_PCGSolve(HYPRE_Solver solver,
                   HYPRE_Matrix A,
                   HYPRE_Vector b,
                   HYPRE_Vector x);

/**
 * (Optional) Set the relative convergence tolerance.
 **/
HYPRE_Int HYPRE_PCGSetTol(HYPRE_Solver solver,
                    HYPRE_Real   tol);

/**
 * (Optional) Set the absolute convergence tolerance (default is
 * 0). If one desires the convergence test to check the absolute
 * convergence tolerance {\it only}, then set the relative convergence
 * tolerance to 0.0.  (The default convergence test is $ <C*r,r> \leq$
 * max(relative$\_$tolerance$^{2} \ast <C*b, b>$, absolute$\_$tolerance$^2$).)
 **/
HYPRE_Int HYPRE_PCGSetAbsoluteTol(HYPRE_Solver solver,
                            HYPRE_Real   a_tol);

/**
 * (Optional) Set a residual-based convergence tolerance which checks if
 * $\|r_{old}-r_{new}\| < rtol \|b\|$. This is useful when trying to converge to
 * very low relative and/or absolute tolerances, in order to bail-out before
 * roundoff errors affect the approximation.
 **/
HYPRE_Int HYPRE_PCGSetResidualTol(HYPRE_Solver solver,
                            HYPRE_Real   rtol);
/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_PCGSetAbsoluteTolFactor(HYPRE_Solver solver, HYPRE_Real abstolf);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_PCGSetConvergenceFactorTol(HYPRE_Solver solver, HYPRE_Real cf_tol);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_PCGSetStopCrit(HYPRE_Solver solver, HYPRE_Int stop_crit);

/**
 * (Optional) Set maximum number of iterations.
 **/
HYPRE_Int HYPRE_PCGSetMaxIter(HYPRE_Solver solver,
                        HYPRE_Int          max_iter);

/**
 * (Optional) Use the two-norm in stopping criteria.
 **/
HYPRE_Int HYPRE_PCGSetTwoNorm(HYPRE_Solver solver,
                        HYPRE_Int          two_norm);

/**
 * (Optional) Additionally require that the relative difference in
 * successive iterates be small.
 **/
HYPRE_Int HYPRE_PCGSetRelChange(HYPRE_Solver solver,
                          HYPRE_Int          rel_change);

/**
 * (Optional) Recompute the residual at the end to double-check convergence.
 **/
HYPRE_Int HYPRE_PCGSetRecomputeResidual(HYPRE_Solver solver,
                                  HYPRE_Int          recompute_residual);

/**
 * (Optional) Periodically recompute the residual while iterating.
 **/
HYPRE_Int HYPRE_PCGSetRecomputeResidualP(HYPRE_Solver solver,
                                   HYPRE_Int          recompute_residual_p);

/**
 * (Optional) Set the preconditioner to use.
 **/
HYPRE_Int HYPRE_PCGSetPrecond(HYPRE_Solver         solver,
                        HYPRE_PtrToSolverFcn precond,
                        HYPRE_PtrToSolverFcn precond_setup,
                        HYPRE_Solver         precond_solver);

/**
 * (Optional) Set the amount of logging to do.
 **/
HYPRE_Int HYPRE_PCGSetLogging(HYPRE_Solver solver,
                        HYPRE_Int          logging);

/**
 * (Optional) Set the amount of printing to do to the screen.
 **/
HYPRE_Int HYPRE_PCGSetPrintLevel(HYPRE_Solver solver,
                           HYPRE_Int          level);

/**
 * Return the number of iterations taken.
 **/
HYPRE_Int HYPRE_PCGGetNumIterations(HYPRE_Solver  solver,
                              HYPRE_Int          *num_iterations);

/**
 * Return the norm of the final relative residual.
 **/
HYPRE_Int HYPRE_PCGGetFinalRelativeResidualNorm(HYPRE_Solver  solver,
                                          HYPRE_Real   *norm);

/**
 * Return the residual.
 **/
HYPRE_Int HYPRE_PCGGetResidual(HYPRE_Solver  solver,
                         void        **residual);

/**
 **/
HYPRE_Int HYPRE_PCGGetTol(HYPRE_Solver  solver,
                    HYPRE_Real   *tol);

/**
 **/
HYPRE_Int HYPRE_PCGGetResidualTol(HYPRE_Solver  solver,
                            HYPRE_Real   *rtol);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_PCGGetAbsoluteTolFactor(HYPRE_Solver solver, HYPRE_Real *abstolf);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_PCGGetConvergenceFactorTol(HYPRE_Solver solver, HYPRE_Real *cf_tol);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_PCGGetStopCrit(HYPRE_Solver solver, HYPRE_Int *stop_crit);

/**
 **/
HYPRE_Int HYPRE_PCGGetMaxIter(HYPRE_Solver  solver,
                        HYPRE_Int          *max_iter);

/**
 **/
HYPRE_Int HYPRE_PCGGetTwoNorm(HYPRE_Solver  solver,
                        HYPRE_Int          *two_norm);

/**
 **/
HYPRE_Int HYPRE_PCGGetRelChange(HYPRE_Solver  solver,
                          HYPRE_Int          *rel_change);

/**
 **/
HYPRE_Int HYPRE_GMRESGetSkipRealResidualCheck(HYPRE_Solver solver,
                                              HYPRE_Int *skip_real_r_check);

/**
 **/
HYPRE_Int HYPRE_PCGGetPrecond(HYPRE_Solver  solver,
                        HYPRE_Solver *precond_data_ptr);

/**
 **/
HYPRE_Int HYPRE_PCGGetLogging(HYPRE_Solver  solver,
                        HYPRE_Int          *level);

/**
 **/
HYPRE_Int HYPRE_PCGGetPrintLevel(HYPRE_Solver  solver,
                           HYPRE_Int          *level);

/**
 **/
HYPRE_Int HYPRE_PCGGetConverged(HYPRE_Solver  solver,
                          HYPRE_Int          *converged);

/*@}*/

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/**
 * @name GMRES Solver
 **/
/*@{*/

/**
 * Prepare to solve the system.  The coefficient data in {\tt b} and {\tt x} is
 * ignored here, but information about the layout of the data may be used.
 **/
HYPRE_Int HYPRE_GMRESSetup(HYPRE_Solver solver,
                     HYPRE_Matrix A,
                     HYPRE_Vector b,
                     HYPRE_Vector x);

/**
 * Solve the system.
 **/
HYPRE_Int HYPRE_GMRESSolve(HYPRE_Solver solver,
                     HYPRE_Matrix A,
                     HYPRE_Vector b,
                     HYPRE_Vector x);

/**
 * (Optional) Set the relative convergence tolerance.
 **/
HYPRE_Int HYPRE_GMRESSetTol(HYPRE_Solver solver,
                      HYPRE_Real   tol);

/**
 * (Optional) Set the absolute convergence tolerance (default is 0). 
 * If one desires
 * the convergence test to check the absolute convergence tolerance {\it only}, then
 * set the relative convergence tolerance to 0.0.  (The convergence test is 
 * $\|r\| \leq$ max(relative$\_$tolerance$\ast \|b\|$, absolute$\_$tolerance).)
 *
 **/
HYPRE_Int HYPRE_GMRESSetAbsoluteTol(HYPRE_Solver solver,
                              HYPRE_Real   a_tol);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_GMRESSetConvergenceFactorTol(HYPRE_Solver solver, HYPRE_Real cf_tol);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_GMRESSetStopCrit(HYPRE_Solver solver, HYPRE_Int stop_crit);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_GMRESSetMinIter(HYPRE_Solver solver, HYPRE_Int min_iter);

/**
 * (Optional) Set maximum number of iterations.
 **/
HYPRE_Int HYPRE_GMRESSetMaxIter(HYPRE_Solver solver,
                          HYPRE_Int          max_iter);

/**
 * (Optional) Set the maximum size of the Krylov space.
 **/
HYPRE_Int HYPRE_GMRESSetKDim(HYPRE_Solver solver,
                       HYPRE_Int          k_dim);

/**
 * (Optional) Additionally require that the relative difference in
 * successive iterates be small.
 **/
HYPRE_Int HYPRE_GMRESSetRelChange(HYPRE_Solver solver,
                            HYPRE_Int          rel_change);

/**
 * (Optional) By default, hypre checks for convergence by evaluating the actual
 * residual before returnig from GMRES (with restart if the true residual does
 * not indicate convergence). This option allows users to skip the evaluation
 * and the check of the actual residual for badly conditioned problems where
 * restart is not expected to be beneficial.
 **/
HYPRE_Int HYPRE_GMRESSetSkipRealResidualCheck(HYPRE_Solver solver,
                                              HYPRE_Int skip_real_r_check);

/**
 * (Optional) Set the preconditioner to use.
 **/
HYPRE_Int HYPRE_GMRESSetPrecond(HYPRE_Solver         solver,
                          HYPRE_PtrToSolverFcn precond,
                          HYPRE_PtrToSolverFcn precond_setup,
                          HYPRE_Solver         precond_solver);

/**
 * (Optional) Set the amount of logging to do.
 **/
HYPRE_Int HYPRE_GMRESSetLogging(HYPRE_Solver solver,
                          HYPRE_Int          logging);

/**
 * (Optional) Set the amount of printing to do to the screen.
 **/
HYPRE_Int HYPRE_GMRESSetPrintLevel(HYPRE_Solver solver,
                             HYPRE_Int          level);

/**
 * Return the number of iterations taken.
 **/
HYPRE_Int HYPRE_GMRESGetNumIterations(HYPRE_Solver  solver,
                                HYPRE_Int          *num_iterations);

/**
 * Return the norm of the final relative residual.
 **/
HYPRE_Int HYPRE_GMRESGetFinalRelativeResidualNorm(HYPRE_Solver  solver,
                                            HYPRE_Real   *norm);

/**
 * Return the residual.
 **/
HYPRE_Int HYPRE_GMRESGetResidual(HYPRE_Solver   solver,
                           void         **residual);

/**
 **/
HYPRE_Int HYPRE_GMRESGetTol(HYPRE_Solver  solver,
                      HYPRE_Real   *tol);

/**
 **/
HYPRE_Int HYPRE_GMRESGetAbsoluteTol(HYPRE_Solver  solver,
                              HYPRE_Real   *tol);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_GMRESGetConvergenceFactorTol(HYPRE_Solver solver, HYPRE_Real *cf_tol);

/*
 * OBSOLETE 
 **/
HYPRE_Int HYPRE_GMRESGetStopCrit(HYPRE_Solver solver, HYPRE_Int *stop_crit);

/*
 * RE-VISIT
 **/
HYPRE_Int HYPRE_GMRESGetMinIter(HYPRE_Solver solver, HYPRE_Int *min_iter);

/**
 **/
HYPRE_Int HYPRE_GMRESGetMaxIter(HYPRE_Solver  solver,
                          HYPRE_Int          *max_iter);

/**
 **/
HYPRE_Int HYPRE_GMRESGetKDim(HYPRE_Solver  solver,
                       HYPRE_Int          *k_dim);

/**
 **/
HYPRE_Int HYPRE_GMRESGetRelChange(HYPRE_Solver  solver,
                            HYPRE_Int          *rel_change);

/**
 **/
HYPRE_Int HYPRE_GMRESGetPrecond(HYPRE_Solver  solver,
                          HYPRE_Solver *precond_data_ptr);

/**
 **/
HYPRE_Int HYPRE_GMRESGetLogging(HYPRE_Solver  solver,
                          HYPRE_Int          *level);

/**
 **/
HYPRE_Int HYPRE_GMRESGetPrintLevel(HYPRE_Solver  solver,
                             HYPRE_Int          *level);

/**
 **/
HYPRE_Int HYPRE_GMRESGetConverged(HYPRE_Solver  solver,
                            HYPRE_Int          *converged);

/*@}*/

/*--------------------------------------------------------------------------
 *--------------------------------------------------------------------------*/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif
