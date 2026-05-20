# Change this to the location of your PETSc source.
PETSC_DIR=/usr/workspace/wsa/laguna/fpchecker/FPChecker/temp/petsc-3.25.0
PETSC_ARCH=arch-single-noftn-fpc-error

FPC_INSTALL=/usr/workspace/wsa/laguna/fpchecker/FPChecker/build/install

.DEFAULT_GOAL := all

FPC_FLAGS := -g -fno-vectorize -fno-slp-vectorize -include ${FPC_INSTALL}/src/Runtime_error.h -fpass-plugin=${FPC_INSTALL}/lib/libfpchecker_error.so

include ${PETSC_DIR}/lib/petsc/conf/variables
include ${PETSC_DIR}/lib/petsc/conf/rules
CFLAGS += -pedantic -std=c99 -I${PETSC_DIR}/include -I${PETSC_DIR}/${PETSC_ARCH}/include

all: petsc_example

petsc_example.o: petsc_example.c
	-${CC} -c petsc_example.c ${CFLAGS} ${FPC_FLAGS} ${PETSC_INCLUDE} -o petsc_example.o

petsc_example: petsc_example.o
	-${CLINKER} -o petsc_example petsc_example.o  ${PETSC_LIB}

.PHONY: clean distclean

distclean: clean

clean::
	@rm -rf *~ *tmp petsc_example *.o .fpc_logs
	@rm -rf *.dat *.dat.info

