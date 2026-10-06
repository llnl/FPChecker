# EFT_HOME = /g/g90/sharmin1/tutorial/FPChecker/experiments/EFTSanitizer
# EFT_PASS = $(EFT_HOME)/llvm_pass/build/EFTSan/libEFTSanitizer.so

# POLYBENCH = /g/g90/sharmin1/tutorial/FPChecker/experiments/EFTSanitizer/PolyBenchC-4.2.1-eftsan
# UTIL = $(POLYBENCH)/utilities

# CC = clang
# OPT = opt
# LLVMLINK = llvm-link
# LLC = llc

# CFLAGS = -g -O0 -fno-inline -emit-llvm -I. -I$(UTIL) \
#          -DMINI_DATASET -DPOLYBENCH_DUMP_ARRAYS -DDATA_TYPE_IS_FLOAT

# LDFLAGS = $(EFT_HOME)/runtime/obj/libeftsanitizer.so \
#           -L$(CONDA_PREFIX)/lib \
#           -lmpfr -lgmp -lm \
#           -Wl,-rpath,$(EFT_HOME)/runtime/obj \
#           -Wl,-rpath,$(CONDA_PREFIX)/lib

# TARGET = gramschmidt.fp32.O0.eft

# all: $(TARGET)

# mathFunc.txt:
# 	echo sqrt > mathFunc.txt
# 	echo sqrtf >> mathFunc.txt

# gramschmidt.fp32.O0.bc: gramschmidt.c mathFunc.txt
# 	$(CC) $(CFLAGS) -c gramschmidt.c -o $@


# polybench.fp32.O0.bc: $(UTIL)/polybench.c
# 	$(CC) $(CFLAGS) -c $(UTIL)/polybench.c -o $@

# linked.fp32.O0.bc: gramschmidt.fp32.O0.bc polybench.fp32.O0.bc
# 	$(LLVMLINK) gramschmidt.fp32.O0.bc polybench.fp32.O0.bc -o $@

# linked.fp32.O0.eft.bc: linked.fp32.O0.bc
# 	$(OPT) -load $(EFT_PASS) -eftsan \
# 	  -eftsan-detect-all-rounding-errors \
# 	  -eftsan-detect-all-exceptions \
# 	  < linked.fp32.O0.bc > linked.fp32.O0.eft.bc

# linked.fp32.O0.eft.o: linked.fp32.O0.eft.bc
# 	$(LLC) linked.fp32.O0.eft.bc -filetype=obj -o linked.fp32.O0.eft.o

# $(TARGET): linked.fp32.O0.eft.o
# 	$(CC) -g -O2 linked.fp32.O0.eft.o -o $(TARGET) $(LDFLAGS)

# run: $(TARGET)
# 	rm -f error.log fp32.out fp32.err
# 	LD_LIBRARY_PATH=$(EFT_HOME)/runtime/obj:$(CONDA_PREFIX)/lib:$$LD_LIBRARY_PATH \
# 	./$(TARGET) > fp32.out 2> fp32.err
	

# gdb: $(TARGET)
# 	LD_LIBRARY_PATH=$(EFT_HOME)/runtime/obj:$(CONDA_PREFIX)/lib:$$LD_LIBRARY_PATH \
# 	gdb ./$(TARGET)

# clean:
# 	rm -f *.bc *.o  *.out *.err error.log mathFunc.txt gramschmidt.fp32.O0.eft



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

# Change this to -O0 if you want to follow the default EFTSanitizer GitHub setting.
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
# 	@if grep -q "cosf\|cos" $(SRC); then echo cos >> mathFunc.txt; echo cosf >> mathFunc.txt; fi
# 	@if grep -q "sinf\|sin" $(SRC); then echo sin >> mathFunc.txt; echo sinf >> mathFunc.txt; fi

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

# run: $(TARGET)
# 	rm -f error.log fp32.out fp32.err
# 	LD_LIBRARY_PATH=$(EFT_HOME)/runtime/obj:/g/g90/sharmin1/conda_env/llvm10/lib:$$LD_LIBRARY_PATH \
# 	./$(TARGET) > fp32.out 2> fp32.err
# 	@if [ -f error.log ]; then cat error.log; fi

# gdb: $(TARGET)
# 	LD_LIBRARY_PATH=$(EFT_HOME)/runtime/obj:/g/g90/sharmin1/conda_env/llvm10/lib:$$LD_LIBRARY_PATH \
# 	gdb ./$(TARGET)

clean:
	rm -f *.bc *.o *.out *.err error.log mathFunc.txt $(TARGET)
