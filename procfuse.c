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
#include "compare-string.h"

#define PROCFUSE_NODETYPE_NONE -1
#define PROCFUSE_NODETYPE_ENDOFNODE 1
#define PROCFUSE_NODETYPE_SUBDIRNODE 0

#define PROCFUSE_YES 1
#define PROCFUSE_NO 0

#define PROCFUSE_FNAMELEN 512

void procfuse_freeNode(void *n){
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)n;
	if(node->eon==PROCFUSE_NODETYPE_SUBDIRNODE){
		hash_table_free(node->root);
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

int procfuse_ctor(struct procfuse *pf, const char *filesystemname, const char *absolutemountpoint, const char *fuse_option){
	if(pf==NULL || filesystemname==NULL || absolutemountpoint==NULL){
		errno = EINVAL;
		return 0;
	}
	memset(pf, '\0', sizeof(struct procfuse));
	pthread_mutex_init(&pf->lock, NULL);

	if(procfuse_ctorht(&pf->root)==0){
		return 0;
	}

    pf->fuseArgv[0] = strdup(filesystemname);
    pf->fuseArgv[1] = strdup(absolutemountpoint);
    pf->absolutemountpoint = strdup(absolutemountpoint);

	if(fuse_option!=NULL){
	    pf->option = strdup(fuse_option);
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

	return 1;
}

void procfuse_dtor(struct procfuse *pf){
	if(pf==NULL || pf->root==NULL){
		return;
	}

	free((void*)pf->fuseArgv[0]);
	free((void*)pf->fuseArgv[1]);
	free((void*)pf->absolutemountpoint);
	if(pf->option!=NULL){
	    free((void*)pf->option);
	}

	procfuse_dtorht(&pf->root);
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
			procfuse_ctorht(&node->root);
		}

		if(hash_table_insert(root, node->key, node)==0){
			procfuse_dtorht(&node->root);

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
		    return procfuse_pathToNode(node->root, absolutepath+flen+1, create);
	    }
	}
	/* in case of wrong node type is returned, it means an eexist error */
	errno = EEXIST;

	return node;
}

int procfuse_registerNode(struct procfuse *pf, const char *absolutepath, struct procfuse_accessor access){
	int rval = 0;
	struct procfuse_hashnode *node = NULL;

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_lock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	node = procfuse_pathToNode(pf->root, absolutepath, PROCFUSE_YES);
	if(node!=NULL){
		printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
		node->onevent = access;
		rval = 1;
	}

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_unlock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

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
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	if(node==NULL){
		errno = EEXIST;
		return 0;
	}

	printf("%s:%d:%s %d,%d,%s\n",__FILE__,__LINE__,__FUNCTION__, hassubpath, node->eon,node->key);
	if(hassubpath && node->eon==PROCFUSE_NODETYPE_SUBDIRNODE){
		int rval = procfuse_unregisterNodeInternal(node->root, absolutepath+flen+1);
		if(hash_table_num_entries(node->root)<=0){
			hash_table_remove(root, fname);
			node = NULL;
		}
		printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
		return rval;
	}
	else {
		hash_table_remove (root, fname);
		printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
		return 1;
	}
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	return 0;
}
int procfuse_unregisterNode(struct procfuse *pf, const char *absolutepath){
	int rval = 0;

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_lock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	rval = procfuse_unregisterNodeInternal(pf->root, absolutepath);

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_unlock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	return rval;
}

/* FUSE functions */
int procfuse_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_lock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

    struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	memset(stbuf, 0, sizeof(struct stat));
	if(node!=NULL && node->eon==PROCFUSE_NODETYPE_ENDOFNODE){
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
        res = -ENOENT;
	}

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_unlock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	return res;
}

int procfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t, struct fuse_file_info *){
	HashTable *htable = NULL;
	int rval = 0;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_lock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

    struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

    if(node==NULL && strcmp(path,"/")==0){
    	htable = pf->root;
    }
    else if(node!=NULL && node->eon!=PROCFUSE_NODETYPE_ENDOFNODE){
    	htable = node->root;
    }
    else{
    	rval = -ENOTDIR;
    }

    if(rval==0){
    	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
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

		    if(value->eon==PROCFUSE_NODETYPE_ENDOFNODE){
		    	printf("%s:%d:%s %s\n",__FILE__,__LINE__,__FUNCTION__, value->key);
			    st.st_mode = S_IFREG;
			    if(value->onevent.onFuseRead){
			    	printf("%s:%d:%s %s\n",__FILE__,__LINE__,__FUNCTION__, value->key);
				    st.st_mode |= (S_IRUSR | S_IRGRP | S_IROTH);
			    }
			    if(value->onevent.onFuseWrite){
			    	printf("%s:%d:%s %s\n",__FILE__,__LINE__,__FUNCTION__, value->key);
				    st.st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH);
			    }
		    }
		    else{
		    	printf("%s:%d:%s %s\n",__FILE__,__LINE__,__FUNCTION__, value->key);
			    st.st_mode = S_IFDIR | (S_IRWXU | S_IRWXG | S_IRWXO);
		    }
		    printf("%s:%d:%s %s\n",__FILE__,__LINE__,__FUNCTION__, value->key);

            if (filler(buf, value->key, &st, 0)){
                break;
            }
	    }
    }

    printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_unlock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

    return rval;
}

