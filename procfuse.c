/*
 * procfuse.c
 *
 */

#include "procfuse.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "hash-table.h"
#include "hash-string.h"
#include "hash-int.h"
#include "compare-string.h"
#include "compare-int.h"

#define PROCFUSE_DELIMC '/'
#define PROCFUSE_DELIMS "/"

#define PROCFUSE_FNAMELEN 512

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

	const void *appdata;
};

struct procfuse_transactionnode{
	char *writebuffer;
	int length;
	int haswritten;

	int tid; /* key */
};

struct procfuse_hashnode{

    HashTable *subdirs;
    HashTable *transactions;

	struct procfuse_pod_accessor onpodevent;
	struct procfuse_accessor onevent;

	char *key;
};
enum PROCFS_TYPE_POD{ T_PROC_POD_NO=0, T_PROC_POD_CHAR, T_PROC_POD_INT, T_PROC_POD_INT64,
	                  T_PROC_POD_FLOAT, T_PROC_POD_DOUBLE, T_PROC_POD_LONGDOUBLE,
	                  T_PROC_POD_STRING};

int procfuse_onFuseOpenPOD(const struct procfuse *pf, const char *path, int tid, const void* appdata);
int procfuse_onFuseReadPOD(const struct procfuse *pf, const char *path, char *buffer, size_t size, off_t offset, int tid, const void* appdata);
int procfuse_onFuseWritePOD(const struct procfuse *pf, const char *path, const char *buffer, size_t size, off_t offset, int tid, const void* appdata);
int procfuse_onFuseReleasePOD(const struct procfuse *pf, const char *path, int tid, const void* appdata);

void procfuse_freeHashNode(void *n){
	if(n!=NULL) return;
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)n;
	if(node->subdirs!=NULL){
		hash_table_free(node->subdirs);
	}
	if(node->transactions!=NULL){
		hash_table_free(node->transactions);
	}

	free(node);
}
void procfuse_freeTransactionNode(void *n){
	if(n!=NULL) return;
	struct procfuse_transactionnode *tnode = (struct procfuse_transactionnode *)n;

	if(tnode->writebuffer!=NULL)
	    free(tnode->writebuffer);

	free(tnode);
}
int procfuse_ctorht(HashTable **ht, int shall_str){
	if(ht==NULL){
		errno = EINVAL;
		return 0;
	}
	if(shall_str){
	    *ht = hash_table_new(string_hash, string_equal);
	}
	else{
		*ht = hash_table_new(int_hash, int_equal);
	}
	if(*ht==NULL){
		return 0;
	}
	if(shall_str)
        hash_table_register_free_functions(*ht, free, procfuse_freeHashNode);
	else
		hash_table_register_free_functions(*ht, NULL, procfuse_freeTransactionNode);
	return 1;
}

int procfuse_dtorht(HashTable **ht){
	if(ht==NULL){
		errno = EINVAL;
		return 0;
	}
	hash_table_free(*ht);
	return 1;
}

struct procfuse* procfuse_ctor(const char *filesystemname, const char *mountpoint, const char *fuse_option,const void *appdata){
	errno = 0;
	struct procfuse *pf = (struct procfuse *)calloc(1, sizeof(struct procfuse));
	char *absolutemountpoint = realpath(mountpoint, NULL);
	if(pf==NULL || filesystemname==NULL || mountpoint==NULL || absolutemountpoint==NULL){
		if(pf!=NULL) free(pf);
		if(absolutemountpoint!=NULL) free(absolutemountpoint);

		errno = EINVAL;
		return NULL;
	}
	memset(pf, '\0', sizeof(struct procfuse));
	pthread_mutex_init(&pf->lock, NULL);

	if(procfuse_ctorht(&pf->root, PROCFUSE_YES)==0){
		free(pf);
		free(absolutemountpoint);
		return NULL;
	}

    pf->fuseArgv[0] = strdup(filesystemname);
    pf->fuseArgv[1] = absolutemountpoint;
    pf->absolutemountpoint = absolutemountpoint;

	if(fuse_option!=NULL){
	    pf->fuse_option = strdup(fuse_option);
	}

	pf->fuse_singlethreaded = 0;

