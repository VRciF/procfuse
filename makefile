all: clean amalgamation test

object:
	gcc -ggdb -W -Wall -pedantic -c procfuse.c -I. -lfuse -lpthread -D_FILE_OFFSET_BITS=64 -lfuse -lpthread

clean:
	touch test
	rm test
test: amalgamation
#	g++ -ggdb -W -Wall -pedantic -o examples/test -I. procfuse-amalgamation.c examples/test.cpp -D_FILE_OFFSET_BITS=64 -lfuse -lpthread
	g++ -ggdb -W -Wall -pedantic -o examples/test -I. hash-table.c hash-string.c hash-int.c compare-int.c compare-string.c 	procfuse.c examples/test.cpp -D_FILE_OFFSET_BITS=64 -lfuse -lpthread
amalgamation:
	@echo '#include "procfuse-amalgamation.h"' > procfuse-amalgamation.c
	@cat compare-string.h compare-int.h hash-int.h hash-string.h hash-table.h > procfuse-amalgamation.h
	@echo "" >> procfuse-amalgamation.h
	@grep -v '#include "' procfuse.h >> procfuse-amalgamation.h
	@echo "" >> procfuse-amalgamation.h
	
	@grep -v '#include "' compare-string.c >> procfuse-amalgamation.c
	@echo "" >> procfuse-amalgamation.c
	@grep -v '#include "' compare-int.c >> procfuse-amalgamation.c
	@echo "" >> procfuse-amalgamation.c
	@grep -v '#include "' hash-int.c >> procfuse-amalgamation.c
	@echo "" >> procfuse-amalgamation.c
	@grep -v '#include "' hash-string.c >> procfuse-amalgamation.c
	@echo "" >> procfuse-amalgamation.c
	@grep -v '#include "' hash-table.c >> procfuse-amalgamation.c
	@echo "" >> procfuse-amalgamation.c
	@grep -v '#include "' procfuse.c >> procfuse-amalgamation.c
	@echo "" >> procfuse-amalgamation.c