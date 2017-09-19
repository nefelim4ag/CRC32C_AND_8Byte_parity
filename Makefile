CFLAGS=-O2 -Wall -Werror -pthread

default all: 8byte_parity

xxhash.o: xxhash.c
	$(CC) $(CFLAGS) -c $? -o $@

8byte_parity.o: 8byte_parity.c
	$(CC) $(CFLAGS) -c $? -o $@

crc32.o: crc32.c
	$(CC) $(CFLAGS) -c $? -o $@

8byte_parity: 8byte_parity.o xxhash.o crc32.o
	$(CC) $(CFLAGS) -o $@ $?


clean: ## Cleanup
	rm -fv *.o

help: ## Show help
	@fgrep -h "##" $(MAKEFILE_LIST) | fgrep -v fgrep | sed -e 's/\\$$//' | sed -e 's/##/\t/'
