/*
 * A hash table (hashtab) maintains associations between
 * key values and datum values.  The type of the key values
 * and the type of the datum values is arbitrary.  The
 * functions for hash computation and key comparison are
 * provided by the creator of the table.
 *
 * Author : Stephen Smalley, <sds@epoch.ncsc.mil>
 */
#ifndef _SS_HASHTAB_H_
#define _SS_HASHTAB_H_

#include "util/rj_type.h"

/* 
 * 拷贝来至 linux-3.18.3\security\selinux\ss
 * 提供Hash查找算法 
 */


#define HASHTAB_MAX_NODES	0xffffffff

struct hashtab_node {
	uint32 key;
	void *datum;
	struct hashtab_node *next;
};

struct hashtab {
	struct hashtab_node **htable;	/* hash table */
	int size;			/* number of slots in hash table */
	int nel;			/* number of elements in hash table */
	uint32 (*hash_value)(struct hashtab *h, const void *key, int len);
					/* hash function 作为索引使用 */
	uint32 (*hash_key)(struct hashtab *h, const void *key, int len); /* 第二个hash作为校验使用 */
	int (*keycmp)(struct hashtab *h, const void *key1, const void *key2);
					/* key comparison function */
};

struct hashtab_info {
	int slots_used;
	int max_chain_len;
};

/*
 * Creates a new hash table with the specified characteristics.
 *
 * Returns NULL if insufficent space is available or
 * the new hash table otherwise.
 */
RJ_API struct hashtab *hashtab_create(uint32 (*hash_value)(struct hashtab *h, const void *key, int len),
					uint32 (*hash_key)(struct hashtab *h, const void *key, int len),
					int (*keycmp)(struct hashtab *h, const void *key1, const void *key2),
					int size);

/*
 * Inserts the specified (key, datum) pair into the specified hash table.
 *
 * Returns -ENOMEM on memory allocation error,
 * -EEXIST if there is already an entry with the same key,
 * -EINVAL for general errors or
  0 otherwise.
 */
RJ_API int hashtab_insert(struct hashtab *h, void *k, int len, void *d);

RJ_API void* hashtab_delete(struct hashtab *h, void *key, int len);

/*
 * Searches for the entry with the specified key in the hash table.
 *
 * Returns NULL if no entry has the specified key or
 * the datum of the entry otherwise.
 */
RJ_API void *hashtab_search(struct hashtab *h, const void *k, int len);

/*
 * Destroys the specified hash table.
 */
RJ_API void hashtab_destroy(struct hashtab *h);

/*
 * Applies the specified apply function to (key,datum,args)
 * for each entry in the specified hash table.
 *
 * The order in which the function is applied to the entries
 * is dependent upon the internal structure of the hash table.
 *
 * If apply returns a non-zero status, then hashtab_map will cease
 * iterating through the hash table and will propagate the error
 * return to its caller.
 */
RJ_API int hashtab_map(struct hashtab *h,
		int (*apply)(uint32 k, void *d, void *args),
		void *args);

/* Fill info with some hash table statistics */
RJ_API void hashtab_stat(struct hashtab *h, struct hashtab_info *info);


// number of slots in hash table
RJ_API int hashtab_size(struct hashtab *h);


// number of elements in hash table
RJ_API int hashtab_nel(struct hashtab *h);


/*
	重置hash表
*/
//void hashtab_reset();


#endif	/* _SS_HASHTAB_H */
