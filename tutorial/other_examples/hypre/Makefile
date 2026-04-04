
MPI_INCLUDE=/opt/anaconda3/envs/tutorial_env/include
HYPRE=/Users/lagunaperalt1/projects/fpchecker/FPChecker/temp/hypre-3.1.0/src/hypre
HYPRE_INCLUDE=$(HYPRE)/include

HYPRE_LIB=$(HYPRE)/lib
CFLAGS = -I$(MPI_INCLUDE) -I$(HYPRE_INCLUDE) -DWITH_MPI
LDFLAGS = -L$(HYPRE_LIB) -lHYPRE

MPICC=mpicc-fpchecker

# all:
# # 	mpicc -c hypre_test.c -I$(HYPRE)
# # 	mpicc hypre_test.o -o hypre_test -lHYPRE -L$(HYPRE)/../lib
all: hypre_test 

hypre_test: hypre_test.o
	$(MPICC) -o hypre_test hypre_test.o $(LDFLAGS)

hypre_test.o: hypre_test.c
	$(MPICC) -c hypre_test.c $(CFLAGS)

clean:
	rm -rf *.o hypre_test verify_warnings_hypre .fpc_logs .fpc_log.txt
