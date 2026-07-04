OP = -O2 -fno-vectorize -fno-slp-vectorize -std=c++17
FPC_CXX = FPC_INSTRUMENT_ERR_TRACKING=1 clang++-fpchecker
RAW_CXX = clang++

.PHONY: all clean

all: main

main: main.o uninstrumented_writer.o
	$(FPC_CXX) main.o uninstrumented_writer.o -o $@ $(OP)

main.o: main.cpp
	$(FPC_CXX) -c $< -o $@ $(OP)

uninstrumented_writer.o: uninstrumented_writer.cpp
	$(RAW_CXX) -c $< -o $@ $(OP)

clean:
	rm -rf *.o main __pycache__ .fpc_logs .fpc_log.txt fpc-report
