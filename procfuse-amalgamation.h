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

/**
 * @file compare-string.h
 * 
 * Comparison functions for strings.
 *
 * To find the difference between two strings, use @ref string_compare.
 *
 * To find if two strings are equal, use @ref string_equal.
 *
 * For case insensitive versions, see @ref string_nocase_compare and
 * @ref string_nocase_equal.
 */

#ifndef ALGORITHM_COMPARE_STRING_H
#define ALGORITHM_COMPARE_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compare two strings to determine if they are equal.
 *
 * @param string1         The first string.
 * @param string2         The second string.
 * @return                Non-zero if the strings are equal, zero if they are
 *                        not equal.
 */

int string_equal(void *string1, void *string2);

/**
 * Compare two strings.
 *
 * @param string1         The first string.
 * @param string2         The second string.
 * @return                A negative value if the first string should be
 *                        sorted before the second, a positive value if the
 *                        first string should be sorted after the second,
 *                        zero if the two strings are equal.
 */

int string_compare(void *string1, void *string2);

/**
 * Compare two strings to determine if they are equal, ignoring the 
 * case of letters.
 *
 * @param string1         The first string.
 * @param string2         The second string.
 * @return                Non-zero if the strings are equal, zero if they are
 *                        not equal.
 */

int string_nocase_equal(void *string1, void *string2);

/**
 * Compare two strings, ignoring the case of letters.
 *
 * @param string1         The first string.
 * @param string2         The second string.
 * @return                A negative value if the first string should be
 *                        sorted before the second, a positive value if the
 *                        first string should be sorted after the second,
 *                        zero if the two strings are equal.
 */

int string_nocase_compare(void *string1, void *string2);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ALGORITHM_COMPARE_STRING_H */

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

/**
 * @file compare-int.h
 *
 * Comparison functions for pointers to integers.
 *
 * To find the difference between two values pointed at, use 
 * @ref int_compare. 
 *
 * To find if two values pointed at are equal, use @ref int_equal.
 */

#ifndef ALGORITHM_COMPARE_INT_H
#define ALGORITHM_COMPARE_INT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compare the integer values pointed at by two pointers to determine
 * if they are equal.
 *
 * @param location1       Pointer to the first value to compare.
 * @param location2       Pointer to the second value to compare.
 * @return                Non-zero if the two values are equal, zero if the 
 *                        two values are not equal.
 */

int int_equal(void *location1, void *location2);

/**
 * Compare the integer values pointed at by two pointers.
 *
 * @param location1        Pointer to the first value to compare.
 * @param location2        Pointer to the second value to compare.
 * @return                 A negative value if the first value is less than 
 *                         the second value, a positive value if the first
 *                         value is greater than the second value, zero if
 *                         they are equal.
 */

int int_compare(void *location1, void *location2);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ALGORITHM_COMPARE_INT_H */

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

/** 
 * @file hash-int.h
 *
 * Hash function for a pointer to an integer.  See @ref int_hash.
 */

#ifndef ALGORITHM_HASH_INT_H
#define ALGORITHM_HASH_INT_H

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Generate a hash key for a pointer to an integer.  The value pointed
 * at is used to generate the key.
 *
 * @param location        The pointer.
 * @return                A hash key for the value at the location.
 */

unsigned long int_hash(void *location);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ALGORITHM_HASH_INT_H */

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

/**
 * @file hash-string.h
 *
 * Hash functions for text strings.  For more information 
 * see @ref string_hash or @ref string_nocase_hash.
 */

#ifndef ALGORITHM_HASH_STRING_H
#define ALGORITHM_HASH_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate a hash key from a string.
 *
 * @param string           The string.
 * @return                 A hash key for the string.
 */

unsigned long string_hash(void *string);

/**
 * Generate a hash key from a string, ignoring the case of letters.
 *
 * @param string           The string.
 * @return                 A hash key for the string.
 */

unsigned long string_nocase_hash(void *string);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ALGORITHM_HASH_STRING_H */

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

/**
 * @file hash-table.h
 *
 * @brief Hash table.
 *
 * A hash table stores a set of values which can be addressed by a 
 * key.  Given the key, the corresponding value can be looked up
 * quickly.
 *
 * To create a hash table, use @ref hash_table_new.  To destroy a 
 * hash table, use @ref hash_table_free.
 *
 * To insert a value into a hash table, use @ref hash_table_insert.
 *
 * To remove a value from a hash table, use @ref hash_table_remove.
 *
 * To look up a value by its key, use @ref hash_table_lookup.
 *
 * To iterate over all values in a hash table, use 
 * @ref hash_table_iterate to initialise a @ref HashTableIterator
 * structure.  Each value can then be read in turn using 
 * @ref hash_table_iter_next and @ref hash_table_iter_has_more.
 */

#ifndef ALGORITHM_HASH_TABLE_H
#define ALGORITHM_HASH_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * A hash table structure.
 */

typedef struct _HashTable HashTable;

