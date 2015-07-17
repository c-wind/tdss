#ifndef __binmap_H__
#define __binmap_H__

#include <stdint.h>

/* Structure of one hash table entry. */
typedef struct binmap_entry_s binmap_entry_t;

struct binmap_entry_s
{
    int64_t            key;			/* lookup key */
    void                *value;			/* associated value */
    binmap_entry_t      *next;		    /* colliding entry */
    binmap_entry_t      *prev;		    /* colliding entry */
};

/* Structure of one hash table. */
typedef struct
{
    int64_t             size;			/* length of entries array */
    int32_t             used;			/* number of entries in table */
    uint8_t             idx;            /* primes id */
    binmap_entry_t     **data;		    /* entries array, auto-resized */

}binmap_t;

binmap_t *binmap_create(int);

binmap_entry_t *binmap_insert(binmap_t *,  int64_t , void *);

void *binmap_find(binmap_t *map, int64_t key);

void binmap_free(binmap_t *map, void (*free_value) (void *));

int binmap_delete(binmap_t *map, int64_t key, void (*free_fn) (char *));

#endif
