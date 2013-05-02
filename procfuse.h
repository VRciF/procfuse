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

#ifndef PROCFUSE_DELIM
#define PROCFUSE_DELIMC '/'
#define PROCFUSE_DELIMS "/"
#endif

#define PROCFUSE_BLOCK 1
#define PROCFUSE_NONBLOCK 1

struct procfuse{
	HashTable *root;
	pthread_mutex_t lock;

	pthread_t procfuseth;
	int fuse_singlethreaded;
	char *option;

	int fuseArgc;
	const char *fuseArgv[9];
	struct fuse_operations procFS_oper;
	struct fuse *fuse;
};

struct procfuse_accessor{
	int (*onFuseRead)(const char *path, char *buffer, size_t size, off_t offset);
	int (*onFuseWrite)(const char *path, const char *buffer, size_t size, off_t offset);
};


struct procfuse_hashnode{
	union{
		struct procfuse subdir;
		struct procfuse_accessor onevent;
	};
	char *key;
	int eon; /* end of node */
};

int procfuse_ctor(struct procfuse *pf, const char *filesystemname, const char *absolutemountpoint, const char *fuse_option);
void procfuse_dtor(struct procfuse *pf);
int procfuse_registerNode(struct procfuse *pf, const char *absolutepath, struct procfuse_accessor access);
int procfuse_unregisterNode(struct procfuse *pf, const char *absolutepath);
void procfuse_main(struct procfuse *pf, int blocking);

#endif /* PROCFUSE_H_ */
