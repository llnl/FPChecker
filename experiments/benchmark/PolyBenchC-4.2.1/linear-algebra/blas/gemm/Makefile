# OP = -O2 -fno-vectorize -fno-slp-vectorize

# CC = FPC_INSTRUMENT_ERR_TRACKING=1 /g/g90/sharmin1/tutorial/install/bin/clang-fpchecker

# INCLUDE_PATH = -I. -I../../../utilities
# DATASET = -DMINI_DATASET
# DUMP = -DPOLYBENCH_DUMP_ARRAYS

# all:
# 	$(CC) -c gemm.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP)
# 	$(CC) -c ../../../utilities/polybench.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP)
# 	$(CC) -o gemm_fpchecker gemm.o polybench.o -lm

# native:
# 	clang -c gemm.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP)
# 	clang -c ../../../utilities/polybench.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP)
# 	clang -o gemm_native gemm.o polybench.o -lm

# clean:
# 	rm -rf *.o gemm_fpchecker gemm_native __pycache__ .fpc_logs .fpc_log.txt

OP = -O2 -fno-vectorize -fno-slp-vectorize

CC = FPC_INSTRUMENT_ERR_TRACKING=1 clang-fpchecker

INCLUDE_PATH = -I. -I../../../utilities
DATASET = -DMINI_DATASET
DUMP = -DPOLYBENCH_DUMP_ARRAYS

all:
	$(CC) -c gemm.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP)
	$(CC) -c ../../../utilities/polybench.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP)
	$(CC) -o gemm gemm.o polybench.o -lm

clean:
	rm -rf *.o gemm __pycache__ .fpc_logs .fpc_log.txt