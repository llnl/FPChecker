HYPRE_SRC_DIR ?= $(abspath hypre-3.1.0/src)
HYPRE_BUILD_DIR ?= $(HYPRE_SRC_DIR)/build
HYPRE_LIB ?= $(HYPRE_BUILD_DIR)/lib/libHYPRE.a

MPICC ?= mpicc
OMPI_CC ?= clang
TARGET ?= hypre_test
SRC ?= hypre_test.c

MATRIX ?= matrix.csv
SOLVER_ID ?= 0

HYPRE_DIR_INCLUDES ?= $(shell find "$(HYPRE_SRC_DIR)" -mindepth 1 -maxdepth 1 -type d -print | sed 's|^|-I|')
HYPRE_INCLUDES = -I$(HYPRE_SRC_DIR) -I$(HYPRE_BUILD_DIR) $(HYPRE_DIR_INCLUDES)

COMMON_FLAGS = -O0 -g -DWITH_MPI

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC) $(HYPRE_LIB) Makefile
	OMPI_CC=$(OMPI_CC) $(MPICC) $(COMMON_FLAGS) $(HYPRE_INCLUDES) $(SRC) -o $(TARGET) $(HYPRE_LIB) -lm

clean:
	rm -f $(TARGET)
	rm -rf .fpc_logs fpc-report *.dSYM
