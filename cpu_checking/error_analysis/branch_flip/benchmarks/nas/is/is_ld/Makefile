CC=gcc
CFLAGS=-O2 -w -I.
SRC=is.c c_timers.c c_print_results.c wtime.c
is_ld: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lm -o is_ld
run: is_ld
	./is_ld
clean:
	rm -f is_ld
