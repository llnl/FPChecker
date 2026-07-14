CFLAGS=-O0 -g -std=c11 -L$(EFT_HOME)/runtime/obj -leftsanitizer -L$(MPFR_HOME)/mpfr-4.1.0/src/.libs -lmpfr -lm  -lpthread  
OPT = opt -load $(LLVM_PASS_LIB) -eftsan-detect-all-rounding-errors -eftsan-detect-all-exceptions -eftsan-enable-debugging
CSRC = $(wildcard *.c)
CPPSRC = $(wildcard *.cpp)
COUT = $(patsubst %.c,%.c.bc,$(CSRC)) $(patsubst %.cpp,%.c.bc,$(CPPSRC))
OCOUT = $(patsubst %.c.bc,%.opt.bc,$(COUT))
OUT = $(patsubst %.opt.bc,%.c.o,$(OCOUT))
FOUT = $(patsubst %.c.o,%.fp.o,$(OUT))


CC = clang -pg -O0 -g
CCE = clang -pg -O0 -fno-inline -emit-llvm -g

CCP = clang++ -pg -O0 -g
CPCE = clang++ -pg -O0 -fno-inline -emit-llvm -g

all: $(FOUT) $(CFOUT)

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

%.cpp.o: %.opt.cpp.bc
	llc $< -filetype=obj -o $@  

%.opt.cpp.bc: %.cpp.bc
	$(OPT) < $< > $@

%.cpp.bc: %.cpp
	$(CPCE) -c -o $@ $< 
clean:
	rm -rf *.ll *.bc *.o *.sout *.dSYM *~ *.log functions.txt  *.out
