# Compiler and flags
CXX = clang++-fpchecker
CXXFLAGS = -Wno-format -std=c++11 -g -O2  -fno-inline-functions

# Source file(s)
SRC = matrix_multiply.cpp
OBJ = $(SRC:.cpp=.o)
EXECUTABLE_NAME = matrix_multiply


# Default target
all: $(EXECUTABLE_NAME)

# Compile the source file
$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link with the library
$(EXECUTABLE_NAME): $(OBJ) $(LIB_FILE)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean up
clean:
	rm -rf $(OBJ) $(EXECUTABLE_NAME) .fpc_logs fpc-report

.PHONY: all clean