	DIR *d = NULL;
	if((d=opendir(pf->absolutemountpoint))==NULL){
		if(errno==ENOTCONN){

			int er = umount2(pf->absolutemountpoint, MNT_DETACH);
			if(er==-1){
				const char *fusermountcmd = "fusermount -u ";
				char *unmountcmd = (char*)calloc(sizeof(char),strlen(pf->absolutemountpoint)+strlen(fusermountcmd)+1);
				if(unmountcmd!=NULL){
				    sprintf(unmountcmd, "%s%s", fusermountcmd, pf->absolutemountpoint);
				    system(unmountcmd);
				    free(unmountcmd);
				}
			}
		}
	}
	else{
		closedir(d);
	}

	pf->appdata = appdata;

	return pf;
}

void procfuse_dtor(struct procfuse *pf){
	if(pf==NULL || pf->root==NULL){
		return;
	}

	free((void*)pf->fuseArgv[0]);
	free((void*)pf->absolutemountpoint);
	if(pf->fuse_option!=NULL){
	    free((void*)pf->fuse_option);
	}

	procfuse_dtorht(&pf->root);
	pf->appdata = NULL;

	memset(pf, '\0', sizeof(struct procfuse));
	free(pf);
}

void procfuse_printTree(HashTable *htable){
	HashTableIterator iterator;
	hash_table_iterate(htable, &iterator);
	struct procfuse_hashnode *value = NULL;
	while ((value = (struct procfuse_hashnode *)hash_table_iter_next(&iterator)) != HASH_TABLE_NULL) {
		printf("%s:%d:%s key=%p\n",__FILE__,__LINE__,__FUNCTION__, value->key);

		if(value->subdirs==NULL){
			printf("%s:%d:%s node = %s\n",__FILE__,__LINE__,__FUNCTION__, value->key);
		}
		else{
			printf("%s:%d:%s subdir = %s\n",__FILE__,__LINE__,__FUNCTION__, value->key);
			procfuse_printTree(value->subdirs);
		}
	}
}

/* return position of first non delim character at the beginning of what */
const char* procfuse_ltrim(const char *what, char delim){
	if(what==NULL){ return NULL; }
	while(*what==delim){
		what++;
	}
	return what;
}
/* return position of first non delim character at the end of what */
const char* procfuse_rtrim(const char *what, char delim){
	if(what==NULL){ return NULL; }

	const char *eow = what+strlen(what);
	do{
		eow--;
	}while(eow>what && *eow==delim);

	return eow;
}

int procfuse_getNextFileName(const char *trimedabsolutepath, char fname[PROCFUSE_FNAMELEN]){
	size_t len = 0;

	const char *eop = strchr(trimedabsolutepath,PROCFUSE_DELIMC);
	if(eop==NULL){
		len = strlen(trimedabsolutepath);
		eop = trimedabsolutepath+len;
	}
	else{
		len = (eop-trimedabsolutepath);
	}

	if(len>sizeof(fname)-1){
		errno = ENOMEM;
		return 0;
	}
	memcpy(fname, trimedabsolutepath, len);

	return 1;
}

struct procfuse_hashnode* procfuse_getNextNode(HashTable *root, char *fname, int create){
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)hash_table_lookup(root, fname);
	if(node==NULL && create==PROCFUSE_YES){
		node = (struct procfuse_hashnode *)calloc(sizeof(struct procfuse_hashnode), 1);
		if(node==NULL){
			errno = ENOMEM;
			return NULL;
		}
		node->key = strdup(fname);

		if(hash_table_insert(root, node->key, node)==0){
			free(node->key);
			free(node);

			node = NULL;
		}
	}

	return node;
}

struct procfuse_hashnode* procfuse_pathToNode(HashTable *root, const char *absolutepath, int create){
	char fname[PROCFUSE_FNAMELEN] = {'\0'};

	if(root==NULL || absolutepath==NULL){
		errno = EINVAL;
		return NULL;
	}

	absolutepath = procfuse_ltrim(absolutepath, PROCFUSE_DELIMC);
	const char *eoap = procfuse_rtrim(absolutepath, PROCFUSE_DELIMC);
	eoap++; /* reposition at delim character (or null byte) to have correct pointer arithmetic */

	if(!procfuse_getNextFileName(absolutepath, fname)){
		return NULL;
	}

	int pathlen = eoap-absolutepath;
	int flen = strlen(fname);
	int hassubpath = (pathlen==flen) ? PROCFUSE_NO : PROCFUSE_YES ;

	struct procfuse_hashnode *node = NULL;
	if(hassubpath){
		node = procfuse_getNextNode(root, fname, create);
		if(node->subdirs==NULL && create==PROCFUSE_YES &&
		   procfuse_ctorht(&node->subdirs, PROCFUSE_YES)==0){
			free(node->key);
			free(node);
			node = NULL;
		}
	}
	else{
		node = procfuse_getNextNode(root, fname, create);
	}

