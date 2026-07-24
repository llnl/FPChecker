OP = -O2 -g

CC = clang

INCLUDE_PATH = -I. -I../../utilities

# Keep the native side in FP32 and initialize MPFR from the same FP32 inputs.
DATASET = -DMINI_DATASET -DDATA_TYPE_IS_FLOAT
DUMP = -DPOLYBENCH_DUMP_ARRAYS
PROTO = -DPOLYBENCH_USE_C99_PROTO

MPFR_INCLUDE =
MPFR_LIB_PATH =

ifdef CONDA_PREFIX
MPFR_INCLUDE = -I$(CONDA_PREFIX)/include
MPFR_LIB_PATH = -L$(CONDA_PREFIX)/lib -Wl,-rpath,$(CONDA_PREFIX)/lib
endif

MPFR_LIBS = $(MPFR_LIB_PATH) -lmpfr -lgmp -lm

TARGET = adi_mpfr
SRC = adi.c

all:
	$(CC) -c $(SRC) $(OP) $(INCLUDE_PATH) $(MPFR_INCLUDE) $(DATASET) $(DUMP) $(PROTO)
	$(CC) -c ../../utilities/polybench.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP) $(PROTO)
	$(CC) -o $(TARGET) adi.o polybench.o $(MPFR_LIBS)

clean:
	rm -rf *.o $(TARGET) *.out *.err
