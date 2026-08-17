CC=gcc
CFLAGS=-O2 -w -I. -DWORKING_T=double
SRC=ep.c c_randdp.c c_print_results.c c_timers.c wtime.c
ep_fp64: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lm -o ep_fp64
run: ep_fp64
	./ep_fp64
clean:
	rm -f ep_fp64
