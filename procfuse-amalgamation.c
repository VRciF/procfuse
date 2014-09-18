#include "procfuse-amalgamation.h"
/*

Copyright (c) 2005-2008, Simon Howard

Permission to use, copy, modify, and/or distribute this software 
for any purpose with or without fee is hereby granted, provided 
that the above copyright notice and this permission notice appear 
in all copies. 

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE 
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR 
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN      
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 

 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>


/* Comparison functions for strings */

int string_equal(void *string1, void *string2)
{
	return strcmp((char *) string1, (char *) string2) == 0;
}

int string_compare(void *string1, void *string2)
{
	int result;

	result = strcmp((char *) string1, (char *) string2);
	
	if (result < 0) {
		return -1;
	} else if (result > 0) {
		return 1;
	} else {
		return 0;
	}
}

/* Comparison functions for strings, which ignore the case of letters. */

int string_nocase_equal(void *string1, void *string2)
{
	return string_nocase_compare((char *) string1, (char *) string2) == 0;
}

/* On many systems, strcasecmp or stricmp will give the same functionality
 * as this function.  However, it is non-standard and cannot be relied
 * on to be present. */

int string_nocase_compare(void *string1, void *string2)
{
	char *p1;
	char *p2;
	int c1, c2;

	/* Iterate over each character in the strings */

	p1 = (char *) string1;
	p2 = (char *) string2;

	for (;;) {

		c1 = tolower(*p1);
		c2 = tolower(*p2);

		if (c1 != c2) {

			/* Strings are different */

			if (c1 < c2) {
				return -1;
			} else {
				return 1;
			}
		}

		/* End of string */

		if (c1 == '\0')
			break;

		/* Advance to the next character */

		++p1;
		++p2;
	}

	/* Reached the end of string and no difference found */

	return 0;
}



/*

Copyright (c) 2005-2008, Simon Howard

Permission to use, copy, modify, and/or distribute this software 
for any purpose with or without fee is hereby granted, provided 
that the above copyright notice and this permission notice appear 
in all copies. 

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE 
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR 
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN      
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 

 */


/* Comparison functions for a pointer to an integer */

int int_equal(void *vlocation1, void *vlocation2)
{
	int *location1;
	int *location2;

	location1 = (int *) vlocation1;
	location2 = (int *) vlocation2;

	return *location1 == *location2;
}

int int_compare(void *vlocation1, void *vlocation2)
{
	int *location1;
	int *location2;

	location1 = (int *) vlocation1;
	location2 = (int *) vlocation2;

	if (*location1 < *location2) {
		return -1;
	} else if (*location1 > *location2) {
		return 1;
	} else {
		return 0;
	}
}



/*

Copyright (c) 2005-2008, Simon Howard

Permission to use, copy, modify, and/or distribute this software 
for any purpose with or without fee is hereby granted, provided 
that the above copyright notice and this permission notice appear 
in all copies. 

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE 
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR 
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN      
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 

 */


/* Hash function for a pointer to an integer */

unsigned long int_hash(void *vlocation)
{
	int *location;

	location = (int *) vlocation;

	return (unsigned long) *location;
}


/*

Copyright (c) 2005-2008, Simon Howard

Permission to use, copy, modify, and/or distribute this software 
for any purpose with or without fee is hereby granted, provided 
that the above copyright notice and this permission notice appear 
in all copies. 

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE 
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR 
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN      
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 

 */

#include <ctype.h>


/* String hash function */

unsigned long string_hash(void *string)
{
	/* This is the djb2 string hash function */

	unsigned long result = 5381;
	unsigned char *p;

	p = (unsigned char *) string;

	while (*p != '\0') {
		result = ((result << 5) ^ result ) ^ (*p);
		++p;
	}

	return result;
}

/* The same function, with a tolower on every character so that 
 * case is ignored.  This code is duplicated for performance. */

unsigned long string_nocase_hash(void *string)
{
	unsigned long result = 5381;
	unsigned char *p;

	p = (unsigned char *) string;

	while (*p != '\0') {
		result = ((result << 5) ^ result ) ^ tolower(*p);
		++p;
	}
	
	return result;
}


/*

Copyright (c) 2005-2008, Simon Howard

Permission to use, copy, modify, and/or distribute this software 
for any purpose with or without fee is hereby granted, provided 
that the above copyright notice and this permission notice appear 
in all copies. 

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE 
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR 
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN      
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 

 */

/* Hash table implementation */

#include <stdlib.h>
#include <string.h>


/* malloc() / free() testing */

#ifdef ALLOC_TESTING
#endif

struct _HashTableEntry {
	HashTableKey key;
	HashTableValue value;
	HashTableEntry *next;
};

struct _HashTable {
	HashTableEntry **table;
	int table_size;
	HashTableHashFunc hash_func;
	HashTableEqualFunc equal_func;
	HashTableKeyFreeFunc key_free_func;
	HashTableValueFreeFunc value_free_func;
	int entries;
	int prime_index;
};

/* This is a set of good hash table prime numbers, from:
 *   http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
 * Each prime is roughly double the previous value, and as far as
 * possible from the nearest powers of two. */

static const unsigned int hash_table_primes[] = {
	193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
	196613, 393241, 786433, 1572869, 3145739, 6291469,
	12582917, 25165843, 50331653, 100663319, 201326611,
	402653189, 805306457, 1610612741,
};

static const int hash_table_num_primes 
	= sizeof(hash_table_primes) / sizeof(int);

/* Internal function used to allocate the table on hash table creation
 * and when enlarging the table */

static int hash_table_allocate_table(HashTable *hash_table)
{
	int new_table_size;

	/* Determine the table size based on the current prime index.  
	 * An attempt is made here to ensure sensible behavior if the
	 * maximum prime is exceeded, but in practice other things are
	 * likely to break long before that happens. */

	if (hash_table->prime_index < hash_table_num_primes) {
		new_table_size = hash_table_primes[hash_table->prime_index];
	} else {
		new_table_size = hash_table->entries * 10;
	}

	hash_table->table_size = new_table_size;

	/* Allocate the table and initialise to NULL for all entries */

	hash_table->table = (HashTableEntry**)calloc(hash_table->table_size,
	                           sizeof(HashTableEntry *));

	return hash_table->table != NULL;
}

/* Free an entry, calling the free functions if there are any registered */

static void hash_table_free_entry(HashTable *hash_table, HashTableEntry *entry)
{
	/* If there is a function registered for freeing keys, use it to free
	 * the key */
	
	if (hash_table->key_free_func != NULL) {
		hash_table->key_free_func(entry->key);
	}

	/* Likewise with the value */

	if (hash_table->value_free_func != NULL) {
		hash_table->value_free_func(entry->value);
	}

	/* Free the data structure */
	
	free(entry);
}

HashTable *hash_table_new(HashTableHashFunc hash_func, 
                          HashTableEqualFunc equal_func)
{
	HashTable *hash_table;

	/* Allocate a new hash table structure */
	
	hash_table = (HashTable *) malloc(sizeof(HashTable));

	if (hash_table == NULL) {
		return NULL;
	}
	
	hash_table->hash_func = hash_func;
	hash_table->equal_func = equal_func;
	hash_table->key_free_func = NULL;
	hash_table->value_free_func = NULL;
	hash_table->entries = 0;
	hash_table->prime_index = 0;

	/* Allocate the table */

	if (!hash_table_allocate_table(hash_table)) {
		free(hash_table);

		return NULL;
	}

	return hash_table;
}

void hash_table_free(HashTable *hash_table)
{
	HashTableEntry *rover;
	HashTableEntry *next;
	int i;
	
	/* Free all entries in all chains */

	for (i=0; i<hash_table->table_size; ++i) {
		rover = hash_table->table[i];
		while (rover != NULL) {
			next = rover->next;
			hash_table_free_entry(hash_table, rover);
			rover = next;
		}
	}
	
	/* Free the table */

	free(hash_table->table);
	
	/* Free the hash table structure */

	free(hash_table);
}

void hash_table_register_free_functions(HashTable *hash_table,
                                        HashTableKeyFreeFunc key_free_func,
                                        HashTableValueFreeFunc value_free_func)
{
	hash_table->key_free_func = key_free_func;
	hash_table->value_free_func = value_free_func;
}


static int hash_table_enlarge(HashTable *hash_table)
{
	HashTableEntry **old_table;
	int old_table_size;
	int old_prime_index;
	HashTableEntry *rover;
	HashTableEntry *next;
	int index;
	int i;
	
	/* Store a copy of the old table */
	
	old_table = hash_table->table;
	old_table_size = hash_table->table_size;
	old_prime_index = hash_table->prime_index;

	/* Allocate a new, larger table */

	++hash_table->prime_index;
	
	if (!hash_table_allocate_table(hash_table)) {

		/* Failed to allocate the new table */

		hash_table->table = old_table;
		hash_table->table_size = old_table_size;
		hash_table->prime_index = old_prime_index;

		return 0;
	}

	/* Link all entries from all chains into the new table */

	for (i=0; i<old_table_size; ++i) {
		rover = old_table[i];

		while (rover != NULL) {
			next = rover->next;

			/* Find the index into the new table */
			
			index = hash_table->hash_func(rover->key) % hash_table->table_size;
			
			/* Link this entry into the chain */

			rover->next = hash_table->table[index];
			hash_table->table[index] = rover;
			
			/* Advance to next in the chain */

			rover = next;
		}
	}

	/* Free the old table */

	free(old_table);
       
	return 1;
}

