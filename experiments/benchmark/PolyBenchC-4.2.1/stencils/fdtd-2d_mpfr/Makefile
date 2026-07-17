OP = -O0 -g

CC = clang

INCLUDE_PATH = -I. -I.. -I../../utilities

# -DDATA_TYPE_IS_FLOAT makes DATA_TYPE = float (fp32). Without it the header
# defaults to double, and your "fp32" side silently runs in double.
DATASET = -DMINI_DATASET -DDATA_TYPE_IS_FLOAT
DUMP = -DPOLYBENCH_DUMP_ARRAYS

# PolyBench uses C99 variable-length-array prototypes for the 2D array args.
PROTO = -DPOLYBENCH_USE_C99_PROTO

MPFR_INCLUDE =
MPFR_LIB_PATH =

ifdef CONDA_PREFIX
MPFR_INCLUDE = -I$(CONDA_PREFIX)/include
MPFR_LIB_PATH = -L$(CONDA_PREFIX)/lib -Wl,-rpath,$(CONDA_PREFIX)/lib
endif

MPFR_LIBS = $(MPFR_LIB_PATH) -lmpfr -lgmp -lm

TARGET = fdtd-2d

all:
	$(CC) -c $(TARGET).c $(OP) $(INCLUDE_PATH) $(MPFR_INCLUDE) $(DATASET) $(DUMP) $(PROTO)
	$(CC) -c ../../utilities/polybench.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP) $(PROTO)
	$(CC) -o $(TARGET) $(TARGET).o polybench.o $(MPFR_LIBS)


clean:
	rm -rf *.o $(TARGET) *.out *.err