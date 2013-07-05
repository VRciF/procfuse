/*
 * procfuse.h
 *
 *  Created on: Apr 28, 2013
 *      Author: user
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

struct procfuse;

typedef int (*procfuse_onFuseOpen)(const struct procfuse *pf, const char *path, int tid, const void* appdata);
typedef int (*procfuse_onFuseTruncate)(const struct procfuse *pf, const char *path, const void* appdata);
typedef int (*procfuse_onFuseRead)(const struct procfuse *pf, const char *path, char *buffer, size_t size, off_t offset, int tid, const void* appdata);
typedef int (*procfuse_onFuseWrite)(const struct procfuse *pf, const char *path, const char *buffer, size_t size, off_t offset, int tid, const void* appdata);
typedef int (*procfuse_onFuseRelease)(const struct procfuse *pf, const char *path, int tid, const void* appdata);

typedef int (*procfuse_touch)(const struct procfuse *pf, const char *path, int tid, int flags, bool pre_or_post);

extern struct procfuse_pod_accessor;
extern struct procfuse_accessor;

struct procfuse* procfuse_ctor(const char *filesystemname, const char *absolutemountpoint, const char *fuse_option, const void *appdata);
void procfuse_dtor(struct procfuse *pf);

struct procfuse_pod_accessor procfuse_podaccessorChar(char *c, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorInt(int *i, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorInt64(int64_t *l, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorFloat(float *f, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorDouble(double *d, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorLongDouble(long double *ld, procfuse_touch touch);
struct procfuse_pod_accessor procfuse_podaccessorString(char **buffer, int *length, procfuse_touch touch);
struct procfuse_accessor procfuse_accessor(procfuse_onFuseOpen onFuseOpen, procfuse_onFuseTruncate onFuseTruncate,
                                           procfuse_onFuseRead onFuseRead, procfuse_onFuseWrite onFuseWrite,
                                           procfuse_onFuseRelease onFuseRelease);

int procfuse_registerNode(struct procfuse *pf, const char *absolutepath, struct procfuse_accessor access);
int procfuse_registerNodePOD(struct procfuse *pf, const char *absolutepath, struct procfuse_pod_accessor podaccess);

void procfuse_run(struct procfuse *pf, int blocking);
void procfuse_caller(uid_t *u, gid_t *g, pid_t *p, mode_t *mask);
void procfuse_teardown(struct procfuse *pf);

#ifdef __cplusplus
}
#endif

#endif /* PROCFUSE_H_ */