	if(node!=NULL){
	    if(hassubpath && node->subdirs!=NULL){
		    return procfuse_pathToNode(node->subdirs, absolutepath+flen+1, create);
	    }
	}
	/* in case of wrong node type is returned, it means an eexist error */
	errno = EEXIST;

	return node;
}

int procfuse_unregisterNodeInternal(HashTable *root, const char *absolutepath){
	char fname[PROCFUSE_FNAMELEN] = {'\0'};

	if(root==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	absolutepath = procfuse_ltrim(absolutepath, PROCFUSE_DELIMC);
	const char *eoap = procfuse_rtrim(absolutepath, PROCFUSE_DELIMC);
	eoap++; /* reposition at delim character (or null byte) to have correct pointer arithmetic */


	if(!procfuse_getNextFileName(absolutepath, fname)){
		return 0;
	}

	int pathlen = eoap-absolutepath;
	int flen = strlen(fname);
	int hassubpath = (pathlen==flen) ? 0 : 1 ;

	/* if
	 *    node exists AND is end node AND absolutepath is filename => update access
	 *    node exists AND is root node AND absolutepath is root => recursive registerNode
	 *    node not exists => alloc node as neeeded
	 */
	struct procfuse_hashnode *node = NULL;
	node = procfuse_getNextNode(root, fname, PROCFUSE_NO);
	if(node==NULL){
		errno = EEXIST;
		return 0;
	}

	if(hassubpath && node->subdirs!=NULL){
		int rval = procfuse_unregisterNodeInternal(node->subdirs, absolutepath+flen+1);
		if(hash_table_num_entries(node->subdirs)<=0){
			hash_table_remove(root, fname);
			node = NULL;
		}
		return rval;
	}
	else {
		hash_table_remove(root, fname);
		return 1;
	}
	return 0;
}

int procfuse_registerNode(struct procfuse *pf, const char *absolutepath, struct procfuse_accessor access){
	int rval = 0;
	struct procfuse_hashnode *node = NULL;

	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	pthread_mutex_lock(&pf->lock);

	node = procfuse_pathToNode(pf->root, absolutepath, PROCFUSE_YES);
	if(node!=NULL){
		node->onevent = access;
		node->onpodevent.type = T_PROC_POD_NO;
		rval = 1;
	}
	if(rval==0)
		procfuse_unregisterNodeInternal(pf->root, absolutepath); /* clean up unneeded tree structures */

	pthread_mutex_unlock(&pf->lock);

	return rval;
}
int procfuse_registerNodePOD(struct procfuse *pf, const char *absolutepath, struct procfuse_pod_accessor podaccess){
	int rval = 0;
	struct procfuse_hashnode *node = NULL;

	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	pthread_mutex_lock(&pf->lock);

	node = procfuse_pathToNode(pf->root, absolutepath, PROCFUSE_YES);
	if(node!=NULL){
		struct procfuse_accessor access = procfuse_accessor(procfuse_onFuseOpenPOD, NULL,
				                                            ((podaccess.flags & O_RDWR)==O_RDWR || (podaccess.flags & O_RDONLY)==O_RDONLY) ? procfuse_onFuseReadPOD : NULL,
			                                                ((podaccess.flags & O_RDWR)==O_RDWR || (podaccess.flags & O_WRONLY)==O_WRONLY) ? procfuse_onFuseWritePOD : NULL,
								                            procfuse_onFuseReleasePOD);
/*		memset(&access, '\0', sizeof(access));

		access.onFuseOpen = procfuse_onFuseOpenPOD;
		if((podaccess.flags & O_RDWR)==O_RDWR || (podaccess.flags & O_RDONLY)==O_RDONLY)
		    access.onFuseRead = procfuse_onFuseReadPOD;
		if((podaccess.flags & O_RDWR)==O_RDWR || (podaccess.flags & O_WRONLY)==O_WRONLY)
		    access.onFuseWrite = procfuse_onFuseWritePOD;
		access.onFuseRelease = procfuse_onFuseReleasePOD;
*/
		node->onevent = access;
		node->onpodevent = podaccess;

		if(node->onpodevent.type!=T_PROC_POD_CHAR && node->onpodevent.type!=T_PROC_POD_STRING &&
		   procfuse_ctorht(&node->transactions, PROCFUSE_NO)==0){
		    rval = 0;
		}
		else{
		    rval = 1;
		}
	}
	if(rval==0)
		procfuse_unregisterNodeInternal(pf->root, absolutepath); /* clean up unneeded tree structures */

	pthread_mutex_unlock(&pf->lock);

	return rval;
}
int procfuse_unregisterNode(struct procfuse *pf, const char *absolutepath){
	int rval = 0;

	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	pthread_mutex_lock(&pf->lock);

	rval = procfuse_unregisterNodeInternal(pf->root, absolutepath);

	pthread_mutex_unlock(&pf->lock);

	return rval;
}


struct procfuse_pod_accessor procfuse_podaccessorChar(char *c, int flags, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	memset(&podaccess, '\0', sizeof(podaccess));

	podaccess.touch = touch;
	podaccess.types.c = c;
	podaccess.type = T_PROC_POD_CHAR;
	podaccess.flags = flags;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorInt(int *i, int flags, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	memset(&podaccess, '\0', sizeof(podaccess));

	podaccess.touch = touch;
	podaccess.types.i = i;
	podaccess.type = T_PROC_POD_INT;
	podaccess.flags = flags;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorInt64(int64_t *l, int flags, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	memset(&podaccess, '\0', sizeof(podaccess));

	podaccess.touch = touch;
	podaccess.types.l = l;
	podaccess.type = T_PROC_POD_INT64;
	podaccess.flags = flags;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorFloat(float *f, int flags, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	memset(&podaccess, '\0', sizeof(podaccess));

	podaccess.touch = touch;
	podaccess.types.f = f;
	podaccess.type = T_PROC_POD_FLOAT;
	podaccess.flags = flags;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorDouble(double *d, int flags, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	memset(&podaccess, '\0', sizeof(podaccess));

	podaccess.touch = touch;
	podaccess.types.d = d;
	podaccess.type = T_PROC_POD_DOUBLE;
	podaccess.flags = flags;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorLongDouble(long double *ld, int flags, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	memset(&podaccess, '\0', sizeof(podaccess));

	podaccess.touch = touch;
	podaccess.types.ld = ld;
	podaccess.type = T_PROC_POD_LONGDOUBLE;
	podaccess.flags = flags;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorString(char **buffer, int *length, int flags, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	memset(&podaccess, '\0', sizeof(podaccess));

	podaccess.touch = touch;
	podaccess.types.str.buffer = buffer;
	podaccess.types.str.length = length;
	podaccess.type = T_PROC_POD_STRING;
	podaccess.flags = flags;

	return podaccess;
}
struct procfuse_accessor procfuse_accessor(procfuse_onFuseOpen onFuseOpen, procfuse_onFuseTruncate onFuseTruncate,
                                           procfuse_onFuseRead onFuseRead, procfuse_onFuseWrite onFuseWrite,
                                           procfuse_onFuseRelease onFuseRelease){
	struct procfuse_accessor access;
	memset(&access, '\0', sizeof(access));

	access.onFuseOpen = onFuseOpen;
	access.onFuseTruncate = onFuseTruncate;
	access.onFuseRead = onFuseRead;
	access.onFuseWrite = onFuseWrite;
	access.onFuseRelease = onFuseRelease;

	return access;
}

int procfuse_onFuseOpenPOD(const struct procfuse *pf, const char *path, int tid, const void* appdata){
	int rval = 0;
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	(void)(pf);
	(void)(path);

	/* transactions not needed for read only files */
	if((node->onpodevent.flags & O_RDWR) == O_RDONLY ||
	   node->onpodevent.type==T_PROC_POD_STRING || node->onpodevent.type==T_PROC_POD_CHAR)
		return rval;

	struct procfuse_transactionnode *tnode = (struct procfuse_transactionnode *)calloc(sizeof(struct procfuse_transactionnode), 1);
	if(tnode==NULL){
		return -EIO;
	}
	tnode->tid = tid;
	tnode->length = 8192;
	tnode->writebuffer = (char*)calloc(sizeof(char), tnode->length);
	if(tnode->writebuffer==NULL){
		free(tnode);
		return -ENOMEM;
	}
	if(hash_table_insert(node->transactions, &tnode->tid, tnode)==0){
		free(tnode->writebuffer);
		free(tnode);

		tnode = NULL;
		return -ENOMEM;
	}

    return rval;
}
int procfuse_onFuseReadPOD(const struct procfuse *pf, const char *path, char *buffer, size_t size, off_t offset, int tid, const void* appdata){
	int rval = 0;
	char podtmp[8192] = {'\0'}; /* more than long enough for pod datatypes - 80bit long double range is 3.65×10^−4951 to 1.18×10^4932  */
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	if(node->onpodevent.touch)
	    node->onpodevent.touch(pf, path, tid, O_RDONLY, PROCFUSE_PRE);

	int printed = 0;
	switch(node->onpodevent.type){
		case T_PROC_POD_CHAR:
			if(node->onpodevent.types.c==NULL){
				rval = -EFAULT;
				break;
			}
			if(offset==0){
			    buffer[0] = *node->onpodevent.types.c;
			    rval = 1;
			}
			else
				rval = 0;
			break;
		case T_PROC_POD_INT:
			if(node->onpodevent.types.i==NULL){
				rval = -EFAULT;
				break;
			}
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%d", *node->onpodevent.types.i);
			break;
		case T_PROC_POD_INT64:
			if(node->onpodevent.types.l==NULL){
				rval = -EFAULT;
				break;
			}
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%"PRId64, *node->onpodevent.types.l);
			break;
		case T_PROC_POD_FLOAT:
			if(node->onpodevent.types.f==NULL){
				rval = -EFAULT;
				break;
			}
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%g", (double)*node->onpodevent.types.f);
			break;
		case T_PROC_POD_DOUBLE:
			if(node->onpodevent.types.d==NULL){
				rval = -EFAULT;
				break;
			}
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%g", *node->onpodevent.types.d);
			break;
		case T_PROC_POD_LONGDOUBLE:
			if(node->onpodevent.types.ld==NULL){
				rval = -EFAULT;
				break;
			}
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%Le", *node->onpodevent.types.ld);
			break;
		default:
			if(node->onpodevent.types.str.buffer==NULL || node->onpodevent.types.str.length==NULL){
				rval = -EFAULT;
				break;
			}
			off_t where = offset;
			size_t cpylen = size;
			if(where>*node->onpodevent.types.str.length){
				return 0;
			}
			if(cpylen>(size_t)((*node->onpodevent.types.str.length)-where)){
				cpylen = (*node->onpodevent.types.str.length)-where;
			}
			memcpy(buffer, (*node->onpodevent.types.str.buffer)+where, cpylen);
			rval = cpylen;
			break;
	}
	if(node->onpodevent.type!=T_PROC_POD_STRING && node->onpodevent.type!=T_PROC_POD_CHAR){
	    if(offset>printed)
		    return -EINVAL;
	    memcpy(buffer, podtmp+offset,printed-offset);
	    rval = printed-offset;
	}

	if(node->onpodevent.touch)
	    node->onpodevent.touch(pf, path, tid, O_RDONLY, PROCFUSE_POST);

	return rval;
}

int procfuse_onFuseWritePOD(const struct procfuse *pf, const char *path, const char *buffer, size_t size, off_t offset, int tid, const void* appdata){
	int rval = 0;
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	char *wbuff = NULL;
	int wlen = 0;

	struct procfuse_transactionnode *tnode = NULL;
	if(node->transactions!=NULL)
		tnode=(struct procfuse_transactionnode *)hash_table_lookup(node->transactions, &tid);

	if(tnode==NULL && node->onpodevent.type!=T_PROC_POD_CHAR && node->onpodevent.type!=T_PROC_POD_STRING){
		rval = -EIO;
	}

    if(rval==0 && node->onpodevent.type ==T_PROC_POD_STRING && node->onpodevent.touch)
        node->onpodevent.touch(pf, path, tid, O_WRONLY, PROCFUSE_PRE);

	switch(node->onpodevent.type){
	    case T_PROC_POD_CHAR:
	        *node->onpodevent.types.c = buffer[size-1]; /* write the last char in buffer */
	        rval = 1;
		    break;
	    case T_PROC_POD_STRING:
		    wbuff = *node->onpodevent.types.str.buffer;
		    wlen = *node->onpodevent.types.str.length;
		    break;
	    default:

		    wbuff = tnode->writebuffer;
		    wlen = tnode->length-1;

		    tnode->haswritten = 0; /* reset in case of an error */

		    break;
	}

	if(rval==0){
	    if(offset > (off_t)wlen){
		    rval = -EFBIG;
	    }
	    else{
		    size_t wsize = (size_t)((off_t)wlen-offset);
		    if(wsize > size){
			    wsize = size;
		    }
		    if(wsize>0){
		        memcpy(wbuff+offset, buffer, wsize);
		        rval = wsize;
		    }

		    if(node->onpodevent.type!=T_PROC_POD_STRING)
			    tnode->haswritten = 1;
	    }

    if(node->onpodevent.type ==T_PROC_POD_STRING && node->onpodevent.touch)
        node->onpodevent.touch(pf, path, tid, O_WRONLY, PROCFUSE_POST);

	}

	return rval;
}
int procfuse_onFuseReleasePOD(const struct procfuse *pf, const char *path, int tid, const void* appdata){
	int rval = 0;
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	if((node->onpodevent.flags & O_RDWR) == O_RDONLY ||
	   node->onpodevent.type==T_PROC_POD_STRING || node->onpodevent.type==T_PROC_POD_CHAR)
		return rval;

	struct procfuse_transactionnode *tnode = (struct procfuse_transactionnode *)hash_table_lookup(node->transactions, &tid);
	if(tnode==NULL){
		return rval;
	}

	if(node->onpodevent.touch)
	    node->onpodevent.touch(pf, path, tid, O_WRONLY, PROCFUSE_PRE);
	switch(node->onpodevent.type){
		case T_PROC_POD_INT:
			if(node->onpodevent.types.i==NULL){
				rval = -EFAULT;
				break;
			}
			*node->onpodevent.types.i = strtol(tnode->writebuffer, NULL, 0);
			break;
		case T_PROC_POD_INT64:
			if(node->onpodevent.types.l==NULL){
				rval = -EFAULT;
				break;
			}
			*node->onpodevent.types.l = strtol(tnode->writebuffer, NULL, 0);
			break;
		case T_PROC_POD_FLOAT:
			if(node->onpodevent.types.f==NULL){
				rval = -EFAULT;
				break;
			}
			*node->onpodevent.types.f = strtof(tnode->writebuffer, NULL);
			break;
		case T_PROC_POD_DOUBLE:
			if(node->onpodevent.types.d==NULL){
				rval = -EFAULT;
				break;
			}
			*node->onpodevent.types.d = strtod(tnode->writebuffer, NULL);
			break;
		case T_PROC_POD_LONGDOUBLE:
			if(node->onpodevent.types.ld==NULL){
				rval = -EFAULT;
				break;
			}
			*node->onpodevent.types.ld = strtold(tnode->writebuffer, NULL);
			break;
	}
	if(node->onpodevent.touch)
	    node->onpodevent.touch(pf, path, tid, O_WRONLY, PROCFUSE_POST);

	hash_table_remove(node->transactions, &tid);

    return rval;
}

/* FUSE functions */
int procfuse_getattr(const char *path, struct stat *stbuf)
{
	int rval = 0;

	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	pthread_mutex_lock(&pf->lock);

    struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	memset(stbuf, 0, sizeof(struct stat));
	if(node!=NULL && node->subdirs==NULL){
		stbuf->st_mode = S_IFREG;
		if(node->onevent.onFuseRead){
		    stbuf->st_mode |= (S_IRUSR | S_IRGRP | S_IROTH);
		}
		if(node->onevent.onFuseWrite){
			stbuf->st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH);
		}

		stbuf->st_nlink = 1;
	}
	else if(node!=NULL || strcmp(path, "/")==0){
		stbuf->st_mode = S_IFDIR | (S_IRWXU | S_IRWXG | S_IRWXO);

		stbuf->st_nlink = 2;
	}
	else{
		rval = -ENOENT;
	}

	pthread_mutex_unlock(&pf->lock);

	return rval;
}

int procfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi){
	HashTable *htable = NULL;
	int rval = 0;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	(void)fi;
	(void)off;

	pthread_mutex_lock(&pf->lock);

    struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

    if(node==NULL && strcmp(path,"/")==0){
    	htable = pf->root;
    }
    else if(node!=NULL && node->subdirs!=NULL){
    	htable = node->subdirs;
    }
    else{
    	rval = -ENOTDIR;
    }

    if(rval==0){
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_size = 0;

	    HashTableIterator iterator;
	    hash_table_iterate(htable, &iterator);
	    while (hash_table_iter_has_more(&iterator)) {
		    struct procfuse_hashnode *value = (struct procfuse_hashnode *)hash_table_iter_next(&iterator);

		    if(value==HASH_TABLE_NULL){
			    break;
		    }

		    st.st_mode = 0;

		    if(value->subdirs==NULL){
			    st.st_mode = S_IFREG;
			    if(value->onevent.onFuseRead){
				    st.st_mode |= (S_IRUSR | S_IRGRP | S_IROTH);
			    }
			    if(value->onevent.onFuseWrite){
				    st.st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH);
			    }
		    }
		    else{
			    st.st_mode = S_IFDIR | (S_IRWXU | S_IRWXG | S_IRWXO);
		    }

            if (filler(buf, value->key, &st, 0)){
                break;
            }
	    }
    }

	pthread_mutex_unlock(&pf->lock);

    return rval;
}

int procfuse_open(const char *path, struct fuse_file_info *fi){
	int rval = 0;

	procfuse_onFuseOpen onFuseOpen = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	pthread_mutex_lock(&pf->lock);

	pf->tidcounter++;
	fi->fh = pf->tidcounter;

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node==NULL || node->subdirs!=NULL){
		rval = -ENOENT;
	}
	else if(!node->onevent.onFuseRead && ((fi->flags & O_RDONLY) || (fi->flags & O_RDWR))){
		rval = -EACCES;
	}
	else if(!node->onevent.onFuseWrite && ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR))){
		rval = -EACCES;
	}
	else{
		onFuseOpen = node->onevent.onFuseOpen;
	}

	pthread_mutex_unlock(&pf->lock);

	if(onFuseOpen){
		const void *appdata = pf->appdata;
		if(node->onpodevent.type!=T_PROC_POD_NO)
			appdata = (const void*)node;
		onFuseOpen(pf, path, fi->fh, appdata);
	}

	return rval;
}
int procfuse_mknod(const char *path, mode_t mode, dev_t dev){
	(void)path;
	(void)mode;
	(void)dev;
	return 0;
}
int procfuse_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	(void)path;
	(void)mode;
	(void)fi;
	return 0;
}
int procfuse_truncate(const char *path, off_t off){
	procfuse_onFuseTruncate onFuseTruncate = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	(void)off;

	pthread_mutex_lock(&pf->lock);

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node!=NULL && node->subdirs==NULL){
		onFuseTruncate = node->onevent.onFuseTruncate;
	}

	pthread_mutex_unlock(&pf->lock);

	if(onFuseTruncate){
		const void *appdata = pf->appdata;
		if(node->onpodevent.type!=T_PROC_POD_NO)
			appdata = (const void*)node;
		onFuseTruncate(pf, path, appdata);
	}

	return 0;
}

