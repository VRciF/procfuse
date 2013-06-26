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
#define FUSE_USE_VERSION 26
#include <fuse.h>

#define PROCFUSE_BLOCK 1
#define PROCFUSE_NONBLOCK 1

struct procfuse{
	HashTable *root;
	pthread_mutex_t lock;

	int tidcounter;

	pthread_t procfuseth;
	struct fuse *fuse;

	int fuseArgc;
	const char *fuseArgv[9];
	char *fuse_option;
	int fuse_singlethreaded;

	char *absolutemountpoint;
	struct fuse_operations procFS_oper;
};

typedef int (*procfuse_onFuseOpen)(const char *path, int tid);
typedef int (*procfuse_onFuseTruncate)(const char *path);
typedef int (*procfuse_onFuseRead)(const char *path, char *buffer, size_t size, off_t offset, int tid);
typedef int (*procfuse_onFuseWrite)(const char *path, const char *buffer, size_t size, off_t offset, int tid);
typedef int (*procfuse_onFuseRelease)(const char *path, int tid);

struct procfuse_accessor{
	procfuse_onFuseOpen onFuseOpen;
	procfuse_onFuseTruncate onFuseTruncate;
	procfuse_onFuseRead onFuseRead;
	procfuse_onFuseWrite onFuseWrite;
	procfuse_onFuseRelease onFuseRelease;
};


struct procfuse_hashnode{
	union{
		HashTable *root;
		struct procfuse_accessor onevent;
	};
	char *key;
	int eon; /* end of node */
};

int procfuse_ctor(struct procfuse *pf, const char *filesystemname, const char *absolutemountpoint, const char *fuse_option);
void procfuse_dtor(struct procfuse *pf);
int procfuse_registerNode(struct procfuse *pf, const char *absolutepath, struct procfuse_accessor access);
int procfuse_unregisterNode(struct procfuse *pf, const char *absolutepath);
void procfuse_run(struct procfuse *pf, int blocking);
void procfuse_teardown(struct procfuse *pf);

#endif /* PROCFUSE_H_ */
