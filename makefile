all: clean test

clean:
	rm test
test:
	g++ -ggdb -o test compare-string.c hash-string.c hash-table.c procfuse.c test.cpp -D_FILE_OFFSET_BITS=64 -lfuse -lpthread -D_FILE_OFFSET_BITS=64 -lfuse -lpthread
