/*  
    ProcFuse is a C library which can be used to register string paths
    representing a file of your own filesystem like /proc used by *nix
    Copyright (C) 2015 - vrcif0@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef PROCFUSE_H_
#define PROCFUSE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <utime.h>

#define PROCFUSE_VERSION "0.0.1"
#define PROCFUSE_BLOCK 1
#define PROCFUSE_NONBLOCK 0

#define PROCFUSE_PRE 1
#define PROCFUSE_POST 0

#define PROCFUSE_YES 1
#define PROCFUSE_NO 0

struct procfuse_error{
	int errn;
	char *error;
	char *solution;
};

struct procfuse;

typedef int (*procfuse_onFuseOpen)(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata);
typedef int (*procfuse_onFuseTruncate)(const struct procfuse *pf, const char *path, const off_t off, const void* appdata);
typedef int (*procfuse_onFuseRead)(const struct procfuse *pf, const char *path, char *buffer, size_t size, off_t offset, int64_t tid, const void* appdata);
typedef int (*procfuse_onFuseWrite)(const struct procfuse *pf, const char *path, const char *buffer, size_t size, off_t offset, int64_t tid, const void* appdata);
typedef int (*procfuse_onFuseRelease)(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata);

typedef int (*procfuse_onModify_c)(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata, char newvalue);
typedef int (*procfuse_onModify_i)(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata, int newvalue);
typedef int (*procfuse_onModify_i64)(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata, int64_t newvalue);
typedef int (*procfuse_onModify_f)(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata, float newvalue);
typedef int (*procfuse_onModify_d)(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata, double newvalue);
typedef int (*procfuse_onModify_ld)(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata, long double newvalue);
typedef int (*procfuse_onModify_s)(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata, const char *newvalue, int64_t length);

struct procfuse_accessor{
	procfuse_onFuseOpen onFuseOpen;

    procfuse_onFuseTruncate onFuseTruncate;
    procfuse_onFuseRead onFuseRead;
    procfuse_onFuseWrite onFuseWrite;

	procfuse_onFuseRelease onFuseRelease;
};

struct procfuse* procfuse_ctor(const char *filesystemname, const char *mountpoint, const char *fuse_option, const void *appdata);
void procfuse_dtor(struct procfuse *pf);

struct procfuse_accessor procfuse_accessor(procfuse_onFuseOpen onFuseOpen, procfuse_onFuseTruncate onFuseTruncate,
                                           procfuse_onFuseRead onFuseRead, procfuse_onFuseWrite onFuseWrite,
                                           procfuse_onFuseRelease onFuseRelease);
int procfuse_create(struct procfuse *pf, const char *absolutepath, struct procfuse_accessor access);

int procfuse_createPOD_c(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_c onModify);
int procfuse_createPOD_i(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_i onModify);
int procfuse_createPOD_i64(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_i64 onModify);
int procfuse_createPOD_f(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_f onModify);
int procfuse_createPOD_d(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_d onModify);
int procfuse_createPOD_ld(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_ld onModify);
int procfuse_createPOD_s(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_s onModify);

int procfuse_readPOD_c(struct procfuse *pf, const char *absolutepath, char *value);
int procfuse_readPOD_i(struct procfuse *pf, const char *absolutepath, int *value);
int procfuse_readPOD_i64(struct procfuse *pf, const char *absolutepath, int64_t *value);
int procfuse_readPOD_f(struct procfuse *pf, const char *absolutepath, float *value);
int procfuse_readPOD_d(struct procfuse *pf, const char *absolutepath, double *value);
int procfuse_readPOD_ld(struct procfuse *pf, const char *absolutepath, long double *value);
int procfuse_readPOD_s(struct procfuse *pf, const char *absolutepath, char *value, int64_t *length);

int procfuse_writePOD_c(struct procfuse *pf, const char *absolutepath, char value);
int procfuse_writePOD_i(struct procfuse *pf, const char *absolutepath, int value);
int procfuse_writePOD_i64(struct procfuse *pf, const char *absolutepath, int64_t value);
int procfuse_writePOD_f(struct procfuse *pf, const char *absolutepath, float value);
int procfuse_writePOD_d(struct procfuse *pf, const char *absolutepath, double value);
int procfuse_writePOD_ld(struct procfuse *pf, const char *absolutepath, long double value);
int procfuse_writePOD_s(struct procfuse *pf, const char *absolutepath, char *value, int64_t length);

int procfuse_isPOD(struct procfuse *pf, const char *absolutepath);
int procfuse_exists(struct procfuse *pf, const char *absolutepath);
int procfuse_chmod(struct procfuse *pf, const char *absolutepath, mode_t mode);
int procfuse_chown(struct procfuse *pf, const char *absolutepath, uid_t owner, gid_t group);
int procfuse_utime(struct procfuse *pf, const char *absolutepath, const struct utimbuf *times);
int procfuse_utimes(struct procfuse *pf, const char *absolutepath, const struct timeval times[2]);

struct procfuse_error* procfuse_error(struct procfuse *pf);

int procfuse_unlink(struct procfuse *pf, const char *absolutepath);


void procfuse_run(struct procfuse *pf, int blocking);
void procfuse_caller(uid_t *u, gid_t *g, pid_t *p, mode_t *mask);
int procfuse_setSingleThreaded(struct procfuse *pf, int yes_or_no);
void procfuse_teardown(struct procfuse *pf);

#ifdef __cplusplus
}
#endif

#endif /* PROCFUSE_H_ */
