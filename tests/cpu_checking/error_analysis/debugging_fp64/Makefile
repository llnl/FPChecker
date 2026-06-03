CXX = clang++
CXXFLAGS = -Wno-format -std=c++11 -g -O2

# FP64 FPChecker plugin/runtime
FPC64_PLUGIN = /g/g90/sharmin1/tutorial/FPChecker/build/libfpchecker_error_fp64.so
FPC64_RUNTIME = /g/g90/sharmin1/tutorial/install/src/Runtime_error_FP64.h

# Source file
SRC = simple_sum.cpp
OBJ = $(SRC:.cpp=.o)
EXECUTABLE_NAME = simple_sum

# Default target
all: $(EXECUTABLE_NAME)

# Compile source file with FP64 instrumentation
$(OBJ): $(SRC)
	$(CXX) -fpass-plugin=$(FPC64_PLUGIN) -include $(FPC64_RUNTIME) $(CXXFLAGS) -c $< -o $@

# Link executable
$(EXECUTABLE_NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

# Clean target
clean:
	rm -rf $(OBJ) $(EXECUTABLE_NAME) .fpc_logs fpc-report

.PHONY: all clean