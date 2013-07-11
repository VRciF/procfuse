#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

#include "procfuse-amalgamation.h"

struct procfuse *pf=NULL;
struct data{
    int port;
    float speed;
    char logfile[8192];

    char readbuffer[8192];
    int rlength;
    char writebuffer[4096];
    int wlength;

    char *lfbuffer;
    int lflength;
};

int onTouch(const struct procfuse *pf, const char *path, int tid, int flags, int pre_or_post, const void *appdata){
	struct data *app = (struct data *)appdata;
	(void)(pf);
	(void)(tid);

	if(pre_or_post==PROCFUSE_PRE){

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

	    printf("current stats: port[%d], speed[%g], logfile[%s]\n", app->port, app->speed, app->logfile);
	}

	return 0;
}

void sig_handler(int s)
{
	(void)(s);
	procfuse_teardown(pf);
}

int main(int argc,char **argv){
	struct data app = {80, 123.45, {'\0'}, {'\0'}, 8191, {'\0'}, 4095, NULL, 0};

	(void)(argc);
    const char *mountpoint = argv[1];

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    pf = procfuse_ctor(argv[0], mountpoint, "allow_other,big_writes", &app);

	procfuse_registerNodePOD(pf, "/port", procfuse_podaccessorInt(&app.port, O_RDWR, NULL));
	procfuse_registerNodePOD(pf, "/speed", procfuse_podaccessorFloat(&app.speed, O_RDONLY, onTouch)); /* read only file */
	procfuse_registerNodePOD(pf, "/log/file", procfuse_podaccessorString(&app.lfbuffer, &app.lflength, O_WRONLY, onTouch)); /* write only file */

	procfuse_setSingleThreaded(pf, PROCFUSE_YES); /* disable concurrent calls to onTouch - so no need for mutex locks */

    if(pf!=NULL) procfuse_run(pf, PROCFUSE_BLOCK);
    umount(mountpoint);
    if(pf!=NULL) procfuse_dtor(pf);
    return 0;
}

