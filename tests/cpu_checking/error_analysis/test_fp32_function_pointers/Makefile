
OP = 	-O2 -fno-vectorize -fno-slp-vectorize #-DFPC_DEBUG_ERROR_ANALYSIS
CXX = FPC_INSTRUMENT_ERR_TRACKING=1 clang++-fpchecker

# Platform-specific flags for dlopen/dlsym and exporting executable symbols
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
DLFLAGS = -ldl -Wl,--export-dynamic
else
DLFLAGS =
endif

all:
	$(CXX) -c main.cpp $(OP)
	$(CXX) -o main main.o $(OP) $(DLFLAGS)

clean:
	rm -rf *.o main __pycache__ .fpc_logs .fpc_log.txt