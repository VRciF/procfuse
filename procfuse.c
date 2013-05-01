/*
 * procfuse.c
 *
 */

#include "procfuse.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>

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
	if(!node->eon){
		hash_table_free(node->subdir.root);
	}

	free(node);
}

int procfuse_ctor(struct procfuse *pf, const char *filesystemname, const char *absolutemountpoint, const char *fuse_option){
	if(pf==NULL){
		errno = EINVAL;
		return 0;
	}
	memset(pf, '\0', sizeof(struct procfuse));
	pthread_mutex_init(&pf->lock, NULL);

	pf->root = hash_table_new(string_hash, string_equal);
	if(pf->root==NULL){
		return 0;
	}

	if(filesystemname!=NULL){
	    pf->fuseArgv[0] = strdup(filesystemname);
	}
	if(absolutemountpoint!=NULL){
	    pf->fuseArgv[1] = strdup(absolutemountpoint);
	}
	if(fuse_option!=NULL){
	    pf->option = strdup(fuse_option);
	}

	pf->fuse_singlethreaded = 1;

	/* Automatically free all the values with the hash table */
	hash_table_register_free_functions(pf->root, free, procfuse_freeNode);
	return 1;
}

void procfuse_dtor(struct procfuse *pf){
	if(pf==NULL || pf->root==NULL){
		return;
	}

	free((void*)pf->fuseArgv[0]);
	free((void*)pf->fuseArgv[1]);
	free((void*)pf->option);

	hash_table_free(pf->root);
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
	int len = 0;
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
	memcpy(fname, trimedabsolutepath+1, len);

	return 1;
}

int procfuse_hasNextNode(struct procfuse *pf, char *fname){
	return hash_table_lookup(pf->root, fname)!=NULL;
}
struct procfuse_hashnode* procfuse_getNextNode(struct procfuse *pf, char *fname, int newtype){
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)hash_table_lookup(pf->root, fname);
	if(node==NULL && newtype>-1){
		node = (struct procfuse_hashnode *)calloc(sizeof(struct procfuse_hashnode), 1);
		if(node==NULL){
			errno = ENOMEM;
			return NULL;
		}
		node->eon = newtype;
		if(node->eon == PROCFUSE_NODETYPE_SUBDIRNODE){
			procfuse_ctor(&node->subdir, NULL, NULL, NULL);
		}

		char *keyfname = strdup(fname);
		if(hash_table_insert(node->subdir.root, keyfname, node)==0){
			procfuse_dtor(&node->subdir);

			free(keyfname);
			free(node);

			node = NULL;
		}
	}
	return node;
}

struct procfuse_hashnode* procfuse_pathToNode(struct procfuse *pf, const char *absolutepath, int create){
	char fname[PROCFUSE_FNAMELEN] = {'\0'};

	if(pf==NULL || absolutepath==NULL){
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
		node = procfuse_getNextNode(pf, fname, create==PROCFUSE_YES ? PROCFUSE_NODETYPE_SUBDIRNODE : -1);
	}
	else{
		node = procfuse_getNextNode(pf, fname, create==PROCFUSE_YES ? PROCFUSE_NODETYPE_ENDOFNODE : -1);
	}

	if(node!=NULL){
	    if(hassubpath && node->eon==PROCFUSE_NODETYPE_SUBDIRNODE){
		    return procfuse_pathToNode(&node->subdir, absolutepath+flen+1, create);
	    }
	}
	/* in case of wrong node type is returned, it means an eexist error */
	errno = EEXIST;

	return node;
}

