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

#include <sys/stat.h>
#include <fcntl.h>

struct procfuse *pf;

struct data{
    std::string hosts;
    int port;
    float speed;
    char logfile[8192];

    pthread_mutex_t lock;
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

int main(int,char **argv){
	struct data app;
	memset(&app, '\0', sizeof(app));
	app.port = 80;
	app.speed = 123.45;
	pthread_mutex_init(&app.lock, NULL);

	const char *mountpoint = "procfuse.test";

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

	mkdir(mountpoint, 0777);

	pf = procfuse_ctor(argv[0], mountpoint, "allow_other,big_writes", &app);

	procfuse_createPOD_i(pf, "/port", O_RDWR, NULL);
	procfuse_createPOD_f(pf, "/speed", O_RDONLY, NULL);

	procfuse_create(pf, "/net/hosts/list", O_RDONLY,
			              procfuse_accessor(NULL, NULL, onFuseRead, NULL, NULL)); // read only
	procfuse_create(pf, "/net/hosts/add", O_WRONLY, procfuse_accessor(NULL, NULL, NULL, onFuseWrite, NULL)); // write only
	procfuse_create(pf, "/net/hosts/del", O_WRONLY, procfuse_accessor(NULL, NULL, NULL, onFuseWrite, NULL));

	procfuse_run(pf, PROCFUSE_BLOCK);

	unlink(mountpoint);
	umount(mountpoint);

	procfuse_dtor(pf);
}
