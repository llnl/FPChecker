#CFLAGS=-O0 -fno-slp-vectorize -std=c11  -L$(FPROOT)/obj -lfpsanitizer -lmpfr -lm -pthread -ltbb -lstdc++
CFLAGS=-O0 -g -std=c11 -L$(EFT_HOME)/runtime/obj -lfpsanitizer -L$(MPFR_HOME)/mpfr-4.1.0/src/.libs -lmpfr -lm  -lpthread -L$(TBB_HOME)/tbb/build  -ltbb 
OPT = opt -load $(EFT_HOME)/llvm_pass/build/FPSan/libFPSanitizer.so -fpsan
CSRC = $(wildcard *.c)
COUT = $(patsubst %.c,%.c.bc,$(CSRC))
OCOUT = $(patsubst %.c.bc,%.opt.bc,$(COUT))
OUT = $(patsubst %.opt.bc,%.c.o,$(OCOUT))
FOUT = $(patsubst %.c.o,%.fp.o,$(OUT))

CPPSRC = $(wildcard *.cpp)
CPOUT = $(patsubst %.cpp,%.cpp.bc,$(CPSRC))
OCPOUT = $(patsubst %.cpp.bc,%.opt.bc,$(CPOUT))
COUT = $(patsubst %.opt.bc,%.cpp.o,$(OCPOUT))
CFOUT = $(patsubst %.cpp.o,%.fp.o,$(COUT))

CC = clang -pg -O0 -g
CCE = clang -pg -O0 -fno-inline -emit-llvm -g

CCP = clang++ -pg -O0 -g
CPCE = clang++ -pg -O0 -fno-inline -emit-llvm -g
all: $(FOUT) 

.PRECIOUS: %.opt.bc %.c.bc

%.fp.o: %.c.o
	$(CC) $^ -o $@  $(CFLAGS) 
	chmod u+x $@

%.c.o: %.opt.bc
	llc $< -filetype=obj -o $@  

%.opt.bc: %.c.bc
	$(OPT) < $< > $@

%.c.bc: %.c
	$(CCE) -c -o $@ $< 

%.fp.o: %.cpp.o
	$(CCP) $^ -o $@  $(CFLAGS) 
	chmod u+x $@

%.cpp.o: %.opt.bc
	llc $< -filetype=obj -o $@  

%.opt.bc: %.cpp.bc
	$(OPT) < $< > $@

%.cpp.bc: %.c
	$(CPCE) -c -o $@ $< 
clean:
	rm -rf *.ll *.bc *.o *.sout *.dSYM *~ *.log functions.txt  *.out
