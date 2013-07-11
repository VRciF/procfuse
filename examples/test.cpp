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

struct procfuse *pf;

struct data{
    std::string hosts;
    int port;
    float speed;
    char logfile[8192];

    char readbuffer[8192];
    int rlength;
    char writebuffer[4096];
    int wlength;

    char *lfbuffer;
    int lflength;

    pthread_mutex_t lock;
};

int onTouch(const struct procfuse *pf, const char *path, int tid, int flags, int pre_or_post, const void* appdata){
	struct data *app = (struct data*)appdata;
	(void)(pf);
	(void)(tid);

	if(pre_or_post==PROCFUSE_PRE){
	    pthread_mutex_lock(&app->lock);
	    printf("locking\n");

	    if(strcmp(path,"/log/file")==0){
	    	if(flags==O_RDONLY){
	    		/* switch to readbuffer */
	    		app->lfbuffer = app->readbuffer;
	    		app->lflength = app->rlength;

	    	    /* clear readbuffer */
	    	    memset(app->readbuffer, '\0', sizeof(app->readbuffer));
	    	    /* copy current logfile to readbuffer */
	    	    strncpy(app->readbuffer, app->logfile, sizeof(app->readbuffer));
	    	}
	    	else{
	    		app->lfbuffer = app->writebuffer;
	    		app->lflength = app->wlength;
	    	}
	    }
	}
	else{
	    if(strcmp(path,"/log/file")==0){
	    	if(flags==O_WRONLY){
	    		/* copy from writebuffer to logfile */
	    		strncpy(app->logfile, app->writebuffer, sizeof(app->logfile));
	    	}
		}

		pthread_mutex_unlock(&app->lock);
		printf("unlocking\n");

	    printf("current stats: port[%d], speed[%g], logfile[%s]\n", app->port, app->speed, app->logfile);
	}

	return 0;
}

int onFuseRead(const struct procfuse *, const char *path, char *buffer, size_t size, off_t offset, int, const void *appdata){
	struct data *app = (struct data*)appdata;
	int wlen = 0;
	uid_t u;
	gid_t g;
	pid_t p;
	mode_t mask;

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
int onFuseWrite(const struct procfuse *, const char *path, const char *buffer, size_t size, off_t, int, const void *appdata){
	struct data *app = (struct data*)appdata;
	int rval = 0;

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
	app.rlength = sizeof(app.readbuffer)-1;
	app.wlength = sizeof(app.writebuffer)-1;
	pthread_mutex_init(&app.lock, NULL);

	const char *mountpoint = "procfuse.test";

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

	mkdir(mountpoint, 0777);

	pf = procfuse_ctor(argv[0], mountpoint, "allow_other,big_writes", &app);

	procfuse_registerNodePOD(pf, "/port", procfuse_podaccessorInt(&app.port, O_RDWR, NULL));
	procfuse_registerNodePOD(pf, "/speed", procfuse_podaccessorFloat(&app.speed, O_RDONLY, onTouch));
	procfuse_registerNodePOD(pf, "/log/file", procfuse_podaccessorString(&app.lfbuffer, &app.lflength, O_RDWR, onTouch));

	procfuse_registerNode(pf, "/net/hosts/list",
			              procfuse_accessor(NULL, NULL, onFuseRead, NULL, NULL)); // read only
	procfuse_registerNode(pf, "/net/hosts/add", procfuse_accessor(NULL, NULL, NULL, onFuseWrite, NULL)); // write only
	procfuse_registerNode(pf, "/net/hosts/del", procfuse_accessor(NULL, NULL, NULL, onFuseWrite, NULL));

	procfuse_run(pf, PROCFUSE_BLOCK);

	unlink(mountpoint);
	umount(mountpoint);

	procfuse_dtor(pf);
}
