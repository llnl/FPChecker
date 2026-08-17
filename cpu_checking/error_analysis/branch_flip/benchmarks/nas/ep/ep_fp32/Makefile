CC=gcc
CFLAGS=-O2 -w -I. -DWORKING_T=float -DWORKING_T_IS_FLOAT=1
SRC=ep.c c_randdp.c c_print_results.c c_timers.c wtime.c
ep_fp32: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lm -o ep_fp32
run: ep_fp32
	./ep_fp32
clean:
	rm -f ep_fp32
