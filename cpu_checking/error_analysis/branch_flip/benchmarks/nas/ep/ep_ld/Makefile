CC=gcc
CFLAGS=-O2 -w -I. "-DWORKING_T=long double" -DWORKING_T_IS_LD=1
SRC=ep.c c_randdp.c c_print_results.c c_timers.c wtime.c
ep_ld: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lm -o ep_ld
run: ep_ld
	./ep_ld
clean:
	rm -f ep_ld
