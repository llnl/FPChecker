CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2 -g -Wall

# FPChecker wrappers (should be in your PATH)
FPC_CXX    ?= clang++-fpchecker
FPC_REPORT ?= fpc-create-report

# We compile the instrumented build at -O0 so the compiler does not
# constant-fold or inline away the FP operations we want to measure.
FPC_CXXFLAGS ?= -std=c++11 -O0 -g -Wall

N   ?= 200000
AMP ?= 32
A   ?= 1.0
B   ?= 1.0e8
C   ?= 1.0
ARGS = $(N) $(AMP) $(A) $(B) $(C)

FP32_OBJ  = fp32.o
MIXED_OBJ = mixed.o
FP64_OBJ  = fp64.o

all: fp32 mixed fp64

# --- FP32 (instrumented) ---------------------------------------------------
fp32: $(FP32_OBJ)
	$(FPC_CXX) $(FPC_CXXFLAGS) -o $@ $^

$(FP32_OBJ): fp32.cpp
	FPC_INSTRUMENT_ERR_TRACKING=1 $(FPC_CXX) $(FPC_CXXFLAGS) -c $< -o $@

# --- Mixed / FP64 (plain builds for comparison) ----------------------------
mixed: $(MIXED_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

fp64: $(FP64_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(MIXED_OBJ): mixed.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(FP64_OBJ): fp64.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Run / report targets ---------------------------------------------------
run-fp32: fp32
	./fp32 $(ARGS)

report-shell: run-fp32
	$(FPC_REPORT) -s rounding .

report-html: run-fp32
	$(FPC_REPORT) .

run-mixed: mixed
	./mixed $(ARGS)

run-fp64: fp64
	./fp64 $(ARGS)

compare: fp32 mixed fp64 compare_accuracy.py
	./compare_accuracy.py ./fp32 ./mixed ./fp64 $(ARGS)

clean:
	rm -rf fp32 mixed fp64 $(FP32_OBJ) $(MIXED_OBJ) $(FP64_OBJ) \
	       .fpc_logs fpc-report .fpc_log.txt

.PHONY: all run-fp32 report-shell report-html run-mixed run-fp64 compare clean