int hash_table_insert(HashTable *hash_table, HashTableKey key, HashTableValue value) 
{
	HashTableEntry *rover;
	HashTableEntry *newentry;
	int index;
	
	/* If there are too many items in the table with respect to the table
	 * size, the number of hash collisions increases and performance
	 * decreases. Enlarge the table size to prevent this happening */

	if ((hash_table->entries * 3) / hash_table->table_size > 0) {
		
		/* Table is more than 1/3 full */

		if (!hash_table_enlarge(hash_table)) {

			/* Failed to enlarge the table */

			return 0;
		}
	}

	/* Generate the hash of the key and hence the index into the table */

	index = hash_table->hash_func(key) % hash_table->table_size;

	/* Traverse the chain at this location and look for an existing
	 * entry with the same key */

	rover = hash_table->table[index];

	while (rover != NULL) {
		if (hash_table->equal_func(rover->key, key) != 0) {

			/* Same key: overwrite this entry with new data */

			/* If there is a value free function, free the old data
			 * before adding in the new data */

			if (hash_table->value_free_func != NULL) {
				hash_table->value_free_func(rover->value);
			}

			/* Same with the key: use the new key value and free 
			 * the old one */

			if (hash_table->key_free_func != NULL) {
				hash_table->key_free_func(rover->key);
			}

			rover->key = key;
			rover->value = value;

			/* Finished */
			
			return 1;
		}
		rover = rover->next;
	}
	
	/* Not in the hash table yet.  Create a new entry */

	newentry = (HashTableEntry *) malloc(sizeof(HashTableEntry));

	if (newentry == NULL) {
		return 0;
	}

	newentry->key = key;
	newentry->value = value;

	/* Link into the list */

	newentry->next = hash_table->table[index];
	hash_table->table[index] = newentry;

	/* Maintain the count of the number of entries */

	++hash_table->entries;

	/* Added successfully */

	return 1;
}

HashTableValue hash_table_lookup(HashTable *hash_table, HashTableKey key)
{
	HashTableEntry *rover;
	int index;

	/* Generate the hash of the key and hence the index into the table */
	
	index = hash_table->hash_func(key) % hash_table->table_size;

	/* Walk the chain at this index until the corresponding entry is
	 * found */

	rover = hash_table->table[index];

	while (rover != NULL) {
		if (hash_table->equal_func(key, rover->key) != 0) {

			/* Found the entry.  Return the data. */

			return rover->value;
		}
		rover = rover->next;
	}

	/* Not found */

	return HASH_TABLE_NULL;
}

int hash_table_remove(HashTable *hash_table, HashTableKey key)
{
	HashTableEntry **rover;
	HashTableEntry *entry;
	int index;
	int result;

	/* Generate the hash of the key and hence the index into the table */
	
	index = hash_table->hash_func(key) % hash_table->table_size;

	/* Rover points at the pointer which points at the current entry
	 * in the chain being inspected.  ie. the entry in the table, or
	 * the "next" pointer of the previous entry in the chain.  This
	 * allows us to unlink the entry when we find it. */

	result = 0;
	rover = &hash_table->table[index];

	while (*rover != NULL) {

		if (hash_table->equal_func(key, (*rover)->key) != 0) {

			/* This is the entry to remove */

			entry = *rover;

			/* Unlink from the list */

			*rover = entry->next;

			/* Destroy the entry structure */

			hash_table_free_entry(hash_table, entry);

			/* Track count of entries */

			--hash_table->entries;

			result = 1;

			break;
		}
		
		/* Advance to the next entry */

		rover = &((*rover)->next);
	}

	return result;
}

int hash_table_num_entries(HashTable *hash_table)
{
	return hash_table->entries;
}

void hash_table_iterate(HashTable *hash_table, HashTableIterator *iterator)
{
	int chain;
	
	iterator->hash_table = hash_table;

	/* Default value of next if no entries are found. */
	
	iterator->next_entry = NULL;
	
	/* Find the first entry */
	
	for (chain=0; chain<hash_table->table_size; ++chain) {
		
		if (hash_table->table[chain] != NULL) {
			iterator->next_entry = hash_table->table[chain];
			iterator->next_chain = chain;
			break;
		}
	}
}

int hash_table_iter_has_more(HashTableIterator *iterator)
{
	return iterator->next_entry != NULL;
}

HashTableValue hash_table_iter_next(HashTableIterator *iterator)
{
	HashTableEntry *current_entry;
	HashTable *hash_table;
	HashTableValue result;
	int chain;

	hash_table = iterator->hash_table;

	/* No more entries? */
	
	if (iterator->next_entry == NULL) {
		return HASH_TABLE_NULL;
	}
	
	/* Result is immediately available */

	current_entry = iterator->next_entry;
	result = current_entry->value;

	/* Find the next entry */

	if (current_entry->next != NULL) {
		
		/* Next entry in current chain */

		iterator->next_entry = current_entry->next;
		
	} else {
	
		/* None left in this chain, so advance to the next chain */

		chain = iterator->next_chain + 1;

		/* Default value if no next chain found */
		
		iterator->next_entry = NULL;

		while (chain < hash_table->table_size) {

			/* Is there anything in this chain? */

			if (hash_table->table[chain] != NULL) {
				iterator->next_entry = hash_table->table[chain];
				break;
			}

			/* Try the next chain */

			++chain;
		}

		iterator->next_chain = chain;
	}

	return result;
}


/*
 * procfuse.c
 *
 */


#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>


#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include <fuse.h>


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
#include <sys/time.h>
#include <stdarg.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <unistd.h>



#define PROCFUSE_DELIMC '/'
#define PROCFUSE_DELIMS "/"

#define PROCFUSE_FNAMELEN 512

typedef enum { T_PROC_POD_NO=0, T_PROC_POD_CHAR, T_PROC_POD_INT, T_PROC_POD_INT64,
	                  T_PROC_POD_FLOAT, T_PROC_POD_DOUBLE, T_PROC_POD_LONGDOUBLE,
	                  T_PROC_POD_STRING, T_PROC_POD_MAX} procfuse_pod_t;

struct procfuse{
	HashTable *root;
	pthread_mutex_t lock;

	int64_t tidcounter;

	int running;
	pthread_t procfuseth;
	struct fuse *fuse;
	pthread_mutex_t fuselock;

	int fuseArgc;
	const char *fuseArgv[9];
	char *fuse_option;
	int fuse_singlethreaded;

	char *absolutemountpoint;
	struct fuse_operations procFS_oper;

	const void *appdata;
	pthread_key_t key_thread_local_storage;
};

struct procfuse_threadlocalstorage{
	struct procfuse_error error;
};

struct procfuse_pod_string{
	int mmapedfd64_r;
    char *mmapedbuffer_r;
    int64_t length_r;

	int mmapedfd64_w;
    char *mmapedbuffer_w;
    int64_t length_w;
};

union procfuse_pod{
	char c;
	int i;
	int64_t l;
	float f;
	double d;
	long double ld;
	struct procfuse_pod_string str;
};

typedef int (*procfuse_onModify)(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata, ...);

struct procfuse_pod_accessor{
	procfuse_onModify onModify; /* modify function corresponding to 'procfuse_pod_t type' */

	union procfuse_pod value;

	procfuse_pod_t type;

	pthread_rwlock_t rwlock;
};

struct procfuse_transactionnode{
	char *writebuffer;
	int length;
	int haswritten;

	int64_t tid; /* key */
};

struct procfuse_hashnode{
	pthread_rwlock_t lock;

	char *absolutepath;

	int pendingforunlink;

    HashTable *subdirs;

    HashTable *transactions;

	struct procfuse_pod_accessor onpodevent;
	struct procfuse_accessor onevent;

	mode_t mode;
	uid_t uid;
    gid_t gid;
    struct timeval created;
	struct timeval access;
	struct timeval modify;

	int flags;  /* one of O_RDONLY,  O_WRONLY,  or  O_RDWR*/

	char *key;

	int concurrent_access_counter;
};


int procfuse_onFuseOpenPOD(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata);
int procfuse_onFuseReadPOD(const struct procfuse *pf, const char *path, char *buffer, size_t size, off_t offset, int64_t tid, const void* appdata);
int procfuse_onFuseWritePOD(const struct procfuse *pf, const char *path, const char *buffer, size_t size, off_t offset, int64_t tid, const void* appdata);
int procfuse_onFuseTruncatePOD(const struct procfuse *pf, const char *path, const off_t off, const void* appdata);
int procfuse_onFuseReleasePOD(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata);

void procfuse_freeHashNode(void *n){
	struct procfuse_hashnode *node = NULL;
	if(n!=NULL) return;
	node = (struct procfuse_hashnode *)n;
	if(node->subdirs!=NULL){
		hash_table_free(node->subdirs);
	}
	if(node->transactions!=NULL){
		hash_table_free(node->transactions);
	}

	pthread_rwlock_destroy(&node->lock);

	switch(node->onpodevent.type){
		case T_PROC_POD_STRING:
			if(node->onpodevent.value.str.mmapedbuffer_r != NULL){
			    munmap(node->onpodevent.value.str.mmapedbuffer_r, node->onpodevent.value.str.length_r);
			}
			if(node->onpodevent.value.str.mmapedfd64_r >= 0){
			    close(node->onpodevent.value.str.mmapedfd64_r);
			}

			if(node->onpodevent.value.str.mmapedbuffer_w != NULL){
			    munmap(node->onpodevent.value.str.mmapedbuffer_w, node->onpodevent.value.str.length_w);
			}
			if(node->onpodevent.value.str.mmapedfd64_w >= 0){
			    close(node->onpodevent.value.str.mmapedfd64_w);
			}
			break;
		default:
			break;
	}

	free(node->key);
	free(node->absolutepath);

	free(node);
}
void procfuse_freeTransactionNode(void *n){
	struct procfuse_transactionnode *tnode = NULL;

	if(n!=NULL) return;
	tnode = (struct procfuse_transactionnode *)n;

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
	*ht=NULL;
	return 1;
}


