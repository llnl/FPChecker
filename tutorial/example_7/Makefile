# Compiler and flags
CXX = clang++-fpchecker
CXXFLAGS = -Wno-format -std=c++11 -g -O2 -fno-vectorize -fno-slp-vectorize

EXECUTABLES = cg cg_mixed cg_FP64

# Default target
all: $(EXECUTABLES)

# FP32 with rounding-error instrumentation (the version we analyze)
cg.o: cg.cpp
	FPC_INSTRUMENT_ERR_TRACKING=1 $(CXX) $(CXXFLAGS) -c $< -o $@

cg: cg.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Mixed precision — no instrumentation (plain build)
cg_mixed.o: cg_mixed.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

cg_mixed: cg_mixed.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# FP64 reference — no instrumentation (plain build)
cg_FP64.o: cg_FP64.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

cg_FP64: cg_FP64.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean target
clean:
	rm -rf *.o $(EXECUTABLES) fpc-report .fpc_logs .fpc_log.txt

.PHONY: all clean
