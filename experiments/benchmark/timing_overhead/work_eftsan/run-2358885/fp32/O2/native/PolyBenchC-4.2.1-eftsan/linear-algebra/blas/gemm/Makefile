EFT_HOME = /g/g90/sharmin1/tutorial/FPChecker/experiments/EFTSanitizer
EFT_PASS = $(EFT_HOME)/llvm_pass/build/EFTSan/libEFTSanitizer.so

POLYBENCH = /g/g90/sharmin1/tutorial/FPChecker/experiments/EFTSanitizer/PolyBenchC-4.2.1-eftsan
UTIL = $(POLYBENCH)/utilities

LLVM10 ?= /g/g90/sharmin1/conda_env/llvm10/bin
LLVM10_LIB ?= /g/g90/sharmin1/conda_env/llvm10/lib

CC = $(LLVM10)/clang
OPT = $(LLVM10)/opt
LLVMLINK = $(LLVM10)/llvm-link
LLC = $(LLVM10)/llc

BENCHMARK = $(notdir $(CURDIR))
SRC = $(BENCHMARK).c

OPT_LEVEL = -O0

CFLAGS = -g $(OPT_LEVEL) -fno-inline -fno-vectorize -fno-slp-vectorize -ffp-contract=off \
         -emit-llvm -I. -I$(UTIL) \
         -DMINI_DATASET -DPOLYBENCH_DUMP_ARRAYS -DDATA_TYPE_IS_FLOAT

LDFLAGS = -L$(EFT_HOME)/runtime/obj \
          -leftsanitizer \
          -L$(LLVM10_LIB) \
          -lmpfr -lgmp -lm \
          -Wl,-rpath,$(EFT_HOME)/runtime/obj \
          -Wl,-rpath,$(LLVM10_LIB)

TARGET = $(BENCHMARK)
BENCHMARK_BC = $(BENCHMARK).bc
POLYBENCH_BC = polybench.bc
LINKED_BC = $(BENCHMARK).linked.bc
OPT_BC = $(BENCHMARK).opt.bc
OBJ = $(BENCHMARK).o

.PHONY: run clean

all: $(TARGET)

mathFunc.txt:
	@touch mathFunc.txt
	@if grep -q "SQRT_FUN\|sqrtf\|sqrt" $(SRC); then echo sqrt >> mathFunc.txt; echo sqrtf >> mathFunc.txt; fi
	@if grep -q "EXP_FUN\|expf\|exp" $(SRC); then echo exp >> mathFunc.txt; echo expf >> mathFunc.txt; fi
	@if grep -q "POW_FUN\|powf\|pow" $(SRC); then echo pow >> mathFunc.txt; echo powf >> mathFunc.txt; fi

$(BENCHMARK_BC): $(SRC) mathFunc.txt
	$(CC) $(CFLAGS) -c $(SRC) -o $(BENCHMARK_BC)

$(POLYBENCH_BC): $(UTIL)/polybench.c
	$(CC) $(CFLAGS) -c $(UTIL)/polybench.c -o $(POLYBENCH_BC)

$(LINKED_BC): $(BENCHMARK_BC) $(POLYBENCH_BC)
	$(LLVMLINK) $(BENCHMARK_BC) $(POLYBENCH_BC) -o $(LINKED_BC)

$(OPT_BC): $(LINKED_BC)
	$(OPT) -load $(EFT_PASS) -eftsan \
	  -eftsan-enable-debugging \
	  -eftsan-detect-all-rounding-errors \
	  -eftsan-detect-all-exceptions \
	  < $(LINKED_BC) > $(OPT_BC)

$(OBJ): $(OPT_BC)
	$(LLC) $(OPT_BC) -filetype=obj -o $(OBJ)

$(TARGET): $(OBJ)
	$(CC) -g $(OPT_LEVEL) $(OBJ) -o $(TARGET) $(LDFLAGS)

run: clean $(TARGET)
	./run.sh ./$(TARGET)

clean:
	rm -f *.bc *.o *.out *.err error.log mathFunc.txt $(TARGET)
