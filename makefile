all: clean amalgamation test

clean:
	touch test
	rm test
test: amalgamation
#	g++ -ggdb -W -Wall -Werror -pedantic -o test compare-string.c hash-string.c hash-table.c procfuse.c test.cpp -D_FILE_OFFSET_BITS=64 -lfuse -lpthread -D_FILE_OFFSET_BITS=64 -lfuse -lpthread
	g++ -ggdb -W -Wall -Werror -pedantic -o test test.cpp -D_FILE_OFFSET_BITS=64 -lfuse -lpthread -D_FILE_OFFSET_BITS=64 -lfuse -lpthread
amalgamation:
	@cat compare-string.h hash-string.h hash-table.h > procfuse-amalgamation.h
	@echo "" >> procfuse-amalgamation.h
	@grep -v '#include "' procfuse.h >> procfuse-amalgamation.h
	@echo "" >> procfuse-amalgamation.h
	@grep -v '#include "' compare-string.c >> procfuse-amalgamation.h
	@echo "" >> procfuse-amalgamation.h
	@grep -v '#include "' hash-string.c >> procfuse-amalgamation.h
	@echo "" >> procfuse-amalgamation.h
	@grep -v '#include "' hash-table.c >> procfuse-amalgamation.h
	@echo "" >> procfuse-amalgamation.h
	@grep -v '#include "' procfuse.c >> procfuse-amalgamation.h
	@echo "" >> procfuse-amalgamation.h