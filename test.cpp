/*
 * test.cpp
 *
 */

#include "procfuse-amalgamation.h"

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
struct procfuse *pf;
int port=80;

int onFuseRead(const struct procfuse *, const char *path, char *buffer, size_t size, off_t offset, int, const void *appdata){
	int wlen = 0;

	printf("Appdata: %s\n", (const char*)appdata);

	if(std::string(path)=="/net/hosts/list"){
		if(offset<(off_t)hosts.length()){
			size_t cpylen = hosts.length()-offset;

			if(cpylen>size) cpylen = size;

			memcpy(buffer, hosts.data()+offset, cpylen);

			wlen = cpylen;
		}
	}

	return wlen;
}
int onFuseWrite(const struct procfuse *, const char *path, const char *buffer, size_t size, off_t, int, const void *){
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
	procfuse_teardown(pf);
}

int main(int,char **argv){
	const char *some = "data";

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

	mkdir(absolutemountpoint, 0777);

	pf = procfuse_ctor(argv[0], absolutemountpoint, NULL, some);

	procfuse_registerNodePOD(pf, "/port", procfuse_podaccessorInt(&port, NULL));
	procfuse_registerNode(pf, "/net/hosts/list",
			              procfuse_podaccessor(NULL, NULL, onFuseRead, NULL, NULL)); // read only
	procfuse_registerNode(pf, "/net/hosts/add", procfuse_podaccessor(NULL, NULL, NULL, onFuseWrite, NULL)); // write only
	procfuse_registerNode(pf, "/net/hosts/del", procfuse_podaccessor(NULL, NULL, NULL, onFuseWrite, NULL));

	procfuse_run(pf, PROCFUSE_BLOCK);

	unlink(absolutemountpoint);
	umount(absolutemountpoint);

	procfuse_dtor(pf);
}
