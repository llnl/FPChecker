# Compiler and flags
CXX = clang++-fpchecker
CXXFLAGS = -Wno-format -std=c++11 -g -O2 -fno-vectorize -fno-slp-vectorize

# Source files
SRC = cg.cpp
OBJ = $(SRC:.cpp=.o)
EXECUTABLES = cg

# Default target
all: $(EXECUTABLES)

# Rule to compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to link executables
cg: cg.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean target
clean:
	rm -rf $(OBJ) $(EXECUTABLES) fpc-report .fpc_logs .fpc_log.txt *.ll *.png

.PHONY: all clean
