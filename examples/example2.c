#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

#include "procfuse-amalgamation.h"

struct procfuse *pf=NULL;
int port=80;
float speed=123.45;

char logfile[8192] = {'\0'};

char readbuffer[8192]={'\0'};
int rlength=sizeof(readbuffer)-1;
char writebuffer[4096]={'\0'};
int wlength=sizeof(writebuffer)-1;

char *lfbuffer=NULL;
int lflength=0;

int onTouch(const struct procfuse *pf, const char *path, int tid, int flags, int pre_or_post){
	(void)(pf);
	(void)(tid);

	if(pre_or_post==PROCFUSE_PRE){

	    if(strcmp(path,"/log/file")==0){
	    	if(flags==O_RDONLY){
	    		/* switch to readbuffer */
	    	    lfbuffer = readbuffer;
	    	    lflength = rlength;

	    	    /* clear readbuffer */
	    	    memset(readbuffer, '\0', sizeof(readbuffer));
	    	    /* copy current logfile to readbuffer */
	    	    strncpy(readbuffer, logfile, sizeof(readbuffer));
	    	}
	    	else{
	    	    lfbuffer = writebuffer;
	    	    lflength = wlength;
	    	}
	    }
	}
	else{
	    if(strcmp(path,"/log/file")==0){
	    	if(flags==O_WRONLY){
	    		/* copy from writebuffer to logfile */
	    		strncpy(logfile, writebuffer, sizeof(logfile));
	    	}
		}

	    printf("current stats: port[%d], speed[%g], logfile[%s]\n", port, speed, logfile);
	}

	return 0;
}

void sig_handler(int s)
{
	(void)(s);
	procfuse_teardown(pf);
}

int main(int argc,char **argv){
	(void)(argc);
    const char *mountpoint = argv[1];

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    pf = procfuse_ctor(argv[0], mountpoint, "allow_other,big_writes", NULL);

	procfuse_registerNodePOD(pf, "/port", procfuse_podaccessorInt(&port, O_RDWR, NULL));
	procfuse_registerNodePOD(pf, "/speed", procfuse_podaccessorFloat(&speed, O_RDONLY, onTouch)); /* read only file */
	procfuse_registerNodePOD(pf, "/log/file", procfuse_podaccessorString(&lfbuffer, &lflength, O_WRONLY, onTouch)); /* write only file */

	procfuse_setSingleThreaded(pf, PROCFUSE_YES); /* disable concurrent calls to onTouch - so no need for mutex locks */

    if(pf!=NULL) procfuse_run(pf, PROCFUSE_BLOCK);
    umount(mountpoint);
    if(pf!=NULL) procfuse_dtor(pf);
    return 0;
}

