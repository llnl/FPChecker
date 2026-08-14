EFT_HOME = /g/g90/sharmin1/tutorial/FPChecker/experiments/EFTSanitizer
EFT_PASS = $(EFT_HOME)/llvm_pass/build/EFTSan/libEFTSanitizer.so

LLVM10 ?= /g/g90/sharmin1/conda_env/llvm10/bin
LLVM10_LIB ?= /g/g90/sharmin1/conda_env/llvm10/lib

CC = $(LLVM10)/clang
OPT = $(LLVM10)/opt
LLC = $(LLVM10)/llc

SRC = toy_return_fails.c
TARGET = toy

OPT_LEVEL = -O0

CFLAGS = -g $(OPT_LEVEL) -fno-inline -fno-vectorize -fno-slp-vectorize -ffp-contract=off
BCFLAGS = $(CFLAGS) -emit-llvm

LDFLAGS = -L$(EFT_HOME)/runtime/obj \
          -leftsanitizer \
          -L$(LLVM10_LIB) \
          -lmpfr -lgmp -lm \
          -Wl,-rpath,$(EFT_HOME)/runtime/obj \
          -Wl,-rpath,$(LLVM10_LIB)

BC = toy.bc
OPT_BC = toy.eft.bc
OBJ = toy.o

.PHONY: all run gdb clean

all: $(TARGET)

$(BC): $(SRC)
	$(CC) $(BCFLAGS) -c $(SRC) -o $(BC)

$(OPT_BC): $(BC)
	$(OPT) -load $(EFT_PASS) -eftsan \
	  -eftsan-enable-debugging \
	  -eftsan-detect-all-rounding-errors \
	  -eftsan-detect-all-exceptions \
	  < $(BC) > $(OPT_BC)

$(OBJ): $(OPT_BC)
	$(LLC) $(OPT_BC) -filetype=obj -o $(OBJ)

$(TARGET): $(OBJ)
	$(CC) -g $(OPT_LEVEL) $(OBJ) -o $(TARGET) $(LDFLAGS)

run: clean $(TARGET)
	./$(TARGET)

gdb: $(TARGET)
	gdb -q ./$(TARGET)

clean:
	rm -f *.bc *.o *.out *.err error.log $(TARGET)
