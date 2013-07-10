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
struct procfuse *pf;
int port=80;
float speed=123.45;
std::string logfile;

char readbuffer[8192]={'\0'};
int rlength=sizeof(readbuffer)-1;
char writebuffer[4096]={'\0'};
int wlength=sizeof(writebuffer)-1;

char *lfbuffer;
int lflength;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int onTouch(const struct procfuse *, const char *path, int , int flags, bool pre_or_post){
	if(pre_or_post==PROCFUSE_PRE){
	    pthread_mutex_lock(&lock);
	    printf("locking\n");

	    if(std::string(path).compare("/log/file")==0){
	    	if(flags==O_RDONLY){
	    	    lfbuffer = readbuffer;
	    	    lflength = rlength;
	    	}
	    	else{
	    	    lfbuffer = readbuffer;
	    	    lflength = wlength;
	    	}
	    }
	}
	else{
		pthread_mutex_unlock(&lock);
		printf("unlocking\n");
	}

	return 0;
}

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
	const char *mountpoint = "procfuse.test";

	const char *some = "data";

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

	mkdir(mountpoint, 0777);

	pf = procfuse_ctor(argv[0], mountpoint, "allow_other,big_writes", some);

	procfuse_registerNodePOD(pf, "/port", procfuse_podaccessorInt(&port, O_RDWR, NULL));
	procfuse_registerNodePOD(pf, "/speed", procfuse_podaccessorFloat(&speed, O_RDONLY, onTouch));
	procfuse_registerNodePOD(pf, "/log/file", procfuse_podaccessorString(&lfbuffer, &lflength, O_RDWR, onTouch));

	procfuse_registerNode(pf, "/net/hosts/list",
			              procfuse_accessor(NULL, NULL, onFuseRead, NULL, NULL)); // read only
	procfuse_registerNode(pf, "/net/hosts/add", procfuse_accessor(NULL, NULL, NULL, onFuseWrite, NULL)); // write only
	procfuse_registerNode(pf, "/net/hosts/del", procfuse_accessor(NULL, NULL, NULL, onFuseWrite, NULL));

	procfuse_run(pf, PROCFUSE_BLOCK);

	unlink(mountpoint);
	umount(mountpoint);

	procfuse_dtor(pf);
}
