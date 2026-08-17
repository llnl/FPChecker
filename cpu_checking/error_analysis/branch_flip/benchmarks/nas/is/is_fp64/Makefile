CC=gcc
CFLAGS=-O2 -w -I.
SRC=is.c c_timers.c c_print_results.c wtime.c
is_fp64: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lm -o is_fp64
run: is_fp64
	./is_fp64
clean:
	rm -f is_fp64
