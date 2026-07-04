OP = -O2 -fno-vectorize -fno-slp-vectorize -std=c++17
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 clang++-fpchecker

.PHONY: all clean

all: main

main: main.o
	$(CXX) main.o -o $@ $(OP)

main.o: main.cpp
	$(CXX) -c $< -o $@ $(OP)

clean:
	rm -rf *.o main __pycache__ .fpc_logs .fpc_log.txt fpc-report
