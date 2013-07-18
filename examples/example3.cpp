/*
 * example3.cpp
 *
 */

#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <pthread.h>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>

#include "procfuse-amalgamation.h"

std::string hosts;
struct procfuse *pf;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int onFuseRead(const struct procfuse *, const char *path, char *buffer, size_t size, off_t offset, int, const void *appdata){
	int wlen = 0;
	uid_t u;
	gid_t g;
	pid_t p;
	mode_t mask;

	procfuse_caller(&u,&g,&p,&mask);

	printf("Appdata: %s\n", (const char*)appdata);
	printf("the process %d running as uid|gid|mask=%d|%d|%d caused this request\n", p, u, g, mask);

	if(std::string(path)=="/net/hosts/list"){
		pthread_mutex_lock(&lock);
		if(offset<(off_t)hosts.length()){
			size_t cpylen = hosts.length()-offset;

			if(cpylen>size) cpylen = size;

			memcpy(buffer, hosts.data()+offset, cpylen);

			wlen = cpylen;
		}
		pthread_mutex_unlock(&lock);
	}

	return wlen;
}
int onFuseWrite(const struct procfuse *, const char *path, const char *buffer, size_t size, off_t, int, const void *){
	int rval = 0;

	std::string s(buffer, size);
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	s.append(";");

	pthread_mutex_lock(&lock);

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

	pthread_mutex_unlock(&lock);

	rval = size;

	return rval;
}

void sig_handler(int)
{
	procfuse_teardown(pf);
}

int main(int,char **argv){
	const char *mountpoint = argv[1];

	const char *some = "data";

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

	pf = procfuse_ctor(argv[0], mountpoint, "allow_other,big_writes", some);

	procfuse_registerNode(pf, "/net/hosts/list",
			              procfuse_accessor(NULL, NULL, onFuseRead, NULL, NULL)); // read only
	procfuse_registerNode(pf, "/net/hosts/add", procfuse_accessor(NULL, NULL, NULL, onFuseWrite, NULL)); // write only
	procfuse_registerNode(pf, "/net/hosts/del", procfuse_accessor(NULL, NULL, NULL, onFuseWrite, NULL)); // write only

	procfuse_run(pf, PROCFUSE_BLOCK);

	procfuse_dtor(pf);
}
