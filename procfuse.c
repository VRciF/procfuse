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

#include "hash-table.h"
#include "hash-string.h"
#include "hash-int.h"
#include "compare-string.h"
#include "compare-int.h"

#define PROCFUSE_DELIMC '/'
#define PROCFUSE_DELIMS "/"

#define PROCFUSE_NODETYPE_NONE -1
#define PROCFUSE_NODETYPE_SUBDIRNODE 0
#define PROCFUSE_NODETYPE_ENDOFNODE 1
#define PROCFUSE_NODETYPE_ENDOFNODE_POD 2

#define PROCFUSE_YES 1
#define PROCFUSE_NO 0

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

enum PROCFS_TYPE_POD{ T_MIN=0, T_CHAR, T_INT, T_INT64, T_FLOAT, T_DOUBLE, T_LONGDOUBLE, T_STRING, T_MAX};

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
	enum PROCFS_TYPE_POD type;
};

struct procfuse_accessor{
	procfuse_onFuseOpen onFuseOpen;

    procfuse_onFuseTruncate onFuseTruncate;
    procfuse_onFuseRead onFuseRead;
    procfuse_onFuseWrite onFuseWrite;

	procfuse_onFuseRelease onFuseRelease;

    struct {
        struct procfuse_pod_accessor onevent;

    	HashTable *transactions;
    }pod;
};
struct procfuse_transactiondata{
    char *writebuffer; /* used in procfuse_onFuseWritePOD */
    int writesize;     /* sizeo of allocated writebuffer */
    int writeoffset;   /* how many bytes have been written into writebuffer */
    int podoffset;     /* where did procfuse_onFuseWritePOD started to write into pod - only used for string type */
};

struct procfuse_transactionnode{
	struct procfuse_transactiondata data;
	int tid;
};

struct procfuse_hashnode{
	union{
		HashTable *root;

	    struct procfuse_accessor onevent;
	}data;
	char *key;
	int eon; /* end of node */
};

int procfuse_onFuseOpenPOD(const struct procfuse *pf, const char *path, int tid, const void* appdata);
int procfuse_onFuseReadPOD(const struct procfuse *pf, const char *path, char *buffer, size_t size, off_t offset, int tid, const void* appdata);
int procfuse_onFuseWritePOD(const struct procfuse *pf, const char *path, const char *buffer, size_t size, off_t offset, int tid, const void* appdata);
int procfuse_onFuseReleasePOD(const struct procfuse *pf, const char *path, int tid, const void* appdata);

