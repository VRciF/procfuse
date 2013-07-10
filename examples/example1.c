#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include "procfuse-amalgamation.h"

struct procfuse *pf=NULL;

void sig_handler(int s)
{
	(void)(s);
	procfuse_teardown(pf);
}

int main(int argc, char **argv){
	(void)(argc);
    const char *mountpoint = argv[1];

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    pf = procfuse_ctor(argv[0], mountpoint, "allow_other,big_writes", NULL);
    if(pf!=NULL) procfuse_run(pf, PROCFUSE_BLOCK);
    umount(mountpoint);
    if(pf!=NULL) procfuse_dtor(pf);
    return 0;
}