int procfuse_read(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi)
{
	int rval = 0;
	procfuse_onFuseRead onFuseRead = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	pthread_mutex_lock(&pf->lock);

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node==NULL || node->subdirs!=NULL){
		rval = -ENOENT;
	}
	else if(!node->onevent.onFuseRead){
		rval = -EBADF;
	}
	else{
		onFuseRead = node->onevent.onFuseRead;
	}

	pthread_mutex_unlock(&pf->lock);

	if(onFuseRead){
		const void *appdata = pf->appdata;
		if(node->onpodevent.type!=T_PROC_POD_NO)
			appdata = (const void*)node;
		rval = onFuseRead(pf, path, buf, size, offset, fi->fh, appdata);
	}

	return rval;
}

int procfuse_write(const char *path, const char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi)
{
	int rval = 0;
	procfuse_onFuseWrite onFuseWrite = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	pthread_mutex_lock(&pf->lock);

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node==NULL || node->subdirs!=NULL){
		rval = -ENOENT;
	}
	else if(!node->onevent.onFuseWrite){
		rval = -EBADF;
	}
	else{
		onFuseWrite = node->onevent.onFuseWrite;
	}

	pthread_mutex_unlock(&pf->lock);

	if(onFuseWrite){
		const void *appdata = pf->appdata;
		if(node->onpodevent.type!=T_PROC_POD_NO)
			appdata = (const void*)node;
		rval = onFuseWrite(pf, path, buf, size, offset, fi->fh, appdata);

		if(rval==0){
			rval = -EIO;
		}
	}

	return rval;
}

