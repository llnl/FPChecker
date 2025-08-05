# Compiler and flags
CXX = clang++-fpchecker
 CXXFLAGS = -Wno-format -std=c++11 -g -O2
#  CXXFLAGS = -Wno-format -std=c++11 -g -O0

# Source file(s)
SRC = derivatives.cpp
OBJ = $(SRC:.cpp=.o)
EXECUTABLE_NAME = derivatives

# Library location and name (reused from common folder)
LIB_DIR = ../tutorial/common
LIB_NAME = solverslib
LIB_FILE = $(LIB_DIR)/lib$(LIB_NAME).a

# Default target
all: $(EXECUTABLE_NAME)

# Compile the source file
$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link with the library
$(EXECUTABLE_NAME): $(OBJ) $(LIB_FILE)
	$(CXX) $(CXXFLAGS) -o $@ $^ -L$(LIB_DIR) -l$(LIB_NAME)

# Clean up
clean:
	rm -rf $(OBJ) $(EXECUTABLE_NAME) .fpc_logs fpc-report

.PHONY: all clean
