#ifdef WITH_MPI
#include <mpi.h>
#endif

#include <math.h>
#include "_hypre_utilities.h"
#include "HYPRE_krylov.h"
#include "HYPRE.h"
#include "HYPRE_parcsr_ls.h"
#include "HYPRE_sstruct_ls.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define MAX_LEN 5000

static void print_usage(const char *prog_name)
{
    printf("Usage: %s <matrix.csv> <solver>\n", prog_name);
    printf("\n");
    printf("Valid solver options:\n");
    printf("  amg      : AMG\n");
    printf("  pcg      : PCG\n");
    printf("  amg_pcg  : PCG with AMG preconditioner\n");
}

int loadMatrix(HYPRE_IJMatrix *A, HYPRE_IJVector *b, HYPRE_IJVector *x, const char *s) {

    // Handle error
    if (s == NULL || s[0] == '\0') {
        printf("Matrix path not provided\n");
        exit(-1);
    }

    // ---- Get sizeof matrix --------------
    printf("Matrix %s\n", s);
    FILE *file;
    file = fopen(s, "r");
    if (file == NULL) {
        printf("Error opening matrix file\n");
        exit(-1);
    }
    char data[MAX_LEN];
    data[0]='\0';
    HYPRE_Complex (*arr)[MAX_LEN] = (HYPRE_Complex (*)[MAX_LEN])malloc(sizeof(HYPRE_Complex[MAX_LEN][MAX_LEN]));
    int line=0;
    int cols = 0;
    while (!feof(file)&& !ferror(file)) {
        if (fgets(data, MAX_LEN, file) != NULL) {
            // Get columns
            char *pt;
            pt = strtok (data,",");
            cols = 0;
            while (pt != NULL) {
                HYPRE_Complex f = (HYPRE_Complex)atof(pt);
                //printf("%f ", f);
                arr[line][cols] = f;
                pt = strtok (NULL, ",");
                cols++;
            }
            data[0]='\0';
            line++;
						//printf("\n");
        }
    }
    fclose(file);
    printf("Rows %d, cols %d\n", line, cols);

    // Iterate on data
    HYPRE_Real sum = 0.0;
    for (int i=0; i < line; ++i) {
        for (int j=0; j < cols; ++j) {
            printf("%f | ", arr[i][j]);
            sum += arr[i][j];
        }
        printf("\n");
    }

    if (line != cols) {
        printf("Not square matrix\n");
        exit(0);
    }

    // Check all zero matrices
    if (sum == 0.0) {
        printf("All-zero matrix\n");
        exit(0);
    }

    // ================ Create Matrix A =========================

    /* Create the matrix.
       Note that this is a square matrix, so we indicate the row partition
       size twice (since number of rows = number of cols) */
    HYPRE_IJMatrixCreate(hypre_MPI_COMM_WORLD, 0, cols - 1, 0, cols - 1, A);

    /* Choose a parallel csr format storage (see the User's Manual) */
    HYPRE_IJMatrixSetObjectType(*A, HYPRE_PARCSR);

    /* Initialize before setting coefficients */
    HYPRE_IJMatrixInitialize(*A);

    // Create indices for columns
    int *j = (int *)malloc(sizeof(int)*cols);
    for (int i=0; i < cols; ++i)
        j[i] = i;

    for (int i=0; i<line; i++) {   // set entries one row at a time
        //PetscCall(MatSetValues(*A,1,&i,cols,j,arr[i],INSERT_VALUES));
        HYPRE_IJMatrixSetValues(*A, 1, &cols, &i, j, arr[i]);
    }

    // ================ Create x and b ===========================
    /* Create the rhs and solution */
    HYPRE_IJVectorCreate(hypre_MPI_COMM_WORLD, 0, cols - 1, b);
    HYPRE_IJVectorSetObjectType(*b, HYPRE_PARCSR);
    HYPRE_IJVectorInitialize(*b);

    HYPRE_IJVectorCreate(hypre_MPI_COMM_WORLD, 0, cols - 1, x);
    HYPRE_IJVectorSetObjectType(*x, HYPRE_PARCSR);
    HYPRE_IJVectorInitialize(*x);

    /* --- set values for x and b ---- */
    HYPRE_Real *b_vals = (HYPRE_Real *)malloc(sizeof(HYPRE_Real)*cols);
    //printf("b: ");
    for (int i=0; i < cols; ++i) {
        b_vals[i] = (HYPRE_Real)(i+1)/100.0;
        //b_vals[i] = ((HYPRE_Real)i)/cols;
        //printf("%f ", b_vals[i]);
    }
    //printf("\n");

    HYPRE_IJVectorSetValues(*b, cols, j, b_vals);

    HYPRE_Real *x_vals = (HYPRE_Real *)malloc(sizeof(HYPRE_Real)*cols);
    for (int i=0; i < cols; ++i)
        x_vals[i] = 0;
    HYPRE_IJVectorSetValues(*x, cols, j, x_vals);

    free(arr);
    free(j);
    free(x_vals);
    //free(b_vals);

    return 1;
}