int procfuse_release(const char *path, struct fuse_file_info *fi){
	procfuse_onFuseRelease onFuseRelease = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	pthread_mutex_lock(&pf->lock);

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node!=NULL && node->subdirs==NULL){
		onFuseRelease = node->onevent.onFuseRelease;
	}

	pthread_mutex_unlock(&pf->lock);

	if(onFuseRelease){
		const void *appdata = pf->appdata;
		if(node->onpodevent.type!=T_PROC_POD_NO)
			appdata = (const void*)node;
		onFuseRelease(pf, path, fi->fh, appdata);
	}
	fi->fh = 0;

	return 0;
}
/* EOF - End of Fuse */

void *procfuse_thread( void *ptr ){
	struct procfuse *pf = (struct procfuse *)ptr;
	char *mountpoint=NULL;
	int multithreaded=0;
	int res=0;

	sigset_t set;

	/* Block all signals in fuse thread - so all signals are delivered to another (main) thread */
	sigemptyset(&set);
	sigfillset(&set);
	pthread_sigmask(SIG_SETMASK, &set, NULL);

	pf->fuse = fuse_setup(pf->fuseArgc, (char**)pf->fuseArgv, &pf->procFS_oper, sizeof(pf->procFS_oper),
						  &mountpoint, &multithreaded, pf);
	if (pf->fuse == NULL)
			return NULL;

	if (multithreaded)
			res = fuse_loop_mt(pf->fuse);
	else
			res = fuse_loop(pf->fuse);

	fuse_teardown(pf->fuse, mountpoint);

	if (res == -1)
			return NULL;

	return NULL;
}

