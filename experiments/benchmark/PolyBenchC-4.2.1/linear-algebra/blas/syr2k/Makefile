OP = -O2 -fno-vectorize -fno-slp-vectorize

CC = FPC_INSTRUMENT_ERR_TRACKING=1 clang-fpchecker

INCLUDE_PATH = -I. -I../../../utilities
DATASET = -DMINI_DATASET
DUMP = -DPOLYBENCH_DUMP_ARRAYS

all:
	$(CC) -c syr2k.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP)
	$(CC) -c ../../../utilities/polybench.c $(OP) $(INCLUDE_PATH) $(DATASET) $(DUMP)
	$(CC) -o syr2k syr2k.o polybench.o -lm

clean:
	rm -rf *.o syr2k __pycache__ .fpc_logs .fpc_log.txt