int procfuse_registerNode(struct procfuse *pf, const char *absolutepath, struct procfuse_accessor access){
	struct procfuse_hashnode *node = procfuse_pathToNode(pf, absolutepath, PROCFUSE_YES);

	if(node==NULL){
		return 0;
	}

	node->onevent = access;

	return 1;
}
int procfuse_unregisterNode(struct procfuse *pf, const char *absolutepath){
	char fname[PROCFUSE_FNAMELEN] = {'\0'};

	if(pf==NULL || absolutepath==NULL){
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
	 *    node exists AND is subdir node AND absolutepath is subdir => recursive registerNode
	 *    node not exists => alloc node as neeeded
	 */
	struct procfuse_hashnode *node = NULL;
	node = procfuse_getNextNode(pf, fname, PROCFUSE_NODETYPE_NONE);
	if(node==NULL){
		errno = EEXIST;
		return 0;
	}

	if(hassubpath && node->eon==PROCFUSE_NODETYPE_SUBDIRNODE){
		int rval = procfuse_unregisterNode(&node->subdir, absolutepath+flen+1);
		if(hash_table_num_entries(node->subdir.root)<=0){
			hash_table_remove(pf->root, fname);
			node = NULL;
		}
		return rval;
	}
	else if(!hassubpath && node->eon==PROCFUSE_NODETYPE_ENDOFNODE){
		hash_table_remove (pf->root, fname);
		return 1;
	}
	return 0;
}

/* FUSE functions */

int procfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;
    struct procfuse_hashnode *node = procfuse_pathToNode(pf, path, PROCFUSE_NO);

    if(node==NULL || node->eon==PROCFUSE_NODETYPE_ENDOFNODE){
        return -ENOTDIR;
    }

    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_size = 0;

	HashTableIterator iterator;
	hash_table_iterate(node->subdir.root, &iterator);
	while (hash_table_iter_has_more(&iterator)) {
		char *key = (char*)hash_table_iter_next(&iterator);

		if(key==HASH_TABLE_NULL){
			break;
		}

		st.st_mode = 0;

		struct procfuse_hashnode *value = (struct procfuse_hashnode *)hash_table_lookup	(node->subdir.root, key);
		if(value){
			if(value->eon==PROCFUSE_NODETYPE_ENDOFNODE){
				if(!value->onevent.onFuseRead){
					st.st_mode |= S_IRUSR;
					st.st_mode |= S_IRGRP;
					st.st_mode |= S_IROTH;
				}
				if(value->onevent.onFuseWrite){
					st.st_mode |= S_IWUSR;
					st.st_mode |= S_IWGRP;
					st.st_mode |= S_IWOTH;
				}
			}
			else{
				st.st_mode |= S_IRWXU;
				st.st_mode |= S_IRWXG;
				st.st_mode |= S_IRWXO;
			}
		}
        if (filler(buf, key, &st, 0)){
            break;
        }
	}

    return 0;
}

int procfuse_open(const char *path, struct fuse_file_info *fi){
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;
	struct procfuse_hashnode *node = procfuse_pathToNode(pf, path, PROCFUSE_NO);

	if(node==NULL || node->eon!=PROCFUSE_NODETYPE_ENDOFNODE){
		return -ENOENT;
	}

	if(!node->onevent.onFuseRead && ((fi->flags & O_RDONLY) || (fi->flags & O_RDWR))){
		return -EACCES;
	}
	if(!node->onevent.onFuseWrite && ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR))){
		return -EACCES;
	}

	return 0;
}


static int procfuse_read(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi)
{
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;
	struct procfuse_hashnode *node = procfuse_pathToNode(pf, path, PROCFUSE_NO);

	if(node==NULL || node->eon!=PROCFUSE_NODETYPE_ENDOFNODE){
		return -ENOENT;
	}
	if(!node->onevent.onFuseRead){
		return -EBADF;
	}

	return node->onevent.onFuseRead(path, buf, size, offset);
}

static int procfuse_write(const char *path, const char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi)
{
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;
	struct procfuse_hashnode *node = procfuse_pathToNode(pf, path, PROCFUSE_NO);

	if(node==NULL || node->eon!=PROCFUSE_NODETYPE_ENDOFNODE){
		return -ENOENT;
	}
	if(!node->onevent.onFuseWrite){
		return -EBADF;
	}

	return node->onevent.onFuseWrite(path, buf, size, offset);
}


void *procfuse_thread( void *ptr ){
	struct procfuse *pf = (struct procfuse *)ptr;

    fuse_main(pf->fuseArgc, (char**)pf->fuseArgv, &pf->procFS_oper, pf);

    return NULL;
}

void procfuse_main(struct procfuse *pf, int blocking){
    pf->procFS_oper.readdir	 = procfuse_readdir;
    pf->procFS_oper.open	 = procfuse_open;
    pf->procFS_oper.read	 = procfuse_read;
    pf->procFS_oper.write	 = procfuse_write;

    pf->fuseArgc=2;
    if(pf->fuse_singlethreaded){
    	pf->fuseArgv[pf->fuseArgc++] = "-s";
    }
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

    pthread_create( &pf->procfuseth, NULL, procfuse_thread, (void*) pf);
    if(blocking == PROCFUSE_BLOCK){
    	pthread_join( pf->procfuseth, NULL);
    }
}