int procfuse_open(const char *path, struct fuse_file_info *fi){
	int rval = 0;

	procfuse_onFuseOpen onFuseOpen = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_lock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	pf->tidcounter++;
	fi->fh = pf->tidcounter;

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node==NULL || node->eon!=PROCFUSE_NODETYPE_ENDOFNODE){
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

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_unlock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	if(onFuseOpen){
		onFuseOpen(path, fi->fh);
	}

	printf("%s:%d:%s %d\n",__FILE__,__LINE__,__FUNCTION__, rval);

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

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_lock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node!=NULL && node->eon==PROCFUSE_NODETYPE_ENDOFNODE){
		onFuseTruncate = node->onevent.onFuseTruncate;
	}

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_unlock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	if(onFuseTruncate){
		onFuseTruncate(path);
	}

	return 0;
}

int procfuse_read(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi)
{
	int rval = 0;
	procfuse_onFuseRead onFuseRead = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_lock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node==NULL || node->eon!=PROCFUSE_NODETYPE_ENDOFNODE){
		rval = -ENOENT;
	}
	else if(!node->onevent.onFuseRead){
		rval = -EBADF;
	}
	else{
		onFuseRead = node->onevent.onFuseRead;
	}

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_unlock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	if(onFuseRead){
		printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
		rval = onFuseRead(path, buf, size, offset, fi->fh);
		printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	}

	printf("%s:%d:%s %d\n",__FILE__,__LINE__,__FUNCTION__, rval);

	return rval;
}

int procfuse_write(const char *path, const char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi)
{
	int rval = 0;
	procfuse_onFuseWrite onFuseWrite = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_lock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node==NULL || node->eon!=PROCFUSE_NODETYPE_ENDOFNODE){
		rval = -ENOENT;
	}
	else if(!node->onevent.onFuseWrite){
		rval = -EBADF;
	}
	else{
		onFuseWrite = node->onevent.onFuseWrite;
	}

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_unlock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	if(onFuseWrite){
		rval = onFuseWrite(path, buf, size, offset, fi->fh);
		if(rval==0){
			return -EIO;
		}
	}

	printf("%s:%d:%s %d\n",__FILE__,__LINE__,__FUNCTION__, rval);

	return rval;
}

int procfuse_release(const char *path, struct fuse_file_info *fi){
	procfuse_onFuseRelease onFuseRelease = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_lock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	struct procfuse_hashnode *node = procfuse_pathToNode(pf->root, path, PROCFUSE_NO);

	if(node!=NULL && node->eon==PROCFUSE_NODETYPE_ENDOFNODE){
		onFuseRelease = node->onevent.onFuseRelease;
	}

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
	pthread_mutex_unlock(&pf->lock);
	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	if(onFuseRelease){
		onFuseRelease(path, fi->fh);
	}
	fi->fh = 0;

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	return 0;
}

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

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	pf->fuse = fuse_setup(pf->fuseArgc, (char**)pf->fuseArgv, &pf->procFS_oper, sizeof(pf->procFS_oper),
						  &mountpoint, &multithreaded, pf);
	if (pf->fuse == NULL)
			return NULL;

	if (multithreaded)
			res = fuse_loop_mt(pf->fuse);
	else
			res = fuse_loop(pf->fuse);

	fuse_teardown(pf->fuse, mountpoint);

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

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

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);

	fuse_exit(pf->fuse);
	stat(pf->absolutemountpoint, &buf);

	printf("%s:%d:%s\n",__FILE__,__LINE__,__FUNCTION__);
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
    if(pf->option==NULL || strstr(pf->option, "allow_")==NULL){
    	pf->fuseArgv[pf->fuseArgc++] = "direct_io,big_writes,default_permissions,nonempty,allow_other";
    }
    else{
    	pf->fuseArgv[pf->fuseArgc++] = "direct_io,big_writes,default_permissions,nonempty";
    }
    if(pf->option!=NULL){
    	pf->fuseArgv[pf->fuseArgc++] = "-o";
    	pf->fuseArgv[pf->fuseArgc++] = pf->option;
    }

    /* the reaseon for creating a thread:
     * to shutdown the procfuse filesystem one can just call procfuse_teardown()
     * i assume, this will usually be called from within a signal handler
     *
     * if this function is called from the same thread which is running procfuse itself within
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
