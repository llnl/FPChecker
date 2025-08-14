# Compiler and flags
CXX = clang++-fpchecker
CXXFLAGS = -Wno-format -std=c++11 -g -O2 -fno-inline-functions -fno-vectorize -fno-slp-vectorize

# List of executables to build
EXECUTABLES = simple_sum vector_dot_product

# Default target builds all executables
all: $(EXECUTABLES)

# Rule to link simple_sum
simple_sum: simple_sum.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Rule to link vector_dot_product
vector_dot_product: vector_dot_product.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Generic pattern rule to compile .cpp files into .o files.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(EXECUTABLES) *.o .fpc_logs fpc-report

.PHONY: all clean