void procfuse_teardown(struct procfuse *pf){
	struct stat buf;

	if(pf==NULL){
		errno = EINVAL;
		return;
	}

	if(!fuse_exited(pf->fuse)){
	    fuse_exit(pf->fuse);
	    stat(pf->absolutemountpoint, &buf);
	}
}

void procfuse_caller(uid_t *u, gid_t *g, pid_t *p, mode_t *mask){
	struct fuse_context *ctx = fuse_get_context();
	if(ctx==NULL) return;
	if(u!=NULL) *u = ctx->uid;
	if(g!=NULL) *g = ctx->gid;
	if(p!=NULL) *p = ctx->pid;
	if(mask!=NULL) *mask = ctx->umask;
}
int procfuse_setSingleThreaded(struct procfuse *pf, int yes_or_no){
	if(pf==NULL){
		errno = EINVAL;
		return 0;
	}
	pf->fuse_singlethreaded = yes_or_no;
	return 1;
}

void procfuse_run(struct procfuse *pf, int blocking){
	if(pf==NULL){
		errno = EINVAL;
		return;
	}

	pf->procFS_oper.getattr	 = procfuse_getattr;
    pf->procFS_oper.readdir	 = procfuse_readdir;
    pf->procFS_oper.mknod    = procfuse_mknod;
    pf->procFS_oper.create   = procfuse_create;

    pf->procFS_oper.open	 = procfuse_open;
    pf->procFS_oper.truncate = procfuse_truncate;
    pf->procFS_oper.read	 = procfuse_read;
    pf->procFS_oper.write	 = procfuse_write;
    pf->procFS_oper.release	 = procfuse_release;

    pf->fuseArgc=2;
    if(pf->fuse_singlethreaded){
    	pf->fuseArgv[pf->fuseArgc++] = "-s";
    }
    pf->fuseArgv[pf->fuseArgc++] = "-f";
    pf->fuseArgv[pf->fuseArgc++] = "-o";
    if(pf->fuse_option==NULL || strstr(pf->fuse_option, "allow_")==NULL){
    	pf->fuseArgv[pf->fuseArgc++] = "direct_io,big_writes,default_permissions,nonempty,allow_other";
    }
    else{
    	pf->fuseArgv[pf->fuseArgc++] = "direct_io,big_writes,default_permissions,nonempty";
    }
    if(pf->fuse_option!=NULL){
    	pf->fuseArgv[pf->fuseArgc++] = "-o";
    	pf->fuseArgv[pf->fuseArgc++] = pf->fuse_option;
    }

    /* the reason for creating a thread:
     * to shutdown the procfuse filesystem one can just call procfuse_teardown()
     * i assume the user of procfuse will usually call it from within a signal handler
     *
     * this causes a problem for example:
     * if this function is called from the same thread which is running procfuse itself from within
     * a signal handler, the stat() functon call within procfuse_teardown is blocking and thus
     * freezes the whole process
     * why?
     * since we're within a signal handler during teardown call, fuse itself is interrupted (cause of the signal handler)
     * thus the stat() call can't reach fuse
     *
     * thats why i create a thread here and make sure that the thread doesnt listen to any signals
     */
    pthread_create( &pf->procfuseth, NULL, procfuse_thread, (void*) pf);
    if(blocking == PROCFUSE_BLOCK){
    	pthread_join(pf->procfuseth, NULL);
    }

}
