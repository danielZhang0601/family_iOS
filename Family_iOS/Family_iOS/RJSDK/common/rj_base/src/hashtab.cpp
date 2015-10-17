/*
 * Implementation of the hash table type.
 *
 * Author : Stephen Smalley, <sds@epoch.ncsc.mil>
 */
//#include <linux/kernel.h>
//#include <linux/slab.h>
//#include <linux/errno.h>
//#include <linux/sched.h>
#include "util/hashtab.h"
#include "sys/sys_mem.h"

#include <stdlib.h>
#include <errno.h>
#include <assert.h>


struct hashtab *hashtab_create(uint32 (*hash_value)(struct hashtab *h, const void *key, int len),
					uint32 (*hash_key)(struct hashtab *h, const void *key, int len),
					int (*keycmp)(struct hashtab *h, const void *key1, const void *key2),
					int size)
{
	struct hashtab *p;
	int i;

	//p = kzalloc(sizeof(*p), GFP_KERNEL);
	p = (struct hashtab*)sys_malloc(sizeof(*p));
	if (p == NULL)
		return p;

	p->size = size;
	p->nel = 0;
	p->hash_value = hash_value;
	p->hash_key = hash_key;
	p->keycmp = keycmp;
	p->htable = (struct hashtab_node **)sys_malloc(sizeof(*(p->htable)) * size);//kmalloc(sizeof(*(p->htable)) * size, GFP_KERNEL);
	if (p->htable == NULL) {
		//kfree(p);
		sys_free(p);
		return NULL;
	}

	for (i = 0; i < size; i++)
		p->htable[i] = NULL;

	return p;
}

int hashtab_insert(struct hashtab *h, void *key, int len, void *datum)
{
	uint32 hvalue, key1;
	struct hashtab_node *prev, *cur, *newnode;

	//cond_resched();

	if (!h || h->nel == HASHTAB_MAX_NODES)
		return -EINVAL;

	hvalue = h->hash_value(h, key, len);
	key1 = h->hash_key(h, key, len);
	prev = NULL;
	cur = h->htable[hvalue];
	while (cur && key1 > cur->key/*h->keycmp(h, key, cur->key) > 0*/) {
		prev = cur;
		cur = cur->next;
	}

	if (cur && key1 == cur->key/*(h->keycmp(h, key, cur->key) == 0)*/)
		return -EEXIST;

	newnode = (struct hashtab_node *)sys_malloc(sizeof(*newnode));//kzalloc(sizeof(*newnode), GFP_KERNEL);
	if (newnode == NULL)
		return -ENOMEM;
	newnode->key = key1;
	newnode->datum = datum;
	if (prev) {
		newnode->next = prev->next;
		prev->next = newnode;
	} else {
		newnode->next = h->htable[hvalue];
		h->htable[hvalue] = newnode;
	}

	h->nel++;
	return 0;
}

void* hashtab_delete(struct hashtab *h, void *key, int len)
{
	uint32 hvalue, key1;
	struct hashtab_node *prev, *cur;

	//cond_resched();

	if (!h)
		return NULL;

	hvalue = h->hash_value(h, key, len);
	key1 = h->hash_key(h, key, len);
	prev = NULL;
	cur = h->htable[hvalue];
	while (cur && key1 > cur->key/*h->keycmp(h, key, cur->key) > 0*/) {
		prev = cur;
		cur = cur->next;
	}

	if (cur && key1 == cur->key/*(h->keycmp(h, key, cur->key) == 0)*/)
	{
		//将当前节点清除掉
        void *datum = cur->datum;

		if (prev) {
			prev->next = cur->next;
		} else {
			h->htable[hvalue] = NULL;
		}

		sys_free(cur);

		h->nel--;
		return datum;
	}

	return NULL;
}

void *hashtab_search(struct hashtab *h, const void *key, int len)
{
	uint32 hvalue, key1;
	struct hashtab_node *cur;

	if (!h)
		return NULL;

	hvalue = h->hash_value(h, key, len);
	key1 = h->hash_key(h, key, len);
	cur = h->htable[hvalue];
	while (cur && key1 > cur->key/*h->keycmp(h, key, cur->key) > 0*/)
		cur = cur->next;

	if (cur == NULL || key1 != cur->key/*(h->keycmp(h, key, cur->key) != 0)*/)
		return NULL;

	return cur->datum;
}

void hashtab_destroy(struct hashtab *h)
{
	int i;
	struct hashtab_node *cur, *temp;

	if (!h)
		return;

	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
		while (cur) {
			temp = cur;
			cur = cur->next;
			//kfree(temp);
			sys_free(temp);
		}
		h->htable[i] = NULL;
	}

	//kfree(h->htable);
	sys_free(h->htable);
	h->htable = NULL;

	//kfree(h);
	sys_free(h);
}

int hashtab_map(struct hashtab *h,
		int (*apply)(uint32 k, void *d, void *args),
		void *args)
{
	int i;
	int ret;
	struct hashtab_node *cur;

	if (!h)
		return 0;

	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
		while (cur) {
			ret = apply(cur->key, cur->datum, args);
			if (ret)
				return ret;
			cur = cur->next;
		}
	}
	return 0;
}


void hashtab_stat(struct hashtab *h, struct hashtab_info *info)
{
	int i, chain_len, slots_used, max_chain_len;
	struct hashtab_node *cur;

	slots_used = 0;
	max_chain_len = 0;
	for (slots_used = max_chain_len = i = 0; i < h->size; i++) {
		cur = h->htable[i];
		if (cur) {
			slots_used++;
			chain_len = 0;
			while (cur) {
				chain_len++;
				cur = cur->next;
			}

			if (chain_len > max_chain_len)
				max_chain_len = chain_len;
		}
	}

	info->slots_used = slots_used;
	info->max_chain_len = max_chain_len;
}

int hashtab_size(struct hashtab *h)
{
    assert(NULL != h);
    return h->size;
}


int hashtab_nel(struct hashtab *h)
{
    assert(NULL != h);
    return h->nel;
}
