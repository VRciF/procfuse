all:
	gcc -ggdb -W -Wall -Werror -o example1 -I.. ../procfuse-amalgamation.c example1.c `pkg-config fuse --cflags --libs` -lpthread
	gcc -ggdb -W -Wall -Werror -o example2 -I.. ../procfuse-amalgamation.c example2.c `pkg-config fuse --cflags --libs` -lpthread
	g++ -ggdb -W -Wall -Werror -o example3 -I.. ../procfuse-amalgamation.c example3.cpp `pkg-config fuse --cflags --libs` -lpthread