# Compiler and flags
CXX = clang++-fpchecker
CXXFLAGS = -Wno-format -std=c++11 -g -O2 -fno-vectorize -fno-slp-vectorize

# Source files
SRC = cg.cpp cg_mixed.cpp cg_FP64.cpp
OBJ = $(SRC:.cpp=.o)
#EXECUTABLES = cg cg_mixed cg_FP64
EXECUTABLES = cg

# Default target
all: $(EXECUTABLES)

# Rule to compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to link executables
cg: cg.o
	$(CXX) $(CXXFLAGS) -o $@ $^

cg_mixed: cg_mixed.o
	$(CXX) $(CXXFLAGS) -o $@ $^

cg_FP64: cg_FP64.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean target
clean:
	rm -rf $(OBJ) $(EXECUTABLES) fpc-report .fpc_logs .fpc_log.txt

.PHONY: all clean
