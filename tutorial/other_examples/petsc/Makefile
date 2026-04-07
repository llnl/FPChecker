# Change this to the location of your PETSc source.
PETSC_DIR=/Users/lagunaperalt1/projects/fpchecker/FPChecker/temp/petsc-3.25.0

.DEFAULT_GOAL := all

FPC_FLAGS := -g -fno-vectorize -fno-slp-vectorize -include /Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/src/Runtime_error.h -fpass-plugin=/Users/lagunaperalt1/projects/fpchecker/FPChecker/build/install/lib/libfpchecker_error.dylib

include ${PETSC_DIR}/lib/petsc/conf/variables
include ${PETSC_DIR}/lib/petsc/conf/rules
CFLAGS += -pedantic -std=c99 -I${PETSC_DIR}/include -I${PETSC_DIR}/arch-single-noftn-fpc-error/include

all: petsc_example

petsc_example.o: petsc_example.c
	-${CC} -c petsc_example.c ${CFLAGS} ${FPC_FLAGS} ${PETSC_INCLUDE} -o petsc_example.o

petsc_example: petsc_example.o
	-${CLINKER} -o petsc_example petsc_example.o  ${PETSC_LIB}

.PHONY: clean distclean

distclean: clean

clean::
	@rm -f *~ *tmp petsc_example *.o
	@rm -f *.dat *.dat.info