struct procfuse* procfuse_ctor(const char *filesystemname, const char *mountpoint, const char *fuse_option,const void *appdata){
	struct procfuse *pf = NULL;
	char *absolutemountpoint = NULL;
	DIR *d = NULL;
	int er = 0, len = 0;
	const char *fusermountcmd = "fusermount -u ";
	char *unmountcmd = NULL;

	if(filesystemname==NULL || mountpoint==NULL){
		errno = EINVAL;
		return NULL;
	}

	errno = 0;
	pf = (struct procfuse *)calloc(1, sizeof(struct procfuse));
	absolutemountpoint = realpath(mountpoint, NULL);
	if(pf==NULL || absolutemountpoint==NULL){
		if(pf!=NULL) free(pf);
		if(absolutemountpoint!=NULL) free(absolutemountpoint);

		errno = ENOMEM;
		return NULL;
	}
	memset(pf, '\0', sizeof(struct procfuse));
	pthread_mutex_init(&pf->lock, NULL);
	pthread_mutex_init(&pf->fuselock, NULL);

	if(pthread_key_create(&pf->key_thread_local_storage, free)!=0){
		free(pf);
		free(absolutemountpoint);
		return NULL;
	}

	if(procfuse_ctorht(&pf->root, PROCFUSE_YES)==0){
		free(pf);
		free(absolutemountpoint);
		return NULL;
	}

    pf->fuseArgv[0] = strdup(filesystemname);
    pf->fuseArgv[1] = absolutemountpoint;
    pf->absolutemountpoint = absolutemountpoint;

	if(fuse_option!=NULL && strlen(fuse_option)>0){
	    pf->fuse_option = strdup(fuse_option);
	}

	pf->fuse_singlethreaded = 0;
	pf->running = 0;

