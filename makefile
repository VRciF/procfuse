all: clean test

clean:
	touch test
	rm test
test:
	g++ -ggdb -W -Wall -Werror -pedantic -o test compare-string.c hash-string.c hash-table.c procfuse.c test.cpp -D_FILE_OFFSET_BITS=64 -lfuse -lpthread -D_FILE_OFFSET_BITS=64 -lfuse -lpthread
