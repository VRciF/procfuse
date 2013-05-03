/*
 * test.cpp
 *
 */

#include "procfuse.h"

#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>

std::string hosts;
const char *absolutemountpoint = "/tmp/procfuse.test";
struct procfuse pf;

int onFuseRead(const char *path, char *buffer, size_t size, off_t offset, int){
	int wlen = 0;

	if(std::string(path)=="/port"){
		const char *port = "80\n";
		if(offset<3){
		    snprintf(buffer, size, "%s", port+offset);
		    wlen = 2;
		}
	}
	else if(std::string(path)=="/net/hosts/list"){
		if(offset<(off_t)hosts.length()){
			size_t cpylen = hosts.length()-offset;

			if(cpylen>size) cpylen = size;

			memcpy(buffer, hosts.data()+offset, cpylen);

			wlen = cpylen;
		}
	}

	return wlen;
}
int onFuseWrite(const char *path, const char *buffer, size_t size, off_t, int){
	int rval = 0;

	std::string s(buffer, size);
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	s.append(";");
	std::size_t pos = hosts.find(s);
	if(std::string(path)=="/net/hosts/add"){
		if(pos==std::string::npos){
			hosts.append(s);
		}
	}
	if(std::string(path)=="/net/hosts/del"){
		if(pos!=std::string::npos){
			hosts.replace(pos, s.length(),std::string());
		}

	}

	rval = size;

	return rval;
}

void sig_handler(int)
{
	procfuse_teardown(&pf);
}

int main(int,char **argv){
	struct procfuse_accessor access;
	memset(&access, '\0', sizeof(access));

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

	mkdir(absolutemountpoint, 0777);

	procfuse_ctor(&pf, argv[0], absolutemountpoint, NULL);

	access.onFuseRead = onFuseRead;
	access.onFuseWrite = onFuseWrite;

	procfuse_registerNode(&pf, "/port", access);

	access.onFuseWrite = NULL; // read only
	procfuse_registerNode(&pf, "/net/hosts/list", access);

	access.onFuseWrite = onFuseWrite;
	access.onFuseRead = NULL; // write only
	procfuse_registerNode(&pf, "/net/hosts/add", access);
	procfuse_registerNode(&pf, "/net/hosts/del", access);

	procfuse_run(&pf, PROCFUSE_BLOCK);

	unlink(absolutemountpoint);
	umount(absolutemountpoint);

	procfuse_dtor(&pf);
}