	d = NULL;
	if((d=opendir(pf->absolutemountpoint))==NULL){
		if(errno==ENOTCONN){

			er = umount2(pf->absolutemountpoint, MNT_DETACH);
			if(er==-1){
				len = strlen(pf->absolutemountpoint)+strlen(fusermountcmd)+1;
				unmountcmd = (char*)calloc(sizeof(char), len);
				if(unmountcmd!=NULL){
				    snprintf(unmountcmd, len, "%s%s", fusermountcmd, pf->absolutemountpoint);
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
	struct timespec ts;
	if(pf==NULL || pf->root==NULL){
		return;
	}

	/* waiting for the thread to exit */
	do{
		procfuse_teardown(pf);
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 100000000; /* 100 milliseconds */
	}while(pthread_timedjoin_np(pf->procfuseth, NULL, &ts)!=0);
    pthread_join(pf->procfuseth, NULL);

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

struct procfuse_threadlocalstorage* procfuse_getThreadLocalStorage(struct procfuse *pf){
	struct procfuse_threadlocalstorage *tls = NULL;

	if(pf==NULL){
		errno = EINVAL;
		return NULL;
	}

    if ((tls = (struct procfuse_threadlocalstorage *)pthread_getspecific(pf->key_thread_local_storage)) == NULL) {
    	tls = (struct procfuse_threadlocalstorage *)calloc(1, sizeof(struct procfuse_threadlocalstorage));
    	if(tls==NULL){
    		errno = ENOMEM;
    		return NULL;
    	}

        pthread_setspecific(pf->key_thread_local_storage, tls);
    }

    return tls;
}

struct procfuse_error* procfuse_error(struct procfuse *pf){
	struct procfuse_threadlocalstorage *tls = procfuse_getThreadLocalStorage(pf);
	if(tls==NULL){
		return NULL;
	}

	return &tls->error;
}

void procfuse_setErrno(struct procfuse *pf, int errn){
	struct procfuse_threadlocalstorage *tls = procfuse_getThreadLocalStorage(pf);
	if(tls==NULL){
		return;
	}

	tls->error.errn = errn;
}
void procfuse_setError(struct procfuse *pf, const char *fmt, ...){
	int size = 0;
	va_list ap;
	struct procfuse_threadlocalstorage *tls = procfuse_getThreadLocalStorage(pf);
	if(tls==NULL){
		return;
	}
	if(fmt==NULL){
		errno = EINVAL;
		return;
	}

	/* determine size of final string */
	va_start(ap, fmt);
	size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if(tls->error.error!=NULL){
    	free(tls->error.error);
    }

    /* finally allocate and snprintf final string */
   	tls->error.error = (char*)calloc(size+1, sizeof(char));
	va_start(ap, fmt);
	size = vsnprintf(tls->error.error, size, fmt, ap);
    va_end(ap);
}
void procfuse_setSolution(struct procfuse *pf, const char *fmt, ...){
	int size = 0;
	va_list ap;
	struct procfuse_threadlocalstorage *tls = procfuse_getThreadLocalStorage(pf);
	if(tls==NULL){
		return;
	}
	if(fmt==NULL){
		errno = EINVAL;
		return;
	}

	/* determine size of final string */
	va_start(ap, fmt);
	size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if(tls->error.solution!=NULL){
    	free(tls->error.solution);
    }

    /* finally allocate and snprintf final string */
   	tls->error.solution = (char*)calloc(size+1, sizeof(char));
	va_start(ap, fmt);
	size = vsnprintf(tls->error.solution, size, fmt, ap);
    va_end(ap);
}


void procfuse_printTree(HashTable *htable){
	struct procfuse_hashnode *value = NULL;
	HashTableIterator iterator;
	hash_table_iterate(htable, &iterator);

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
	const char *eow = NULL;
	if(what==NULL){ return NULL; }

	eow = what+strlen(what);
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
	int pathlen = 0, flen = 0, hassubpath = 0;
	struct procfuse_hashnode *node = NULL;
	const char *eoap = NULL;
	char fname[PROCFUSE_FNAMELEN] = {'\0'};

	if(root==NULL || absolutepath==NULL){
		errno = EINVAL;
		return NULL;
	}

	absolutepath = procfuse_ltrim(absolutepath, PROCFUSE_DELIMC);
	eoap = procfuse_rtrim(absolutepath, PROCFUSE_DELIMC);
	eoap++; /* reposition at delim character (or null byte) to have correct pointer arithmetic */

	if(!procfuse_getNextFileName(absolutepath, fname)){
		return NULL;
	}

	pathlen = eoap-absolutepath;
	flen = strlen(fname);
	hassubpath = (pathlen==flen) ? PROCFUSE_NO : PROCFUSE_YES ;

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
	int pathlen = 0, flen = 0, hassubpath = 0;
	const char *eoap = NULL;
	struct procfuse_hashnode *node = NULL;
	char fname[PROCFUSE_FNAMELEN] = {'\0'};

	if(root==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	absolutepath = procfuse_ltrim(absolutepath, PROCFUSE_DELIMC);
	eoap = procfuse_rtrim(absolutepath, PROCFUSE_DELIMC);
	eoap++; /* reposition at delim character (or null byte) to have correct pointer arithmetic */


	if(!procfuse_getNextFileName(absolutepath, fname)){
		return 0;
	}

	pathlen = eoap-absolutepath;
	flen = strlen(fname);
	hassubpath = (pathlen==flen) ? 0 : 1 ;

	/* if
	 *    node exists AND is end node AND absolutepath is filename => update access
	 *    node exists AND is root node AND absolutepath is root => recursive unregisterNode
	 *    node not exists => alloc node as neeeded
	 */
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

int procfuse_openTempFile(const char *pfx, int deleteOnOpen){
	int descriptor = -1;
    char tmpfiletemplate[8192];
    int iteration=0, maxiteration = 64*1024; /* max 2^16 iterations */
    static const char *tmpdir = NULL;

	memset(tmpfiletemplate, '\0', sizeof(tmpfiletemplate));

	/* i'm not using a mutex here because the pointer values are very strict and since it's only written once here
	 * even a race condition should result in the same tmpdir value
	 */
	if(tmpdir==NULL){
	    tmpdir = getenv("TMPDIR");
	}
	if(tmpdir==NULL && P_tmpdir!=NULL){
		tmpdir = P_tmpdir;
	}
	if(tmpdir==NULL){
		tmpdir = "/tmp";
	}

	if(pfx==NULL){
		pfx = "";
	}

	do{
		iteration++;

	    snprintf(tmpfiletemplate, sizeof(tmpfiletemplate)-1, "%s/procfuse.%d.%s.tmp.%d%d", tmpdir, getpid(), pfx, rand(),rand());
	    descriptor = open(tmpfiletemplate, O_RDWR | O_CREAT | O_EXCL | O_LARGEFILE, S_IRUSR|S_IWUSR);

        if (descriptor >= 0){
        	if(deleteOnOpen == PROCFUSE_YES){
        		unlink(tmpfiletemplate);
        	}
	        break;
	    }
        else if(errno != EEXIST){
        	break;
        }
	}while(iteration<maxiteration);

	return descriptor;
}


int procfuse_create(struct procfuse *pf, const char *absolutepath, struct procfuse_accessor access){
	int rval = 0, flags = 0;
	struct procfuse_hashnode *node = NULL;

	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	pthread_mutex_lock(&pf->lock);

	node = procfuse_pathToNode(pf->root, absolutepath, PROCFUSE_YES);
	if(node!=NULL){
		node->absolutepath = strdup(absolutepath);
		if(node->absolutepath==NULL){
			procfuse_unregisterNodeInternal(pf->root, absolutepath);
		}
		else{
			memcpy(&node->onevent, &access, sizeof(access));
			node->onpodevent.type = T_PROC_POD_NO;
			flags = 0;
			if(access.onFuseRead!=NULL && access.onFuseWrite!=NULL){
				flags = O_RDWR;
			}
			else if(access.onFuseRead!=NULL){
				flags = O_RDONLY;
			}
			else if(access.onFuseWrite!=NULL){
				flags = O_WRONLY;
			}
			node->flags = flags;

			pthread_rwlock_init(&node->lock, NULL);
			node->concurrent_access_counter = 0;
			node->pendingforunlink = PROCFUSE_NO;

			gettimeofday(&node->created, NULL);
			rval = 1;
		}
	}

	pthread_mutex_unlock(&pf->lock);

	return rval;
}

int procfuse_createPOD(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify onModify, procfuse_pod_t pod_type){
	int rval = 0;
	struct procfuse_hashnode *node = NULL;
	struct procfuse_accessor access;
	struct procfuse_pod_accessor podaccess;

	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}
	if(pod_type<=T_PROC_POD_NO || pod_type>=T_PROC_POD_MAX){
		errno = EINVAL;
		return 0;
	}

	memset(&podaccess, '\0', sizeof(podaccess));

	podaccess.onModify = onModify;
	podaccess.type = pod_type;

	if(pthread_rwlock_init(&podaccess.rwlock, NULL)!=0){
		return 0;
	}

	pthread_mutex_lock(&pf->lock);

	node = procfuse_pathToNode(pf->root, absolutepath, PROCFUSE_YES);
	if(node!=NULL){
		node->absolutepath = strdup(absolutepath);
		if(node->absolutepath!=NULL){
			memset(&access, '\0', sizeof(access));

			access.onFuseOpen = procfuse_onFuseOpenPOD;
			access.onFuseTruncate = procfuse_onFuseTruncatePOD;
			access.onFuseRead = procfuse_onFuseReadPOD;
			access.onFuseWrite = procfuse_onFuseWritePOD;
			access.onFuseRelease = procfuse_onFuseReleasePOD;

			memcpy(&node->onevent, &access, sizeof(access));
			memcpy(&node->onpodevent, &podaccess, sizeof(podaccess));
			node->flags = flags;

			pthread_rwlock_init(&node->lock, NULL);
			node->concurrent_access_counter = 0;
			node->pendingforunlink = PROCFUSE_NO;

			gettimeofday(&node->created, NULL);

			rval = 1;
			if(node->onpodevent.type == T_PROC_POD_STRING){
				node->onpodevent.value.str.mmapedfd64_r = node->onpodevent.value.str.mmapedfd64_w = -1;
				node->onpodevent.value.str.mmapedfd64_r = procfuse_openTempFile("podstring", PROCFUSE_YES);

				node->onpodevent.value.str.length_r = node->onpodevent.value.str.length_w = 4*1024*1024;
				if(node->onpodevent.value.str.mmapedfd64_r==-1 ||
				   ftruncate(node->onpodevent.value.str.mmapedfd64_r, node->onpodevent.value.str.length_r)==-1 ||
				   (node->onpodevent.value.str.mmapedbuffer_r = (char*)mmap(NULL, node->onpodevent.value.str.length_r, PROT_READ | PROT_WRITE,
																			MAP_SHARED, node->onpodevent.value.str.mmapedfd64_r, 0))==NULL){
					if(node->onpodevent.value.str.mmapedfd64_r!=-1){
						close(node->onpodevent.value.str.mmapedfd64_r);
					}

					rval = 0;
				}

			}
			if(node->onpodevent.type!=T_PROC_POD_CHAR && node->onpodevent.type!=T_PROC_POD_STRING &&
			   procfuse_ctorht(&node->transactions, PROCFUSE_NO)==0){
				rval = 0;
			}
		}

		if(rval==0)
			procfuse_unregisterNodeInternal(pf->root, absolutepath); /* clean up unneeded tree structures */
	}

	pthread_mutex_unlock(&pf->lock);

	return rval;
}

int procfuse_createPOD_c(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_c onModify){
	return procfuse_createPOD(pf, absolutepath, flags, (procfuse_onModify)onModify, T_PROC_POD_CHAR);
}
int procfuse_createPOD_i(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_i onModify){
	return procfuse_createPOD(pf, absolutepath, flags, (procfuse_onModify)onModify, T_PROC_POD_INT);
}
int procfuse_createPOD_i64(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_i64 onModify){
	return procfuse_createPOD(pf, absolutepath, flags, (procfuse_onModify)onModify, T_PROC_POD_INT64);
}
int procfuse_createPOD_f(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_f onModify){
	return procfuse_createPOD(pf, absolutepath, flags, (procfuse_onModify)onModify, T_PROC_POD_FLOAT);
}
int procfuse_createPOD_d(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_d onModify){
	return procfuse_createPOD(pf, absolutepath, flags, (procfuse_onModify)onModify, T_PROC_POD_DOUBLE);
}
int procfuse_createPOD_ld(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_ld onModify){
	return procfuse_createPOD(pf, absolutepath, flags, (procfuse_onModify)onModify, T_PROC_POD_LONGDOUBLE);
}
int procfuse_createPOD_s(struct procfuse *pf, const char *absolutepath, int flags, procfuse_onModify_s onModify){
	return procfuse_createPOD(pf, absolutepath, flags, (procfuse_onModify)onModify, T_PROC_POD_STRING);
}

int procfuse_unlink(struct procfuse *pf, const char *absolutepath){
	int rval = 0;
	int unlinknode = 0;
	struct procfuse_hashnode *node = NULL;

	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	pthread_mutex_lock(&pf->lock);

	node = procfuse_pathToNode(pf->root, absolutepath, PROCFUSE_NO);
	if(node!=NULL){
		pthread_rwlock_wrlock(&node->lock);

		node->pendingforunlink = PROCFUSE_YES;

		if(node->concurrent_access_counter<=0){
			unlinknode = 1;
		}

		pthread_rwlock_unlock(&node->lock);
	}

	if(unlinknode){
	    rval = procfuse_unregisterNodeInternal(pf->root, absolutepath);
	}

	pthread_mutex_unlock(&pf->lock);

	return rval;
}

struct procfuse_hashnode* procfuse_acquireAccessToNode(struct procfuse *pf, const char *absolutepath){
	struct procfuse_hashnode *node = NULL;

	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return NULL;
	}

	/* acquire outer global filesystem lock */
	pthread_mutex_lock(&pf->lock);

	/* search node */
	node = procfuse_pathToNode(pf->root, absolutepath, PROCFUSE_NO);
	if(node!=NULL){
		/* acquire inner node lock */
		pthread_rwlock_wrlock(&node->lock);
		/* and increase access counter */
		node->concurrent_access_counter++;
		/* release inner node lock */
		pthread_rwlock_unlock(&node->lock);

		/* acquire read only lock!! and leave it that */
		pthread_rwlock_rdlock(&node->lock);
	}
	/* realease outer lock */
	pthread_mutex_unlock(&pf->lock);

	return node;
}

struct procfuse_hashnode* procfuse_upgradeNodeReadLockToWriteLock(struct procfuse_hashnode *node){
	if(node==NULL){
		errno = -EINVAL;
		return NULL;
	}

	pthread_rwlock_unlock(&node->lock);
	pthread_rwlock_wrlock(&node->lock);

	return node;
}
struct procfuse_hashnode* procfuse_downgradeNodeWriteLockToReadLock(struct procfuse_hashnode *node){
	if(node==NULL){
		errno = -EINVAL;
		return NULL;
	}

	pthread_rwlock_unlock(&node->lock);
	pthread_rwlock_rdlock(&node->lock);

	return node;
}

void procfuse_releaseAccessToNode(struct procfuse *pf, struct procfuse_hashnode *node){
	int unlinknode = PROCFUSE_NO;

	if(pf==NULL || node==NULL){
		errno = EINVAL;
		return;
	}

	/* first relase the read only lock!! */
	pthread_rwlock_unlock(&node->lock);

	/* acquire outer global filesystem lock */
	pthread_mutex_lock(&pf->lock);

	/* search node */
	/* acquire inner node lock */
	pthread_rwlock_wrlock(&node->lock);
	/* and decrease access counter */
	node->concurrent_access_counter--;

	if(node->concurrent_access_counter<=0 &&
	   node->pendingforunlink==PROCFUSE_YES &&
	   node->transactions!=NULL && hash_table_num_entries(node->transactions)<=0){
		unlinknode = PROCFUSE_YES;
	}

	/* release inner node lock */
	pthread_rwlock_unlock(&node->lock);

	if(unlinknode==PROCFUSE_YES){
		/* why is it safe to unlink the node from the hashmap here?
		 * as the decision of unlinknode=PROCFUSE_YES is done a few lines above
		 * a lock is held on pf->lock AND exclusive lock on node->lock
		 * at that time, no one else has access to the node object, not because the lock was held!
		 * but because concurrent_access_counter<=0 and no "procfuse file system user" is accessing
		 * this file cause of hash_table_num_entries <= 0
		 *
		 * thus we could be sure that we're the only ones having absolute exclusive access to the node
		 *
		 * if by the time of releasing the lock above and unlinking in the next line would have acquired the lock
		 * e.g. cause of calling procfuse_upgradeNodeReadLockToWriteLock this upgrade would count as concurrent_access_counter
		 * and thus unlinknode wouldn't have been set
		 *
		 * so every "new" access would have been blocked and concurrent access wouldn't lead to the next line unlinking the node
		 */
		procfuse_unregisterNodeInternal(pf->root, node->absolutepath);
	}

	/* realease outer lock */
	pthread_mutex_unlock(&pf->lock);
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

int procfuse_copyPOD(procfuse_pod_t dst_type, union procfuse_pod *dstpod, procfuse_pod_t src_type, union procfuse_pod *srcpod){
	int rval = 0, printed = 0;
	switch(src_type){
	    case T_PROC_POD_CHAR:
	    	switch(dst_type){
				case T_PROC_POD_CHAR:
					dstpod->c = srcpod->c;
					rval = 1;
					break;
				case T_PROC_POD_INT:
					dstpod->i = (int)srcpod->c;
					rval = 1;
					break;
	    	    case T_PROC_POD_INT64:
	    	    	dstpod->l = (int64_t)srcpod->c;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_FLOAT:
	    	    	dstpod->f = (float)srcpod->c;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_DOUBLE:
	    	    	dstpod->d = (double)srcpod->c;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_LONGDOUBLE:
	    	    	dstpod->ld = (long double)srcpod->c;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_STRING:
					if(dstpod->str.length_r>=1){
						dstpod->str.mmapedbuffer_r[0] = srcpod->c;
						dstpod->str.length_r = 1;
						rval = 1;
					}
	    	    	break;
	    	    default: break;
	    	}
	    	break;

	    case T_PROC_POD_INT:
	    	switch(dst_type){
				case T_PROC_POD_INT:
					dstpod->i = srcpod->i;
					rval = 1;
					break;
	    	    case T_PROC_POD_INT64:
	    	    	dstpod->l = srcpod->i;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_FLOAT:
	    	    	dstpod->f = (float)srcpod->i;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_DOUBLE:
	    	    	dstpod->d = srcpod->i;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_LONGDOUBLE:
	    	    	dstpod->ld = srcpod->i;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_STRING:
	    	    	printed = snprintf(dstpod->str.mmapedbuffer_r,
	    	    			           dstpod->str.length_r, "%d", srcpod->i);
	    	    	/* if successfully written */
	    	    	if(printed>0 && printed<dstpod->str.length_r){
	    	    		dstpod->str.length_r = printed;
	    	    		rval = 1;
	    	    	}
	    	    	else{
	    	    		rval = printed;
	    	    	}
	    	    	break;
	    	    default: break;
	    	}
	    	break;

	    case T_PROC_POD_INT64:
	    	switch(dst_type){
	    	    case T_PROC_POD_INT64:
	    	    	dstpod->l = srcpod->l;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_FLOAT:
	    	    	if(sizeof(dstpod->f)>=sizeof(srcpod->l)){
						dstpod->f = (float)srcpod->l;
						rval = 1;
	    	    	}
	    	    	break;
	    	    case T_PROC_POD_DOUBLE:
					dstpod->d = srcpod->l;
					rval = 1;
	    	    	break;
	    	    case T_PROC_POD_LONGDOUBLE:
	    	    	dstpod->ld = srcpod->l;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_STRING:
	    	    	printed = snprintf(dstpod->str.mmapedbuffer_r,
	    	    			           dstpod->str.length_r, "%"PRId64, srcpod->l);
	    	    	/* if successfully written */
	    	    	if(printed>0 && printed<dstpod->str.length_r){
	    	    		dstpod->str.length_r = printed;
	    	    		rval = 1;
	    	    	}
	    	    	else{
	    	    		rval = printed;
	    	    	}
	    	    	break;
	    	    default: break;
	    	}
	    	break;

		case T_PROC_POD_FLOAT:
	    	switch(dst_type){
				case T_PROC_POD_FLOAT:
					dstpod->f = srcpod->f;
					rval = 1;
					break;
	    	    case T_PROC_POD_DOUBLE:
	    	    	dstpod->d = srcpod->f;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_LONGDOUBLE:
	    	    	dstpod->ld = srcpod->f;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_STRING:
	    	    	printed = snprintf(dstpod->str.mmapedbuffer_r,
	    	    			           dstpod->str.length_r, "%g", srcpod->f);
	    	    	/* if successfully written */
	    	    	if(printed>0 && printed<dstpod->str.length_r){
	    	    		dstpod->str.length_r = printed;
	    	    		rval = 1;
	    	    	}
	    	    	else{
	    	    		rval = printed;
	    	    	}
	    	    	break;
	    	    default: break;
	    	}
			break;

	    case T_PROC_POD_DOUBLE:
	    	switch(dst_type){
	    	    case T_PROC_POD_DOUBLE:
	    	    	dstpod->d = srcpod->d;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_LONGDOUBLE:
	    	    	dstpod->ld = srcpod->d;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_STRING:
	    	    	printed = snprintf(dstpod->str.mmapedbuffer_r,
	    	    			           dstpod->str.length_r, "%g", srcpod->d);
	    	    	/* if successfully written */
	    	    	if(printed>0 && printed<dstpod->str.length_r){
	    	    		dstpod->str.length_r = printed;
	    	    		rval = 1;
	    	    	}
	    	    	else{
	    	    		rval = printed;
	    	    	}
	    	    	break;
	    	    default: break;
	    	}
	    	break;

	    case T_PROC_POD_LONGDOUBLE:
	    	switch(dst_type){
	    	    case T_PROC_POD_LONGDOUBLE:
	    	    	dstpod->ld = srcpod->ld;
	    	    	rval = 1;
	    	    	break;
	    	    case T_PROC_POD_STRING:
	    	    	printed = snprintf(dstpod->str.mmapedbuffer_r,
	    	    			           dstpod->str.length_r, "%Le", srcpod->ld);
	    	    	/* if successfully written */
	    	    	if(printed>0 && printed<dstpod->str.length_r){
	    	    		dstpod->str.length_r = printed;
	    	    		rval = 1;
	    	    	}
	    	    	else{
	    	    		rval = printed;
	    	    	}
	    	    	break;
	    	    default: break;
	    	}
	    	break;

	    case T_PROC_POD_STRING:
	    	if(dst_type==T_PROC_POD_CHAR && srcpod->str.length_w==1){
	    		dstpod->c = srcpod->str.mmapedbuffer_w[0];
	    		rval = 1;
	    	}
	    	else if(dst_type==T_PROC_POD_STRING && dstpod->str.length_r >= srcpod->str.length_w){
	    		memcpy(dstpod->str.mmapedbuffer_r, srcpod->str.mmapedbuffer_w, srcpod->str.length_w);
	    		dstpod->str.length_r = srcpod->str.length_w;
	    		rval = 1;
	    	}
	    	break;
	    default:
		    /* error */
	    	break;
	}

	return rval;
}

int procfuse_readPOD(struct procfuse *pf, const char *absolutepath, procfuse_pod_t pod_type, void *buffer){
	int rval = 0;
	struct procfuse_hashnode *node = NULL;

	if(pf==NULL || absolutepath==NULL || buffer==NULL){
		errno = EINVAL;
		return 0;
	}

	node = procfuse_acquireAccessToNode(pf, absolutepath);
	if(node!=NULL){
	    rval = procfuse_copyPOD(pod_type, (union procfuse_pod *)buffer, node->onpodevent.type, &node->onpodevent.value);
	}
	procfuse_releaseAccessToNode(pf, node);

	return rval;
}

int procfuse_readPOD_c(struct procfuse *pf, const char *absolutepath, char *value){
	int rval = 0;
	union procfuse_pod buffer;

	rval = procfuse_readPOD(pf, absolutepath, T_PROC_POD_CHAR, &buffer);
	if(rval!=0){
		*value = buffer.c;
	}
	return rval;
}
int procfuse_readPOD_i(struct procfuse *pf, const char *absolutepath, int *value){
	int rval = 0;
	union procfuse_pod buffer;

	rval = procfuse_readPOD(pf, absolutepath, T_PROC_POD_INT, &buffer);
	if(rval!=0){
		*value = buffer.i;
	}
	return rval;
}
int procfuse_readPOD_i64(struct procfuse *pf, const char *absolutepath, int64_t *value){
	int rval = 0;
	union procfuse_pod buffer;

	rval = procfuse_readPOD(pf, absolutepath, T_PROC_POD_INT64, &buffer);
	if(rval!=0){
		*value = buffer.l;
	}
	return rval;
}
int procfuse_readPOD_f(struct procfuse *pf, const char *absolutepath, float *value){
	int rval = 0;
	union procfuse_pod buffer;

	rval = procfuse_readPOD(pf, absolutepath, T_PROC_POD_FLOAT, &buffer);
	if(rval!=0){
		*value = buffer.f;
	}
	return rval;
}
int procfuse_readPOD_d(struct procfuse *pf, const char *absolutepath, double *value){
	int rval = 0;
	union procfuse_pod buffer;

	rval = procfuse_readPOD(pf, absolutepath, T_PROC_POD_DOUBLE, &buffer);
	if(rval!=0){
		*value = buffer.d;
	}
	return rval;
}
int procfuse_readPOD_ld(struct procfuse *pf, const char *absolutepath, long double *value){
	int rval = 0;
	union procfuse_pod buffer;

	rval = procfuse_readPOD(pf, absolutepath, T_PROC_POD_LONGDOUBLE, &buffer);
	if(rval!=0){
		*value = buffer.ld;
	}
	return rval;
}
int procfuse_readPOD_s(struct procfuse *pf, const char *absolutepath, char *value, int64_t *length){
	int rval = 0;
	union procfuse_pod buffer;

	buffer.str.length_w = *length;
	buffer.str.mmapedbuffer_w = value;

	rval = procfuse_readPOD(pf, absolutepath, T_PROC_POD_INT64, &buffer);
	return rval;
}

int procfuse_writePOD(struct procfuse *pf, const char *absolutepath, procfuse_pod_t pod_type, void *buffer){
	int rval = 0;
	struct procfuse_hashnode *node = NULL;

	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	node = procfuse_acquireAccessToNode(pf, absolutepath);
	if(node!=NULL){
		rval = procfuse_copyPOD(node->onpodevent.type, &node->onpodevent.value, pod_type, (union procfuse_pod *)buffer);
	}
	procfuse_releaseAccessToNode(pf, node);

	return rval;
}


int procfuse_writePOD_c(struct procfuse *pf, const char *absolutepath, char value){
	union procfuse_pod buffer;

	buffer.c = value;
	return procfuse_readPOD(pf, absolutepath, T_PROC_POD_CHAR, &buffer);
}
int procfuse_writePOD_i(struct procfuse *pf, const char *absolutepath, int value){
	union procfuse_pod buffer;

	buffer.i = value;
	return procfuse_readPOD(pf, absolutepath, T_PROC_POD_INT, &buffer);
}
int procfuse_writePOD_i64(struct procfuse *pf, const char *absolutepath, int64_t value){
	union procfuse_pod buffer;

	buffer.ld = value;
	return procfuse_readPOD(pf, absolutepath, T_PROC_POD_INT64, &buffer);
}
int procfuse_writePOD_f(struct procfuse *pf, const char *absolutepath, float value){
	union procfuse_pod buffer;

	buffer.f = value;
	return procfuse_readPOD(pf, absolutepath, T_PROC_POD_FLOAT, &buffer);
}
int procfuse_writePOD_d(struct procfuse *pf, const char *absolutepath, double value){
	union procfuse_pod buffer;

	buffer.d = value;
	return procfuse_readPOD(pf, absolutepath, T_PROC_POD_DOUBLE, &buffer);
}
int procfuse_writePOD_ld(struct procfuse *pf, const char *absolutepath, long double value){
	union procfuse_pod buffer;

	buffer.ld = value;
	return procfuse_readPOD(pf, absolutepath, T_PROC_POD_LONGDOUBLE, &buffer);
}
int procfuse_writePOD_s(struct procfuse *pf, const char *absolutepath, char *value, int64_t length){
	union procfuse_pod buffer;

	buffer.str.length_w = length;
	buffer.str.mmapedbuffer_w = value;
	return procfuse_readPOD(pf, absolutepath, T_PROC_POD_STRING, &buffer);
}

int procfuse_isPOD(struct procfuse *pf, const char *absolutepath){
	struct procfuse_hashnode *node = NULL;
	int rval = PROCFUSE_NO;
	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	node = procfuse_acquireAccessToNode(pf, absolutepath);
	if(node!=NULL && node->onpodevent.type>T_PROC_POD_NO && node->onpodevent.type<T_PROC_POD_MAX){
		rval = PROCFUSE_YES;
	}
	procfuse_releaseAccessToNode(pf, node);

	return rval;
}

int procfuse_exists(struct procfuse *pf, const char *absolutepath){
	struct procfuse_hashnode *node = NULL;
	int rval = PROCFUSE_NO;
	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}


	node = procfuse_acquireAccessToNode(pf, absolutepath);
	if(node!=NULL){
		rval = PROCFUSE_YES;
	}
	procfuse_releaseAccessToNode(pf, node);

	return rval;
}
int procfuse_chmod(struct procfuse *pf, const char *absolutepath, mode_t mode){
	struct procfuse_hashnode *node = NULL;
	int rval = PROCFUSE_NO;
	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	node = procfuse_acquireAccessToNode(pf, absolutepath);
	if(node!=NULL){
		node->mode = mode;
		rval = PROCFUSE_YES;
	}
	procfuse_releaseAccessToNode(pf, node);

	return rval;
}
int procfuse_chown(struct procfuse *pf, const char *absolutepath, uid_t owner, gid_t group){
	struct procfuse_hashnode *node = NULL;
	int rval = PROCFUSE_NO;
	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	node = procfuse_acquireAccessToNode(pf, absolutepath);
	if(node!=NULL){
		node->uid = owner;
		node->gid = group;
		rval = PROCFUSE_YES;
	}
	procfuse_releaseAccessToNode(pf, node);

	return rval;
}
int procfuse_utime(struct procfuse *pf, const char *absolutepath, const struct utimbuf *times){
	struct procfuse_hashnode *node = NULL;
	int rval = PROCFUSE_NO;
	if(pf==NULL || absolutepath==NULL || times==NULL){
		errno = EINVAL;
		return 0;
	}

	node = procfuse_acquireAccessToNode(pf, absolutepath);
	if(node!=NULL){
		node->access.tv_sec = times->actime;
		node->access.tv_usec = 0;
		node->modify.tv_sec = times->modtime;
		node->modify.tv_usec = 0;
		rval = PROCFUSE_YES;
	}
	procfuse_releaseAccessToNode(pf, node);

	return rval;
}
int procfuse_utimes(struct procfuse *pf, const char *absolutepath, const struct timeval times[2]){
	struct procfuse_hashnode *node = NULL;
	int rval = PROCFUSE_NO;
	if(pf==NULL || absolutepath==NULL){
		errno = EINVAL;
		return 0;
	}

	node = procfuse_acquireAccessToNode(pf, absolutepath);
	if(node!=NULL){
		node->access = times[0];
		node->modify = times[1];
		rval = PROCFUSE_YES;
	}
	procfuse_releaseAccessToNode(pf, node);

	return rval;
}

int procfuse_onFuseOpenPOD(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata){
	int rval = 0;
	struct procfuse_transactionnode *tnode = NULL;
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	(void)(pf);
	(void)(path);

	/* transactions not needed for read only files */
	if((node->flags & O_RDONLY) == O_RDONLY ||
	   node->onpodevent.type==T_PROC_POD_STRING || node->onpodevent.type==T_PROC_POD_CHAR)
		return rval;

	tnode = (struct procfuse_transactionnode *)calloc(sizeof(struct procfuse_transactionnode), 1);
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

	procfuse_upgradeNodeReadLockToWriteLock(node);

	if(hash_table_insert(node->transactions, &tnode->tid, tnode)==0){
		free(tnode->writebuffer);
		free(tnode);

		tnode = NULL;
		rval = -ENOMEM;
	}

	procfuse_downgradeNodeWriteLockToReadLock(node);

    return rval;
}
int procfuse_onFuseReadPOD(const struct procfuse *pf, const char *path, char *buffer, size_t size, off_t offset, int64_t tid, const void* appdata){
	int rval = 0, printed = 0;
	off_t where = 0;
	size_t cpylen = 0;
	char podtmp[8192] = {'\0'}; /* more than long enough for pod datatypes - 80bit long double range is 3.65×10^−4951 to 1.18×10^4932  */
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	(void)pf;
	(void)path;
	(void)tid;
	(void)size;

	if(!((node->flags & O_RDWR)==O_RDWR || (node->flags & O_RDONLY)==O_RDONLY)){
		return 0;
	}

	printed = 0;

	procfuse_upgradeNodeReadLockToWriteLock(node);

	switch(node->onpodevent.type){
		case T_PROC_POD_CHAR:
			if(offset==0){
			    buffer[0] = node->onpodevent.value.c;
			    rval = 1;
			}
			else
				rval = 0;
			break;
		case T_PROC_POD_INT:
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%d", node->onpodevent.value.i);
			break;
		case T_PROC_POD_INT64:
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%"PRId64, node->onpodevent.value.l);
			break;
		case T_PROC_POD_FLOAT:
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%g", (double)node->onpodevent.value.f);
			break;
		case T_PROC_POD_DOUBLE:
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%g", node->onpodevent.value.d);
			break;
		case T_PROC_POD_LONGDOUBLE:
			printed = snprintf(podtmp, sizeof(podtmp)-1, "%Le", node->onpodevent.value.ld);
			break;
		case T_PROC_POD_STRING:
			if(node->onpodevent.value.str.length_r==0 || node->onpodevent.value.str.mmapedbuffer_r==NULL){
				rval = 0;
				break;
			}

			where = offset;
			cpylen = size;
			if(where>node->onpodevent.value.str.length_r){
				rval = 0;
				break;
			}
			if(cpylen>(size_t)((node->onpodevent.value.str.length_r)-where)){
				cpylen = (node->onpodevent.value.str.length_r)-where;
			}
			memcpy(buffer, (node->onpodevent.value.str.mmapedbuffer_r)+where, cpylen);
			rval = cpylen;
			break;
		default: break;
	}
	if(node->onpodevent.type!=T_PROC_POD_STRING && node->onpodevent.type!=T_PROC_POD_CHAR){
	    if(offset>printed){
		    rval = -EINVAL;
	    }
	    else{
	        memcpy(buffer, podtmp+offset,printed-offset);
	        rval = printed-offset;
	    }
	}

	procfuse_downgradeNodeWriteLockToReadLock(node);

	return rval;
}

int procfuse_onFuseWritePOD(const struct procfuse *pf, const char *path, const char *buffer, size_t size, off_t offset, int64_t tid, const void* appdata){
	int rval = 0;
	char podtmp[8192] = {'\0'}; /* more than long enough for pod datatypes - 80bit long double range is 3.65×10^−4951 to 1.18×10^4932  */
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;
	struct procfuse_transactionnode *tnode = NULL;

	podtmp[sizeof(podtmp)-1]='\0';

	if(!((node->flags & O_RDWR)==O_RDWR || (node->flags & O_WRONLY)==O_WRONLY)){
		return 0;
	}

	if(size<=0){
		return 0;
	}

	procfuse_upgradeNodeReadLockToWriteLock(node);

	if(node->transactions!=NULL)
		tnode=(struct procfuse_transactionnode *)hash_table_lookup(node->transactions, &tid);

	procfuse_downgradeNodeWriteLockToReadLock(node);

	/*
	if(tnode==NULL && node->onpodevent.type!=T_PROC_POD_CHAR && node->onpodevent.type!=T_PROC_POD_STRING){
		rval = -EIO;
		return rval;
	}
	*/

/*
    if(rval==0 && node->onpodevent.type ==T_PROC_POD_STRING && node->onpodevent.touch)
        node->onpodevent.touch(pf, path, tid, O_WRONLY, PROCFUSE_PRE, pf->appdata);
*/

	procfuse_upgradeNodeReadLockToWriteLock(node);

	switch(node->onpodevent.type){
	    case T_PROC_POD_CHAR:
	        node->onpodevent.value.c = buffer[size-1]; /* write the last char in buffer */
	        rval = 1;
		    break;
	    case T_PROC_POD_STRING:
	    	if(node->onpodevent.value.str.length_r==0 || node->onpodevent.value.str.mmapedbuffer_r==NULL){
	    		rval = -EIO;
	    		break;
	    	}
	    	if(offset > node->onpodevent.value.str.length_r){
	    		rval = -EFBIG;
	    		break;
	    	}

	    	if((off_t)size > ((off_t)node->onpodevent.value.str.length_r-offset)){
	    		size = ((off_t)node->onpodevent.value.str.length_r-offset);
	    	}
	    	if(size>0){
	    		memcpy(node->onpodevent.value.str.mmapedbuffer_r+offset, buffer, size);
	    	}
	    	rval = size;

		    break;
	    default:
	    	if(offset > (off_t)(sizeof(podtmp)-1)){
	    		rval = -EFBIG;
	    		break;
	    	}

	    	if((rval=procfuse_onFuseReadPOD(pf, path, podtmp, sizeof(podtmp), 0, tid, appdata))>0){
	    		if(offset > (off_t)strlen(podtmp)){
	    			memset(podtmp+strlen(podtmp), '0', offset-strlen(podtmp));
	    			podtmp[offset] = '\0';
	    		}
	    		if((off_t)size > ((off_t)sizeof(podtmp)-offset)){
	    			size = ((off_t)sizeof(podtmp)-offset);
	    		}

	    		memcpy(podtmp+offset, buffer, size);
	    		rval = size;

	    		switch(node->onpodevent.type){
					case T_PROC_POD_INT:
						node->onpodevent.value.i = atoi(podtmp);
						break;
					case T_PROC_POD_INT64:
						node->onpodevent.value.l = (int64_t)strtoll(podtmp, NULL, 10);
						break;
					case T_PROC_POD_FLOAT:
						node->onpodevent.value.f = strtof(podtmp, NULL);
						break;
					case T_PROC_POD_DOUBLE:
						node->onpodevent.value.d = strtod(podtmp, NULL);
						break;
					case T_PROC_POD_LONGDOUBLE:
						node->onpodevent.value.ld = strtold(podtmp, NULL);
						break;
					default:
						break;
	    		}
	    	}

		    break;
	}

	procfuse_downgradeNodeWriteLockToReadLock(node);

	if(rval > 0){
	    if(node->onpodevent.type!=T_PROC_POD_STRING && tnode!=NULL)
		    tnode->haswritten = 1;
/*	    if(node->onpodevent.type ==T_PROC_POD_STRING && node->onpodevent.touch)
	        node->onpodevent.touch(pf, path, tid, O_WRONLY, PROCFUSE_POST, pf->appdata);
*/
	}

	return rval;
}

int procfuse_callPODModify(const struct procfuse *pf, const char *path, struct procfuse_hashnode *node, union procfuse_pod *newvalue){
	int rval = 0;

	if(node->onpodevent.onModify!=NULL){
		switch(node->onpodevent.type){
			case T_PROC_POD_CHAR:
				rval = ((procfuse_onModify_c)node->onpodevent.onModify)(pf, path, -1, pf->appdata, newvalue->c);
				break;
			case T_PROC_POD_INT:
				rval = ((procfuse_onModify_i)node->onpodevent.onModify)(pf, path, -1, pf->appdata, newvalue->i);
				break;
			case T_PROC_POD_INT64:
				rval = ((procfuse_onModify_i64)node->onpodevent.onModify)(pf, path, -1, pf->appdata, newvalue->l);
				break;
			case T_PROC_POD_FLOAT:
				rval = ((procfuse_onModify_f)node->onpodevent.onModify)(pf, path, -1, pf->appdata, newvalue->f);
				break;
			case T_PROC_POD_DOUBLE:
				rval = ((procfuse_onModify_d)node->onpodevent.onModify)(pf, path, -1, pf->appdata, newvalue->d);
				break;
			case T_PROC_POD_LONGDOUBLE:
				rval = ((procfuse_onModify_ld)node->onpodevent.onModify)(pf, path, -1, pf->appdata, newvalue->ld);
				break;
			case T_PROC_POD_STRING:
				rval = ((procfuse_onModify_s)node->onpodevent.onModify)(pf, path, -1, pf->appdata, newvalue->str.mmapedbuffer_w, newvalue->str.length_w);
				break;
			default:break;
		}
	}

	return rval;
}

int procfuse_onFuseTruncatePOD(const struct procfuse *pf, const char *path, const off_t off, const void* appdata){
	int rval = 0;
	union procfuse_pod newvalue;
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;
	void *remapedbuffer = NULL;

    if(!((node->flags & O_RDWR)==O_RDWR || (node->flags & O_WRONLY)==O_WRONLY || (node->flags & O_TRUNC)==O_TRUNC)){
    	return 0;
    }

    procfuse_upgradeNodeReadLockToWriteLock(node);

	memset(&newvalue, '\0', sizeof(newvalue));
	memcpy(&newvalue, &node->onpodevent.value, sizeof(union procfuse_pod));
	if(node->onpodevent.type == T_PROC_POD_STRING){
	    newvalue.str.mmapedbuffer_w = newvalue.str.mmapedbuffer_r;
	    newvalue.str.length_w = off;
	}

	procfuse_downgradeNodeWriteLockToReadLock(node);


	rval = procfuse_callPODModify(pf, path, node, &newvalue);
	if(rval!=PROCFUSE_YES){
		rval = -EIO;
		return rval;
	}

	procfuse_upgradeNodeReadLockToWriteLock(node);

	if(rval==PROCFUSE_YES){
		switch(node->onpodevent.type){
			case T_PROC_POD_CHAR:
				node->onpodevent.value.c = '\0';
				break;
			case T_PROC_POD_INT:
				node->onpodevent.value.i = 0;
				break;
			case T_PROC_POD_INT64:
				node->onpodevent.value.l = 0;
				break;
			case T_PROC_POD_FLOAT:
				node->onpodevent.value.f = 0.0;
				break;
			case T_PROC_POD_DOUBLE:
				node->onpodevent.value.d = 0.0;
				break;
			case T_PROC_POD_LONGDOUBLE:
				node->onpodevent.value.ld = 0.0;
				break;
			case T_PROC_POD_STRING:
				if(node->onpodevent.value.str.mmapedbuffer_r!=NULL){
					if(off<=node->onpodevent.value.str.length_r){
						memset(node->onpodevent.value.str.mmapedbuffer_r + off, '\0', node->onpodevent.value.str.length_r-off);
					}
					else{
						if(ftruncate(node->onpodevent.value.str.mmapedfd64_r, off)==-1){
							rval = -errno;
						}
						else{
						    node->onpodevent.value.str.length_r = off;
						    remapedbuffer = mremap(node->onpodevent.value.str.mmapedbuffer_r, node->onpodevent.value.str.length_r, off, MREMAP_MAYMOVE);
						    if(remapedbuffer==MAP_FAILED){
							    rval = -errno;
						    }
						}
					}
				}
				break;
			default:break;
		}
	}
	procfuse_downgradeNodeWriteLockToReadLock(node);

	return rval;
}
int procfuse_onFuseReleasePOD(const struct procfuse *pf, const char *path, int64_t tid, const void* appdata){
	int rval = 0;
	struct procfuse_transactionnode *tnode = NULL;
	struct procfuse_hashnode *node = (struct procfuse_hashnode *)appdata;

	(void)pf;
	(void)path;

	if((node->flags & O_RDWR) == O_RDONLY ||
	   node->onpodevent.type==T_PROC_POD_STRING || node->onpodevent.type==T_PROC_POD_CHAR)
		return rval;

	procfuse_upgradeNodeReadLockToWriteLock(node);

	tnode = (struct procfuse_transactionnode *)hash_table_lookup(node->transactions, &tid);
	if(tnode!=NULL){
		switch(node->onpodevent.type){
			case T_PROC_POD_INT:
				node->onpodevent.value.i = strtol(tnode->writebuffer, NULL, 0);
				break;
			case T_PROC_POD_INT64:
				node->onpodevent.value.l = strtol(tnode->writebuffer, NULL, 0);
				break;
			case T_PROC_POD_FLOAT:
				node->onpodevent.value.f = strtof(tnode->writebuffer, NULL);
				break;
			case T_PROC_POD_DOUBLE:
				node->onpodevent.value.d = strtod(tnode->writebuffer, NULL);
				break;
			case T_PROC_POD_LONGDOUBLE:
				node->onpodevent.value.ld = strtold(tnode->writebuffer, NULL);
				break;
			default: break;
		}

		hash_table_remove(node->transactions, &tid);
	}

	procfuse_downgradeNodeWriteLockToReadLock(node);

    return rval;
}

/* FUSE functions */
int procfuse_FUSEgetattr(const char *path, struct stat *stbuf)
{
	int rval = 0;
	struct procfuse_hashnode *node = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	node = procfuse_acquireAccessToNode(pf, path);

	memset(stbuf, 0, sizeof(struct stat));
	if(node!=NULL){
		stbuf->st_gid = node->gid;
		stbuf->st_uid = node->uid;
		stbuf->st_atime = node->access.tv_sec;
		stbuf->st_mtime = node->modify.tv_sec;
	}

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

	procfuse_releaseAccessToNode(pf, node);

	return rval;
}

int procfuse_FUSEreaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi){
	HashTable *htable = NULL;
	int rval = 0;
	HashTableIterator iterator;
	struct procfuse_hashnode *node = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	(void)fi;
	(void)off;

	node = procfuse_acquireAccessToNode(pf, path);

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

	    hash_table_iterate(htable, &iterator);
	    while (hash_table_iter_has_more(&iterator)) {
		    struct procfuse_hashnode *value = (struct procfuse_hashnode *)hash_table_iter_next(&iterator);

		    if(value==HASH_TABLE_NULL){
			    break;
		    }
		    /* don't list files which shall have been unlinked, this reflects standard linux filesystem behaviour */
		    if(value->pendingforunlink==PROCFUSE_YES){
		    	continue;
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

	procfuse_releaseAccessToNode(pf, node);

    return rval;
}

int procfuse_FUSEopen(const char *path, struct fuse_file_info *fi){
	int rval = 0;

	struct procfuse_hashnode *node = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	node = procfuse_acquireAccessToNode(pf, path);

	if(node==NULL || node->subdirs!=NULL || node->pendingforunlink==PROCFUSE_YES){
		rval = -ENOENT;
	}
	else if(!node->onevent.onFuseRead && ((fi->flags & O_RDONLY) || (fi->flags & O_RDWR))){
		rval = -EACCES;
	}
	else if(!node->onevent.onFuseWrite && ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR))){
		rval = -EACCES;
	}
	else{
		pf->tidcounter++;
		fi->fh = pf->tidcounter;

		if(node->onevent.onFuseOpen){
			const void *appdata = pf->appdata;
			if(node->onpodevent.type!=T_PROC_POD_NO)
				appdata = (const void*)node;
			node->onevent.onFuseOpen(pf, path, fi->fh, appdata);
		}

	}

	procfuse_releaseAccessToNode(pf, node);

	return rval;
}
int procfuse_FUSEmknod(const char *path, mode_t mode, dev_t dev){
	(void)path;
	(void)mode;
	(void)dev;

	return 0;
}
int procfuse_FUSEcreate(const char *path, mode_t mode, struct fuse_file_info *fi){
	(void)path;
	(void)mode;
	(void)fi;

	return 0;
}
int procfuse_FUSEtruncate(const char *path, off_t off){
	struct procfuse_hashnode *node = NULL;

	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	node = procfuse_acquireAccessToNode(pf, path);

	if(node!=NULL && node->subdirs==NULL && node->onevent.onFuseTruncate!=NULL){
		const void *appdata = pf->appdata;
		if(node->onpodevent.type!=T_PROC_POD_NO)
			appdata = (const void*)node;
		node->onevent.onFuseTruncate(pf, path, off, appdata);
	}

	procfuse_releaseAccessToNode(pf, node);

	return 0;
}

int procfuse_FUSEread(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi)
{
	int rval = 0;
	struct procfuse_hashnode *node = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;


	node = procfuse_acquireAccessToNode(pf, path);

	if(node==NULL || node->subdirs!=NULL){
		rval = -ENOENT;
	}
	else if(!node->onevent.onFuseRead){
		rval = -EBADF;
	}
	else{
		if(node->onevent.onFuseRead!=NULL){
			const void *appdata = pf->appdata;
			if(node->onpodevent.type!=T_PROC_POD_NO)
				appdata = (const void*)node;
			rval = node->onevent.onFuseRead(pf, path, buf, size, offset, fi->fh, appdata);
		}
	}

	procfuse_releaseAccessToNode(pf, node);

	return rval;
}

int procfuse_FUSEwrite(const char *path, const char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi)
{
	int rval = 0;
	struct procfuse_hashnode *node = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;
	const void *appdata = NULL;

	node = procfuse_acquireAccessToNode(pf, path);

	if(node==NULL || node->subdirs!=NULL){
		rval = -ENOENT;
	}
	else if(!node->onevent.onFuseWrite){
		rval = -EBADF;
	}
	else{
		if(node->onevent.onFuseWrite){
			appdata = pf->appdata;
			if(node->onpodevent.type!=T_PROC_POD_NO){
				appdata = (void*)node;
			}
			rval = node->onevent.onFuseWrite(pf, path, buf, size, offset, fi->fh, appdata);

			if(rval==0){
				rval = -EIO;
			}
		}
	}

	procfuse_releaseAccessToNode(pf, node);


	return rval;
}

int procfuse_FUSErelease(const char *path, struct fuse_file_info *fi){
	struct procfuse_hashnode *node = NULL;
	struct procfuse *pf = (struct procfuse *)fuse_get_context()->private_data;

	node = procfuse_acquireAccessToNode(pf, path);

	if(node!=NULL && node->subdirs==NULL){
		if(node->onevent.onFuseRelease){
			const void *appdata = pf->appdata;
			if(node->onpodevent.type!=T_PROC_POD_NO)
				appdata = (const void*)node;
			node->onevent.onFuseRelease(pf, path, fi->fh, appdata);
		}
	}

	fi->fh = 0;
	procfuse_releaseAccessToNode(pf, node);

	return 0;
}
/* EOF - End of Fuse */

void *procfuse_thread( void *ptr ){
	struct procfuse *pf = (struct procfuse *)ptr;
	char *mountpoint=NULL;
	int multithreaded=0;
	int res=0;

	sigset_t set;

	pthread_mutex_lock(&pf->fuselock);

	/* Block all signals in fuse thread - so all signals are delivered to another (main) thread */
	sigemptyset(&set);
	sigfillset(&set);
	pthread_sigmask(SIG_SETMASK, &set, NULL);

	pf->fuse = fuse_setup(pf->fuseArgc, (char**)pf->fuseArgv, &pf->procFS_oper, sizeof(pf->procFS_oper),
						  &mountpoint, &multithreaded, pf);
	pthread_mutex_unlock(&pf->fuselock);

	if (pf->fuse == NULL)
			return NULL;

	if (multithreaded){
		res = fuse_loop_mt(pf->fuse);
	}else{
		res = fuse_loop(pf->fuse);
	}

	pthread_mutex_lock(&pf->fuselock);
	fuse_teardown(pf->fuse, mountpoint);
	pf->fuse = NULL;
	pf->running = 0;
	pthread_mutex_unlock(&pf->fuselock);

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

	pthread_mutex_lock(&pf->fuselock);
	if(pf->fuse==NULL){
		errno = EINVAL;
	}
	else if(!fuse_exited(pf->fuse)){
	    fuse_exit(pf->fuse);
	}
	pthread_mutex_unlock(&pf->fuselock);

	stat(pf->absolutemountpoint, &buf);
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
	if(pf==NULL || pf->running){
		errno = EINVAL;
		return;
	}
	pf->running = 1;

	pf->procFS_oper.getattr	 = procfuse_FUSEgetattr;
    pf->procFS_oper.readdir	 = procfuse_FUSEreaddir;
    pf->procFS_oper.mknod    = procfuse_FUSEmknod;
    pf->procFS_oper.create   = procfuse_FUSEcreate;

    pf->procFS_oper.open	 = procfuse_FUSEopen;
    pf->procFS_oper.truncate = procfuse_FUSEtruncate;
    pf->procFS_oper.read	 = procfuse_FUSEread;
    pf->procFS_oper.write	 = procfuse_FUSEwrite;
    pf->procFS_oper.release	 = procfuse_FUSErelease;

    pf->fuseArgc=2;
    if(pf->fuse_singlethreaded){
    	pf->fuseArgv[pf->fuseArgc++] = "-s"; /* single threaded */
    }
    pf->fuseArgv[pf->fuseArgc++] = "-f"; /* foreground */
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

