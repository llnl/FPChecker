//STARTWHOLE
static char help[] = "Solve a 4x4 linear system using KSP.\n";

#include <petsc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 5000

int loadMatrix(Mat *A, Vec *b) {
    const char* s = getenv("PETSC_MATRIX");
    
    // Handle error
    if (s==NULL) {
        printf("PETSC_MATRIX var not found");
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
    PetscScalar (*arr)[MAX_LEN] = malloc(sizeof(PetscScalar[MAX_LEN][MAX_LEN]));
    int line=0;
    int cols = 0;
    while (!feof(file)&& !ferror(file)) {
        if (fgets(data, MAX_LEN, file) != NULL) {
            // Get columns
            char *pt;
            pt = strtok (data,",");
            cols = 0;
            while (pt != NULL) {
                PetscScalar f = (PetscScalar)strtof(pt, NULL);
                //printf(">%f ", f);
                arr[line][cols] = f;
                pt = strtok (NULL, ",");
                cols++;
            }
            data[0]='\0';
            line++;
        }
    }
    fclose(file);
    printf("Rows %d, cols %d\n", line, cols);
    
    // Iterate on data
    for (int i=0; i < line; ++i) {
        for (int j=0; j < cols; ++j) {
            printf("%f | ", arr[i][j]);
        }
        printf("\n");
    }
    
    if (line != cols) {
        printf("Not square matrix\n");
        exit(0);
    }
     
    
    // =============== Create Mat A Object =======================
    
    // Create indices for columns
    PetscInt *j = malloc(sizeof(PetscInt)*cols);
    for (int i=0; i < cols; ++i)
        j[i] = i;
    
    PetscCall(MatCreate(PETSC_COMM_WORLD,A));
    PetscCall(MatSetSizes(*A,PETSC_DECIDE,PETSC_DECIDE,(PetscInt)line,(PetscInt)cols));
    PetscCall(MatSetFromOptions(*A));
    PetscCall(MatSetUp(*A));
    for (int i=0; i<line; i++) {   // set entries one row at a time
        PetscInt row = (PetscInt)i;
        PetscCall(MatSetValues(*A,1,&row,(PetscInt)cols,j,arr[i],INSERT_VALUES));
    }
    PetscCall(MatAssemblyBegin(*A,MAT_FINAL_ASSEMBLY));
    PetscCall(MatAssemblyEnd(*A,MAT_FINAL_ASSEMBLY));
    free(arr);
    
    // =============== Cretor Vector b =====================
    PetscScalar *bs = malloc(sizeof(PetscScalar)*cols);
    for (int i=0; i < cols; ++i)
        bs[i] = (PetscScalar)1.0f;
    
    PetscCall(VecCreate(PETSC_COMM_WORLD,b));
    PetscCall(VecSetSizes(*b,PETSC_DECIDE,(PetscInt)cols));
    PetscCall(VecSetFromOptions(*b));
    PetscCall(VecSetValues(*b,(PetscInt)cols,j,bs,INSERT_VALUES));
    PetscCall(VecAssemblyBegin(*b));
    PetscCall(VecAssemblyEnd(*b));

    free(j);
    free(bs);
    
    return 1;
}

int main(int argc,char **args) {
    Vec        x, b;
    Mat        A;
    KSP        ksp;

    PetscCall(PetscInitialize(&argc,&args,NULL,help));
    loadMatrix(&A, &b);

    PetscCall(KSPCreate(PETSC_COMM_WORLD,&ksp));
    PetscCall(KSPSetOperators(ksp,A,A));
    PetscCall(KSPSetFromOptions(ksp));
    //KSPSetType(ksp,KSPCG);
    PetscCall(VecDuplicate(b,&x));
    PetscCall(KSPSolve(ksp,b,x));
    PetscCall(VecView(x,PETSC_VIEWER_STDOUT_WORLD));

    PetscCall(KSPDestroy(&ksp));
    PetscCall(MatDestroy(&A));
    PetscCall(VecDestroy(&x));
    PetscCall(VecDestroy(&b));
    PetscCall(PetscFinalize());
    return 0;
}
//ENDWHOLE