/**
 * Structure used to iterate over a hash table.
 */

typedef struct _HashTableIterator HashTableIterator;

/**
 * Internal structure representing an entry in a hash table.
 */

typedef struct _HashTableEntry HashTableEntry;

/**
 * A key to look up a value in a @ref HashTable.
 */

typedef void *HashTableKey;

/**
 * A value stored in a @ref HashTable.
 */

typedef void *HashTableValue;

/**
 * Definition of a @ref HashTableIterator.
 */

struct _HashTableIterator {
	HashTable *hash_table;
	HashTableEntry *next_entry;
	int next_chain;
};

/**
 * A null @ref HashTableValue. 
 */

#define HASH_TABLE_NULL ((void *) 0)

/**
 * Hash function used to generate hash values for keys used in a hash
 * table.
 *
 * @param value  The value to generate a hash value for.
 * @return       The hash value.
 */

typedef unsigned long (*HashTableHashFunc)(HashTableKey value);

/**
 * Function used to compare two keys for equality.
 *
 * @return   Non-zero if the two keys are equal, zero if the keys are 
 *           not equal.
 */

typedef int (*HashTableEqualFunc)(HashTableKey value1, HashTableKey value2);

/**
 * Type of function used to free keys when entries are removed from a 
 * hash table.
 */

typedef void (*HashTableKeyFreeFunc)(HashTableKey value);

/**
 * Type of function used to free values when entries are removed from a 
 * hash table.
 */

typedef void (*HashTableValueFreeFunc)(HashTableValue value);

/**
 * Create a new hash table.
 *
 * @param hash_func            Function used to generate hash keys for the 
 *                             keys used in the table.
 * @param equal_func           Function used to test keys used in the table 
 *                             for equality.
 * @return                     A new hash table structure, or NULL if it 
 *                             was not possible to allocate the new hash
 *                             table.
 */

HashTable *hash_table_new(HashTableHashFunc hash_func, 
                          HashTableEqualFunc equal_func);

/**
 * Destroy a hash table.
 *
 * @param hash_table           The hash table to destroy.
 */

void hash_table_free(HashTable *hash_table);

/**
 * Register functions used to free the key and value when an entry is
 * removed from a hash table.
 *
 * @param hash_table           The hash table.
 * @param key_free_func        Function used to free keys.
 * @param value_free_func      Function used to free values.
 */

void hash_table_register_free_functions(HashTable *hash_table,
                                        HashTableKeyFreeFunc key_free_func,
                                        HashTableValueFreeFunc value_free_func);

/**
 * Insert a value into a hash table, overwriting any existing entry 
 * using the same key.
 *
 * @param hash_table           The hash table.
 * @param key                  The key for the new value.
 * @param value                The value to insert.
 * @return                     Non-zero if the value was added successfully,
 *                             or zero if it was not possible to allocate
 *                             memory for the new entry.
 */

int hash_table_insert(HashTable *hash_table, 
                      HashTableKey key, 
                      HashTableValue value);

/**
 * Look up a value in a hash table by key.
 *
 * @param hash_table          The hash table.
 * @param key                 The key of the value to look up.
 * @return                    The value, or @ref HASH_TABLE_NULL if there 
 *                            is no value with that key in the hash table.
 */

HashTableValue hash_table_lookup(HashTable *hash_table, 
                                 HashTableKey key);

/**
 * Remove a value from a hash table.
 *
 * @param hash_table          The hash table.
 * @param key                 The key of the value to remove.
 * @return                    Non-zero if a key was removed, or zero if the
 *                            specified key was not found in the hash table.
 */

int hash_table_remove(HashTable *hash_table, HashTableKey key);

/** 
 * Retrieve the number of entries in a hash table.
 *
 * @param hash_table          The hash table.
 * @return                    The number of entries in the hash table.
 */

int hash_table_num_entries(HashTable *hash_table);

/**
 * Initialise a @ref HashTableIterator to iterate over a hash table.
 *
 * @param hash_table          The hash table.
 * @param iter                Pointer to an iterator structure to 
 *                            initialise.
 */

void hash_table_iterate(HashTable *hash_table, HashTableIterator *iter);

/**
 * Determine if there are more keys in the hash table to iterate
 * over. 
 *
 * @param iterator            The hash table iterator.
 * @return                    Zero if there are no more values to iterate
 *                            over, non-zero if there are more values to 
 *                            iterate over.
 */

int hash_table_iter_has_more(HashTableIterator *iterator);

/**
 * Using a hash table iterator, retrieve the next key.
 *
 * @param iterator            The hash table iterator.
 * @return                    The next key from the hash table, or 
 *                            @ref HASH_TABLE_NULL if there are no more 
 *                            keys to iterate over.
 */

HashTableValue hash_table_iter_next(HashTableIterator *iterator);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ALGORITHM_HASH_TABLE_H */


/*
 * procfuse.h
 *
 */

#ifndef PROCFUSE_H_
#define PROCFUSE_H_


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

