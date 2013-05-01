/*
 * test.cpp
 *
 */

#include "procfuse.h"

#include <set>
#include <string>
#include <iostream>
#include <fstream>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>

std::set<std::string> hosts;
const char *absolutemountpoint = "/tmp/procfuse.test";

int onFuseRead(const char *path, char *buffer, size_t size, off_t offset){
	int wlen = 0;

	if(std::string(path)=="/port"){
		const char *port = "80\n";
		if(offset<3){
		    snprintf(buffer, size, "%s", port+offset);
		    wlen = 2;
		}
	}
	else if(std::string(path)=="/net/hosts/list"){
		std::set<std::string>::iterator it;
		char mybuffer [512];

		std::fstream filestr;
		filestr.rdbuf()->pubsetbuf(buffer,size);

  	    off_t countingoffset = 0;
		for(it = hosts.begin(); it!=hosts.end(); it++){
			if(offset>=countingoffset && offset<countingoffset+it->length()){
				break;
			}
			countingoffset += it->length();
		}

		countingoffset = offset-countingoffset;
		int written = 0;

		for(;it!=hosts.end();it++,countingoffset=0){

			written = filestr.rdbuf()->sputn(it->data()+countingoffset,it->length()-countingoffset);
			if(written<=0){ break; }
			wlen += written;

			written = filestr.rdbuf()->sputc('\n');
			if(written<=0){ break; }
			wlen += written;

		}
	}

	return wlen;
}
int onFuseWrite(const char *path, const char *buffer, size_t size, off_t offset){
	int rval = 0;

	if(std::string(path)=="/net/hosts/add"){
		hosts.insert(std::string(buffer, size));
	}
	if(std::string(path)=="/net/hosts/del"){
		hosts.erase(std::string(buffer, size));
	}

	return rval;
}

void sig_handler(int signum)
{
	printf("%s:%d\n",__FILE__,__LINE__);
	//int rval = umount(absolutemountpoint);
	//printf("umount: %d:%s\n", rval, strerror(errno));
	fuse_exit(fuse_get_context()->fuse);
}

int main(int argc,char **argv){
	struct procfuse pf;
	struct procfuse_accessor access;

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

	mkdir(absolutemountpoint, 0777);

	procfuse_ctor(&pf, argv[0], absolutemountpoint, NULL);

	access.onFuseRead = onFuseRead;
	access.onFuseWrite = onFuseWrite;

	procfuse_registerNode(&pf, "/port", access);

	access.onFuseWrite = NULL; /* read only */
	procfuse_registerNode(&pf, "/net/hosts/list", access);

	access.onFuseWrite = onFuseWrite;
	access.onFuseRead = NULL; /* write only */
	procfuse_registerNode(&pf, "/net/hosts/add", access);
	procfuse_registerNode(&pf, "/net/hosts/del", access);

	printf("%s:%d\n",__FILE__,__LINE__);
	procfuse_main(&pf, PROCFUSE_BLOCK);
	printf("%s:%d\n",__FILE__,__LINE__);

	unlink(absolutemountpoint);
	umount(absolutemountpoint);

	procfuse_dtor(&pf);
}
