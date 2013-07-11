/*
 * procfuse.h
 *
 */

#ifndef PROCFUSE_H_
#define PROCFUSE_H_

#include "hash-table.h"

#include <pthread.h>
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif
#include <fuse.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROCFUSE_VERSION "0.0.1"
#define PROCFUSE_BLOCK 1
#define PROCFUSE_NONBLOCK 1

#define PROCFUSE_PRE 1
#define PROCFUSE_POST 0

#define PROCFUSE_YES 1
#define PROCFUSE_NO 0

struct procfuse;

typedef int (*procfuse_onFuseOpen)(const struct procfuse *pf, const char *path, int tid, const void* appdata);
typedef int (*procfuse_onFuseTruncate)(const struct procfuse *pf, const char *path, const void* appdata);
typedef int (*procfuse_onFuseRead)(const struct procfuse *pf, const char *path, char *buffer, size_t size, off_t offset, int tid, const void* appdata);
typedef int (*procfuse_onFuseWrite)(const struct procfuse *pf, const char *path, const char *buffer, size_t size, off_t offset, int tid, const void* appdata);
typedef int (*procfuse_onFuseRelease)(const struct procfuse *pf, const char *path, int tid, const void* appdata);

typedef int (*procfuse_touch)(const struct procfuse *pf, const char *path, int tid, int flags, int pre_or_post, const void* appdata);



struct procfuse_accessor{
	procfuse_onFuseOpen onFuseOpen;

    procfuse_onFuseTruncate onFuseTruncate;
    procfuse_onFuseRead onFuseRead;
    procfuse_onFuseWrite onFuseWrite;

	procfuse_onFuseRelease onFuseRelease;
};

struct procfuse_pod_accessor{
	procfuse_touch touch;

	union{
		char *c;
		int *i;
		int64_t *l;
		float *f;
		double *d;
		long double *ld;
		struct{
			char **buffer;
			int *length;
		} str;
	}types;
	int type;
	int flags;
};


struct procfuse* procfuse_ctor(const char *filesystemname, const char *mountpoint, const char *fuse_option, const void *appdata);
void procfuse_dtor(struct procfuse *pf);

struct procfuse_pod_accessor procfuse_podaccessorChar(char *c, int flags, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorInt(int *i, int flags, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorInt64(int64_t *l, int flags, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorFloat(float *f, int flags, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorDouble(double *d, int flags, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorLongDouble(long double *ld, int flags, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorString(char **buffer, int *length, int flags, procfuse_touch touch);
struct procfuse_accessor procfuse_accessor(procfuse_onFuseOpen onFuseOpen, procfuse_onFuseTruncate onFuseTruncate,
                                           procfuse_onFuseRead onFuseRead, procfuse_onFuseWrite onFuseWrite,
                                           procfuse_onFuseRelease onFuseRelease);

int procfuse_registerNode(struct procfuse *pf, const char *absolutepath, struct procfuse_accessor access);
int procfuse_registerNodePOD(struct procfuse *pf, const char *absolutepath, struct procfuse_pod_accessor podaccess);

void procfuse_run(struct procfuse *pf, int blocking);
void procfuse_caller(uid_t *u, gid_t *g, pid_t *p, mode_t *mask);
int procfuse_setSingleThreaded(struct procfuse *pf, int yes_or_no);
void procfuse_teardown(struct procfuse *pf);

#ifdef __cplusplus
}
#endif

#endif /* PROCFUSE_H_ */