void procfuse_freeNode(void *n){
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)n;
	if(node->eon==PROCFUSE_NODETYPE_SUBDIRNODE){
		hash_table_free(node->data.root);
	}

	free(node);
}
int procfuse_ctorht(HashTable **ht){
	if(ht==NULL){
		errno = EINVAL;
		return 0;
	}
	*ht = hash_table_new(string_hash, string_equal);
	if(*ht==NULL){
		return 0;
	}
	hash_table_register_free_functions(*ht, free, procfuse_freeNode);
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

struct procfuse* procfuse_ctor(const char *filesystemname, const char *absolutemountpoint, const char *fuse_option,const void *appdata){
	struct procfuse *pf = calloc(1, sizeof(struct procfuse));
	if(pf==NULL || filesystemname==NULL || absolutemountpoint==NULL){
		errno = EINVAL;
		return NULL;
	}
	memset(pf, '\0', sizeof(struct procfuse));
	pthread_mutex_init(&pf->lock, NULL);

	if(procfuse_ctorht(&pf->root)==0){
		return NULL;
	}

    pf->fuseArgv[0] = strdup(filesystemname);
    pf->fuseArgv[1] = strdup(absolutemountpoint);
    pf->absolutemountpoint = strdup(absolutemountpoint);

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
	free((void*)pf->fuseArgv[1]);
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

		if(value->eon){
			printf("%s:%d:%s node = %s\n",__FILE__,__LINE__,__FUNCTION__, value->key);
		}
		else{
			printf("%s:%d:%s subdir = %s\n",__FILE__,__LINE__,__FUNCTION__, value->key);
			procfuse_printTree(value->data.root);
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

int procfuse_hasNextNode(HashTable *root, char *fname){
	return hash_table_lookup(root, fname)!=NULL;
}
struct procfuse_hashnode* procfuse_getNextNode(HashTable *root, char *fname, int newtype){
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)hash_table_lookup(root, fname);
	if(node==NULL && newtype!=PROCFUSE_NODETYPE_NONE){
		node = (struct procfuse_hashnode *)calloc(sizeof(struct procfuse_hashnode), 1);
		if(node==NULL){
			errno = ENOMEM;
			return NULL;
		}
		node->eon = newtype;
		node->key = strdup(fname);

		if(node->eon == PROCFUSE_NODETYPE_SUBDIRNODE){
			procfuse_ctorht(&node->data.root);
		}

		if(hash_table_insert(root, node->key, node)==0){
			procfuse_dtorht(&node->data.root);

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
		node = procfuse_getNextNode(root, fname, create==PROCFUSE_YES ? PROCFUSE_NODETYPE_SUBDIRNODE : PROCFUSE_NODETYPE_NONE);
	}
	else{
		node = procfuse_getNextNode(root, fname, create==PROCFUSE_YES ? PROCFUSE_NODETYPE_ENDOFNODE : PROCFUSE_NODETYPE_NONE);
	}

	if(node!=NULL){
	    if(hassubpath && node->eon==PROCFUSE_NODETYPE_SUBDIRNODE){
		    return procfuse_pathToNode(node->data.root, absolutepath+flen+1, create);
	    }
	}
	/* in case of wrong node type is returned, it means an eexist error */
	errno = EEXIST;

	return node;
}

struct procfuse_pod_accessor procfuse_podaccessorChar(char *c, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	podaccess.touch = touch;

	podaccess.types.c = c;
	podaccess.type = T_CHAR;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorInt(int *i, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	podaccess.touch = touch;

	podaccess.types.i = i;
	podaccess.type = T_INT;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorInt64(int64_t *l, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	podaccess.touch = touch;

	podaccess.types.l = l;
	podaccess.type = T_INT64;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorFloat(float *f, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	podaccess.touch = touch;

	podaccess.types.f = f;
	podaccess.type = T_FLOAT;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorDouble(double *d, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	podaccess.touch = touch;

	podaccess.types.d = d;
	podaccess.type = T_DOUBLE;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorLongDouble(long double *ld, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	podaccess.touch = touch;

	podaccess.types.ld = ld;
	podaccess.type = T_LONGDOUBLE;

	return podaccess;
}
struct procfuse_pod_accessor procfuse_podaccessorString(char **buffer, int *length, procfuse_touch touch){
	struct procfuse_pod_accessor podaccess;
	podaccess.touch = touch;

	podaccess.types.str.buffer = buffer;
	podaccess.types.str.length = length;
	podaccess.type = T_STRING;

	return podaccess;
}
struct procfuse_accessor procfuse_accessor(procfuse_onFuseOpen onFuseOpen, procfuse_onFuseTruncate onFuseTruncate,
                                           procfuse_onFuseRead onFuseRead, procfuse_onFuseWrite onFuseWrite,
                                           procfuse_onFuseRelease onFuseRelease){
	struct procfuse_accessor access;
	access.onFuseOpen = onFuseOpen;
	access.onFuseTruncate = onFuseTruncate;
	access.onFuseRead = onFuseRead;
	access.onFuseWrite = onFuseWrite;
	access.onFuseRelease = onFuseRelease;
	return access;
}

int procfuse_registerNode(struct procfuse *pf, const char *absolutepath, struct procfuse_accessor access){
	int rval = 0;
	struct procfuse_hashnode *node = NULL;

	pthread_mutex_lock(&pf->lock);

	node = procfuse_pathToNode(pf->root, absolutepath, PROCFUSE_YES);
	if(node!=NULL){
		node->data.onevent = access;
		node->eon = PROCFUSE_NODETYPE_ENDOFNODE;
		rval = 1;
	}

	pthread_mutex_unlock(&pf->lock);

	return rval;
}

int procfuse_registerNodePOD(struct procfuse *pf, const char *absolutepath, struct procfuse_pod_accessor podaccess){
	int rval = 0;
	struct procfuse_hashnode *node = NULL;

	pthread_mutex_lock(&pf->lock);

	node = procfuse_pathToNode(pf->root, absolutepath, PROCFUSE_YES);
	if(node!=NULL){
		struct procfuse_accessor access;
		access.onFuseOpen = procfuse_onFuseOpenPOD;
		access.onFuseRead = procfuse_onFuseReadPOD;
		access.onFuseWrite = procfuse_onFuseWritePOD;
		access.onFuseRelease = procfuse_onFuseReleasePOD;
		access.pod.onevent = podaccess;
		access.pod.transactions = hash_table_new(int_hash, int_equal);

		node->data.onevent = access;
		node->eon = PROCFUSE_NODETYPE_ENDOFNODE_POD;
		rval = 1;
	}

	pthread_mutex_unlock(&pf->lock);

	return rval;
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
	node = procfuse_getNextNode(root, fname, PROCFUSE_NODETYPE_NONE);
	if(node==NULL){
		errno = EEXIST;
		return 0;
	}

	if(hassubpath && node->eon==PROCFUSE_NODETYPE_SUBDIRNODE){
		int rval = procfuse_unregisterNodeInternal(node->data.root, absolutepath+flen+1);
		if(hash_table_num_entries(node->data.root)<=0){
			hash_table_remove(root, fname);
			node = NULL;
		}
		return rval;
	}
	else {
		hash_table_remove (root, fname);
		return 1;
	}
	return 0;
}
int procfuse_unregisterNode(struct procfuse *pf, const char *absolutepath){
	int rval = 0;

	pthread_mutex_lock(&pf->lock);

	rval = procfuse_unregisterNodeInternal(pf->root, absolutepath);

	pthread_mutex_unlock(&pf->lock);

	return rval;
}


struct procfuse_transactionnode* procfuse_getTransactionNode(enum PROCFS_TYPE_POD type, int tid){
	struct procfuse_transactionnode *tnode = (struct procfuse_transactionnode *)calloc(1, sizeof(struct procfuse_transactionnode));
	if(tnode==NULL){ return NULL; }

	size_t s=0;
	switch(type){
		case T_CHAR: s = sizeof(char)*3+1; break;
		case T_INT: s = sizeof(int)*3+1; break;
		case T_INT64: s = sizeof(int64_t)*3+1; break;
		case T_FLOAT: s = sizeof(float)*3+1; break;
		case T_DOUBLE: s = sizeof(double)*3+1; break;
		case T_LONGDOUBLE: s = sizeof(long double)*3+1; break;
		default: s = 8192; break;
	}

	tnode->data.writebuffer = (char*)calloc(1, s);
	tnode->data.writesize = s;
	tnode->data.writeoffset = tnode->data.podoffset = 0;

	tnode->tid = tid;

	return tnode;
}

int procfuse_onFuseOpenPOD(const struct procfuse *, const char *, int tid, const void* appdata){
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	struct procfuse_transactionnode *tnode = procfuse_getTransactionNode(node->data.onevent.pod.onevent.type, tid);
	hash_table_insert(node->data.onevent.pod.transactions, &tnode->tid, tnode);

	return 0;
}
int procfuse_onFuseReadPOD(const struct procfuse *pf, const char *path, char *buffer, size_t size, off_t offset, int tid, const void* appdata){
	int rval = 0;
	char podtmp[100] = {'\0'};
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	if(node->data.onevent.pod.onevent.touch)
	    node->data.onevent.pod.onevent.touch(pf, path, tid, O_RDONLY, PROCFUSE_PRE);

	int printed = 0;
	switch(node->data.onevent.pod.onevent.type){
		case T_CHAR:
			if(node->data.onevent.pod.onevent.types.c==NULL){
				rval = -EFAULT;
				break;
			}
			if(offset==0){
			    buffer[0] = *node->data.onevent.pod.onevent.types.c;
			    rval = 1;
			}
			else
				rval = 0;
			break;
		case T_INT:
			if(node->data.onevent.pod.onevent.types.i==NULL){
				rval = -EFAULT;
				break;
			}
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%d", *node->data.onevent.pod.onevent.types.i);
			break;
		case T_INT64:
			if(node->data.onevent.pod.onevent.types.l==NULL){
				rval = -EFAULT;
				break;
			}
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%ld", *node->data.onevent.pod.onevent.types.l);
			break;
		case T_FLOAT:
			if(node->data.onevent.pod.onevent.types.f==NULL){
				rval = -EFAULT;
				break;
			}
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%f", *node->data.onevent.pod.onevent.types.f);
			break;
		case T_DOUBLE:
			if(node->data.onevent.pod.onevent.types.d==NULL){
				rval = -EFAULT;
				break;
			}
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%f", *node->data.onevent.pod.onevent.types.d);
			break;
		case T_LONGDOUBLE:
			if(node->data.onevent.pod.onevent.types.ld==NULL){
				rval = -EFAULT;
				break;
			}
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%L", *node->data.onevent.pod.onevent.types.ld);
			break;
		default:
			if(node->data.onevent.pod.onevent.types.str.buffer==NULL || node->data.onevent.pod.onevent.types.str.length==NULL){
				rval = -EFAULT;
				break;
			}
			off_t where = offset;
			size_t cpylen = size;
			if(where>*node->data.onevent.pod.onevent.types.str.length){
				return 0;
			}
			if(cpylen>(*node->data.onevent.pod.onevent.types.str.length)-where){
				cpylen = (*node->data.onevent.pod.onevent.types.str.length)-where;
			}
			memcpy(buffer, (*node->data.onevent.pod.onevent.types.str.buffer)+where, cpylen);
			rval = cpylen;
			break;
	}
	if(node->data.onevent.pod.onevent.type!=T_STRING && node->data.onevent.pod.onevent.type!=T_CHAR){
	    if(offset>printed)
		    return -EINVAL;
	    memcpy(buffer, podtmp+offset,printed-offset);
	    rval = printed-offset;
	}

	if(node->data.onevent.pod.onevent.touch)
	    node->data.onevent.pod.onevent.touch(pf, path, tid, O_RDONLY, PROCFUSE_POST);

	return rval;
}
int procfuse_onFuseWritePOD(const struct procfuse *pf, const char *path, const char *buffer, size_t size, off_t offset, int tid, const void* appdata){
	int rval = 0;
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	if(node->data.onevent.pod.onevent.touch)
	    node->data.onevent.pod.onevent.touch(pf, path, tid, O_RDONLY, PROCFUSE_PRE);

	if(node->data.onevent.pod.onevent.touch)
	    node->data.onevent.pod.onevent.touch(pf, path, tid, O_RDONLY, PROCFUSE_POST);

	return rval;
}
int procfuse_onFuseReleasePOD(const struct procfuse *pf, const char *path, int tid, const void* appdata){
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	struct procfuse_transactionnode *tnode = hash_table_lookup(node->data.onevent.pod.transactions, &tid);
	if(tnode!=NULL){
		hash_table_remove(node->data.onevent.pod.transactions, &tid);
		free(tnode->data.writebuffer);
		free(tnode);
	}

	return 0;
}

/* FUSE functions */
int procfuse_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	pthread_mutex_lock(&pf->lock);

    struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	memset(stbuf, 0, sizeof(struct stat));
	if(node!=NULL && node->eon>=PROCFUSE_NODETYPE_ENDOFNODE){
		stbuf->st_mode = S_IFREG;
		if(node->data.onevent.onFuseRead){
		    stbuf->st_mode |= (S_IRUSR | S_IRGRP | S_IROTH);
		}
		if(node->data.onevent.onFuseWrite){
			stbuf->st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH);
		}

		stbuf->st_nlink = 1;
	}
	else if(node!=NULL || strcmp(path, "/")==0){
		stbuf->st_mode = S_IFDIR | (S_IRWXU | S_IRWXG | S_IRWXO);

		stbuf->st_nlink = 2;
	}
	else{
        res = -ENOENT;
	}

	pthread_mutex_unlock(&pf->lock);

	return res;
}

int procfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, struct fuse_file_info *){
	HashTable *htable = NULL;
	int rval = 0;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	pthread_mutex_lock(&pf->lock);

    struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

    if(node==NULL && strcmp(path,"/")==0){
    	htable = pf->root;
    }
    else if(node!=NULL && node->eon>=PROCFUSE_NODETYPE_ENDOFNODE){
    	htable = node->data.root;
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

		    if(value->eon>=PROCFUSE_NODETYPE_ENDOFNODE){
			    st.st_mode = S_IFREG;
			    if(value->data.onevent.onFuseRead){
				    st.st_mode |= (S_IRUSR | S_IRGRP | S_IROTH);
			    }
			    if(value->data.onevent.onFuseWrite){
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

	if(node==NULL || node->eon<PROCFUSE_NODETYPE_ENDOFNODE){
		rval = -ENOENT;
	}
	else if(!node->data.onevent.onFuseRead && ((fi->flags & O_RDONLY) || (fi->flags & O_RDWR))){
		rval = -EACCES;
	}
	else if(!node->data.onevent.onFuseWrite && ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR))){
		rval = -EACCES;
	}
	else{
		onFuseOpen = node->data.onevent.onFuseOpen;
	}

	pthread_mutex_unlock(&pf->lock);

	if(onFuseOpen){
		const void *appdata = pf->appdata;
		if(node->eon == PROCFUSE_NODETYPE_ENDOFNODE_POD)
			appdata = (const void*)node;
		onFuseOpen(pf, path, fi->fh, appdata);
	}

	return rval;
}
int procfuse_mknod(const char *, mode_t, dev_t){
	return 0;
}
int procfuse_create(const char *, mode_t, struct fuse_file_info *){
	return 0;
}
int procfuse_truncate(const char *path, off_t){
	procfuse_onFuseTruncate onFuseTruncate = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	pthread_mutex_lock(&pf->lock);

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node!=NULL && node->eon>=PROCFUSE_NODETYPE_ENDOFNODE){
		onFuseTruncate = node->data.onevent.onFuseTruncate;
	}

	pthread_mutex_unlock(&pf->lock);

	if(onFuseTruncate){
		const void *appdata = pf->appdata;
		if(node->eon == PROCFUSE_NODETYPE_ENDOFNODE_POD)
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

	if(node==NULL || node->eon<PROCFUSE_NODETYPE_ENDOFNODE){
		rval = -ENOENT;
	}
	else if(!node->data.onevent.onFuseRead){
		rval = -EBADF;
	}
	else{
		onFuseRead = node->data.onevent.onFuseRead;
	}

	pthread_mutex_unlock(&pf->lock);

	if(onFuseRead){
		const void *appdata = pf->appdata;
		if(node->eon == PROCFUSE_NODETYPE_ENDOFNODE_POD)
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

	if(node==NULL || node->eon<PROCFUSE_NODETYPE_ENDOFNODE){
		rval = -ENOENT;
	}
	else if(!node->data.onevent.onFuseWrite){
		rval = -EBADF;
	}
	else{
		onFuseWrite = node->data.onevent.onFuseWrite;
	}

	pthread_mutex_unlock(&pf->lock);

	if(onFuseWrite){
		const void *appdata = pf->appdata;
		if(node->eon == PROCFUSE_NODETYPE_ENDOFNODE_POD)
			appdata = (const void*)node;
		rval = onFuseWrite(pf, path, buf, size, offset, fi->fh, appdata);

		if(rval==0){
			return -EIO;
		}
	}

	return rval;
}

int procfuse_release(const char *path, struct fuse_file_info *fi){
	procfuse_onFuseRelease onFuseRelease = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	pthread_mutex_lock(&pf->lock);

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node!=NULL && node->eon>=PROCFUSE_NODETYPE_ENDOFNODE){
		onFuseRelease = node->data.onevent.onFuseRelease;
	}

	pthread_mutex_unlock(&pf->lock);

	if(onFuseRelease){
		const void *appdata = pf->appdata;
		if(node->eon == PROCFUSE_NODETYPE_ENDOFNODE_POD)
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
	if(pf==NULL){
		errno = EINVAL;
		return;
	}

	struct stat buf;

	fuse_exit(pf->fuse);
	stat(pf->absolutemountpoint, &buf);
}

const void* procfuse_appdata(const struct procfuse *pf){
	if(pf==NULL){
		errno = EINVAL;
		return NULL;
	}

	return pf->appdata;
}
void procfuse_caller(uid_t *u, gid_t *g, pid_t *p, mode_t *mask){
	struct fuse_context *ctx = fuse_get_context();
	if(ctx==NULL) return;
	if(u!=NULL) *u = ctx->uid;
	if(g!=NULL) *g = ctx->gid;
	if(p!=NULL) *p = ctx->pid;
	if(mask!=NULL) *mask = ctx->umask;
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