HYPRE_Int main (HYPRE_Int argc, char *argv[])
{
    if (argc != 3)
    {
        print_usage(argv[0]);
        return -1;
    }

    const char *matrix_path = argv[1];
    const char *solver_name = argv[2];
    HYPRE_Int solver_id = -1;

    if (strcmp(solver_name, "amg") == 0) {
        solver_id = 0;
    }
    else if (strcmp(solver_name, "pcg") == 0) {
        solver_id = 1;
    }
    else if (strcmp(solver_name, "amg_pcg") == 0) {
        solver_id = 2;
    }
    else {
        printf("Invalid solver option: %s\n", solver_name);
        print_usage(argv[0]);
        return -1;
    }

    HYPRE_Int i;
    HYPRE_Int myid, num_procs;
    HYPRE_Int N, n;

    HYPRE_Int ilower, iupper;
    HYPRE_Int local_size, extra;
    HYPRE_Int print_solution;

    HYPRE_Real h, h2;

    HYPRE_IJMatrix A;
    HYPRE_ParCSRMatrix parcsr_A;
    HYPRE_IJVector b;
    HYPRE_ParVector par_b;
    HYPRE_IJVector x;
    HYPRE_ParVector par_x;

    HYPRE_Solver solver, precond;

    HYPRE_Int time_index;

    /* Initialize MPI */
    hypre_MPI_Init(&argc, &argv);
    hypre_MPI_Comm_rank(hypre_MPI_COMM_WORLD, &myid);
    hypre_MPI_Comm_size(hypre_MPI_COMM_WORLD, &num_procs);

    HYPRE_Initialize();
    printf("In main... %d\n", myid);

    loadMatrix(&A, &b, &x, matrix_path);

    /* Assemble after setting the coefficients */
    HYPRE_IJMatrixAssemble(A);

    /* Get the parcsr matrix object to use */
    HYPRE_IJMatrixGetObject(A, (void**) &parcsr_A);

    HYPRE_IJVectorAssemble(b);
    HYPRE_IJVectorGetObject(b, (void **) &par_b);

    HYPRE_IJVectorAssemble(x);
    HYPRE_IJVectorGetObject(x, (void **) &par_x);

    /* Choose a solver and solve the system */

    // ------------------------- AMG --------------------------------
    if (solver_id == 0)
    {
       HYPRE_Int num_iterations;
       HYPRE_Real final_res_norm;

       // Create solver
       HYPRE_BoomerAMGCreate(&solver);

       // Set some parameters (See Reference Manual for more parameters)
       HYPRE_BoomerAMGSetPrintLevel(solver, 2);  // print solve info + parameters
       HYPRE_BoomerAMGSetCoarsenType(solver, 6); // Falgout coarsening
       HYPRE_BoomerAMGSetRelaxType(solver, 3);   // G-S/Jacobi hybrid relaxation
       HYPRE_BoomerAMGSetNumSweeps(solver, 1);   // Sweeeps on each level
       HYPRE_BoomerAMGSetMaxLevels(solver, 20);  // maximum number of levels
       HYPRE_BoomerAMGSetTol(solver, 1e-7);      // conv. tolerance

       // Now setup and solve!
       HYPRE_BoomerAMGSetup(solver, parcsr_A, par_b, par_x);
       HYPRE_BoomerAMGSolve(solver, parcsr_A, par_b, par_x);

       // Run info - needed logging turned on
       HYPRE_BoomerAMGGetNumIterations(solver, &num_iterations);
       HYPRE_BoomerAMGGetFinalRelativeResidualNorm(solver, &final_res_norm);

       if (myid == 0)
       {
          hypre_printf("\n");
          hypre_printf("Iterations = %d\n", num_iterations);
          hypre_printf("Final Relative Residual Norm = %e\n", final_res_norm);
          hypre_printf("\n");
       }

       // Destroy solver
       HYPRE_BoomerAMGDestroy(solver);
    }
    /* PCG */
    else if (solver_id == 1)
    {
       printf("\n SOLVER : PCG \n ");
       HYPRE_Int num_iterations;
       HYPRE_Real final_res_norm;

       /* Create solver */
       HYPRE_ParCSRPCGCreate(hypre_MPI_COMM_WORLD, &solver);

       /* Set some parameters (See Reference Manual for more parameters) */
       HYPRE_PCGSetMaxIter(solver, 1000); /* max iterations */
       HYPRE_PCGSetTol(solver, 1e-12); /* conv. tolerance */
       //HYPRE_PCGSetResidualTol(solver, 1e-6); /* force zero residual for convergence */
       HYPRE_PCGSetTwoNorm(solver, 1); /* use the two norm as the stopping criteria */
       HYPRE_PCGSetPrintLevel(solver, 2); /* prints out the iteration info */
       HYPRE_PCGSetLogging(solver, 1); /* needed to get run info later */

       /* Now setup and solve! */
       HYPRE_ParCSRPCGSetup(solver, parcsr_A, par_b, par_x);
       HYPRE_ParCSRPCGSolve(solver, parcsr_A, par_b, par_x);

       /* Run info - needed logging turned on */
       HYPRE_PCGGetNumIterations(solver, &num_iterations);
       HYPRE_PCGGetFinalRelativeResidualNorm(solver, &final_res_norm);
       if (myid == 0)
       {
          hypre_printf("\n");
          hypre_printf("Iterations = %d\n", num_iterations);
          hypre_printf("Final Relative Residual Norm = %e\n", final_res_norm);
          hypre_printf("\n");
       }

       /* Destroy solver */
       HYPRE_ParCSRPCGDestroy(solver);
    }
    /* PCG with AMG preconditioner */
    else if (solver_id == 2)
    {
       printf("\n SOLVER : PCG with AMG preconditioner \n ");
       HYPRE_Int num_iterations;
       HYPRE_Real final_res_norm;

       /* Create solver */
       HYPRE_ParCSRPCGCreate(hypre_MPI_COMM_WORLD, &solver);

       /* Set some parameters (See Reference Manual for more parameters) */
       HYPRE_PCGSetMaxIter(solver, 1000); /* max iterations */
       HYPRE_PCGSetTol(solver, 1e-7); /* conv. tolerance */
       HYPRE_PCGSetTwoNorm(solver, 1); /* use the two norm as the stopping criteria */
       HYPRE_PCGSetPrintLevel(solver, 2); /* print solve info */
       HYPRE_PCGSetLogging(solver, 1); /* needed to get run info later */

       /* Now set up the AMG preconditioner and specify any parameters */
       HYPRE_BoomerAMGCreate(&precond);
       HYPRE_BoomerAMGSetPrintLevel(precond, 1); /* print amg solution info*/
       HYPRE_BoomerAMGSetCoarsenType(precond, 6);
       HYPRE_BoomerAMGSetRelaxType(precond, 3);
       HYPRE_BoomerAMGSetNumSweeps(precond, 1);
       HYPRE_BoomerAMGSetTol(precond, 1e-3);

       /* Set the PCG preconditioner */
       HYPRE_PCGSetPrecond(solver, (HYPRE_PtrToSolverFcn) HYPRE_BoomerAMGSolve,
                           (HYPRE_PtrToSolverFcn) HYPRE_BoomerAMGSetup, precond);

       /* Now setup and solve! */
       HYPRE_ParCSRPCGSetup(solver, parcsr_A, par_b, par_x);
       HYPRE_ParCSRPCGSolve(solver, parcsr_A, par_b, par_x);

       /* Run info - needed logging turned on */
       HYPRE_PCGGetNumIterations(solver, &num_iterations);
       HYPRE_PCGGetFinalRelativeResidualNorm(solver, &final_res_norm);

       if (myid == 0)
       {
          hypre_printf("\n");
          hypre_printf("Iterations = %d\n", num_iterations);
          hypre_printf("Final Relative Residual Norm = %e\n", final_res_norm);
          hypre_printf("\n");
       }

       /* Destroy solver and preconditioner */
       HYPRE_ParCSRPCGDestroy(solver);
       HYPRE_BoomerAMGDestroy(precond);
    }
    if (myid == 0) {
        // Print the solution
        int pid = getpid();
        char name[2048];
        name[0] = '\0';
        //sprintf(name, "solution_x_%d", pid);
        HYPRE_IJVectorPrint(x, name);
    }

    /* Clean up */
    HYPRE_IJMatrixDestroy(A);
    HYPRE_IJVectorDestroy(b);
    HYPRE_IJVectorDestroy(x);

    /* Finalize MPI*/
    hypre_MPI_Finalize();

    return (0);
 }
