/*
 * test.cpp
 *
 */

//#include "procfuse-amalgamation.h"
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

#include <sys/stat.h>
#include <fcntl.h>

struct procfuse *pf;

struct data{
    std::string hosts;
    int port;
    float speed;
    char logfile[8192];
};

int onFuseRead(const struct procfuse *pf, const char *path, char *buffer, size_t size, off_t offset, int64_t tid, const void* appdata){
	struct data *app = (struct data*)appdata;
	int wlen = 0;
	uid_t u;
	gid_t g;
	pid_t p;
	mode_t mask;

	(void)pf;
	(void)tid;

	procfuse_caller(&u,&g,&p,&mask);

	printf("Appdata: %s\n", (const char*)appdata);
	printf("the process %d running as uid|gid|mask=%d|%d|%d caused this request\n", p, u, g, mask);

	if(std::string(path)=="/net/hosts/list"){
		if(offset<(off_t)app->hosts.length()){
			size_t cpylen = app->hosts.length()-offset;

			if(cpylen>size) cpylen = size;

			memcpy(buffer, app->hosts.data()+offset, cpylen);

			wlen = cpylen;
		}
	}

	return wlen;
}
int onFuseWrite(const struct procfuse *pf, const char *path, const char *buffer, size_t size, off_t offset, int64_t tid, const void* appdata){
	struct data *app = (struct data*)appdata;
	int rval = 0;

	(void)pf;
	(void)offset;
	(void)tid;

	std::string s(buffer, size);
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	s.append(";");
	std::size_t pos = app->hosts.find(s);
	if(std::string(path)=="/net/hosts/add"){
		if(pos==std::string::npos){
			app->hosts.append(s);
		}
	}
	if(std::string(path)=="/net/hosts/del"){
		if(pos!=std::string::npos){
			app->hosts.replace(pos, s.length(),std::string());
		}

	}

	rval = size;

	return rval;
}

void sig_handler(int)
{
	procfuse_teardown(pf);
}

int main(int,char **){
	struct data app;
	app.port = 80;
	app.speed = 123.45;

	char executablepath[8192] = {'\0'};

	std::string mountpoint;

	if(readlink("/proc/self/exe", executablepath, sizeof(executablepath)-1)==-1){
		std::cerr << "couldn't read link /proc/self/exe" << std::endl;
		return -1;
	}
	if(strrchr(executablepath, '/')!=NULL){
		*strrchr(executablepath, '/')='\0';
	}
	mountpoint.append(executablepath).append("/procfuse.test");
	std::cout << mountpoint << std::endl;

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

	mkdir(mountpoint.c_str(), 0777);

	pf = procfuse_ctor("procfs.test", mountpoint.c_str(), "allow_other,big_writes", &app);

	procfuse_createPOD_i(pf, "/port", O_RDWR, NULL);
	procfuse_createPOD_f(pf, "/speed", O_RDONLY, NULL);

	procfuse_create(pf, "/net/hosts/list", procfuse_accessor(NULL, NULL, onFuseRead, NULL, NULL)); // read only
	procfuse_create(pf, "/net/hosts/add", procfuse_accessor(NULL, NULL, NULL, onFuseWrite, NULL)); // write only
	procfuse_create(pf, "/net/hosts/del", procfuse_accessor(NULL, NULL, NULL, onFuseWrite, NULL)); // write only

	procfuse_run(pf, PROCFUSE_BLOCK);

	unlink(mountpoint.c_str());
	umount(mountpoint.c_str());

	procfuse_dtor(pf);
}
