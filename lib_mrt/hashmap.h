#ifndef __hashmap_H__
#define __hashmap_H__

#include <stdint.h>

/* Structure of one hash table entry. */
typedef struct hashmap_entry_s hashmap_entry_t;

struct hashmap_entry_s
{
    char                *key;			/* lookup key */
    int32_t             key_len;
    uint32_t            hash_code;		/* hash code */
    char                *value;			/* associated value */
    hashmap_entry_t     *next;		    /* colliding entry */
    hashmap_entry_t     *prev;		    /* colliding entry */
};

/* Structure of one hash table. */
typedef struct
{
    int64_t             size;			/* length of entries array */
    int64_t             max;			/* 最大可用空间为size * 0.72, 超过这个空间就要增加总空间大小 */
    int64_t             used;			/* number of entries in table */
    int16_t             idx;            /* primes index */
    hashmap_entry_t     **data;		    /* entries array, auto-resized */

}hashmap_t;

hashmap_entry_t *hashmap_insert(hashmap_t *, char *, int32_t , void *);

uint32_t hashcode_create(char *key, int len);

/* hashmap_create - create initial hash map */
int hashmap_create(int size, hashmap_t **rmap);

int hashmap_init(hashmap_t *map, int size);

void *hashmap_find(hashmap_t *table, char *key, int key_len);

void hashmap_free(hashmap_t *table, void (*free_key) (void *), void (*free_value) (void *));

void hashmap_delete(hashmap_t *table, char *key, int key_len, void (*free_fn) (char *));

#endif
