# Compiler and flags
CXX = clang++-fpchecker
CXXFLAGS = -Wno-format -std=c++11 -g -O2

# Source file
SRC = reaction_diffusion.cpp
OBJ = $(SRC:.cpp=.o)
EXECUTABLE_NAME = reaction_diffusion

# Default target
all: $(EXECUTABLE_NAME)

# Rule to compile the source file
$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to link with the library
$(EXECUTABLE_NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean target
clean:
	rm -rf $(OBJ) $(EXECUTABLE_NAME) fpc-report .fpc_logs .fpc_log.txt

.PHONY: all clean
