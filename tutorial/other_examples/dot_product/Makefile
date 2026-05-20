# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++11 -fno-vectorize -fno-slp-vectorize -O2 -g \
	-include ../../../build/install/src/Runtime_error.h \
	-fpass-plugin=../../../build/install/lib/libfpchecker_error.dylib -DFPC_CALCULATE_LOCAL_ERRORS_ONLY

# Source file
SRC = dotprod.cpp
OBJ = $(SRC:.cpp=.o)
EXECUTABLE_NAME = dotprod

# Default target
all: $(EXECUTABLE_NAME)

# Rule to compile the source file
$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	# Create LLVM IR file for the source file for debugging purposes
	$(CXX) $(CXXFLAGS) -S -emit-llvm $< -o $(@:.o=.ll)

# Rule to link the executable
$(EXECUTABLE_NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean target
clean:
	rm -rf $(OBJ) $(EXECUTABLE_NAME) .fpc_logs fpc-report *.out *.ll

.PHONY: all clean
