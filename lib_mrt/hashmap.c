#include "global.h"



static int32_t primes[] =
{
    3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89,
    107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
    1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419,
    10103, 12143, 14591, 17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523,
    108631, 130363, 156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897,
    1162687, 1395263, 1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369
};




/* hashmap_hash - hash a string
 * hashcode_index -- RShash
 * hashcode_create -- BKDRhash
 * */


int32_t hashcode_index(char *key, int len, int32_t size)
{
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    char *str = key;

    while (len-- > 0)
    {
        hash = hash * a + (*str++);
        a *= b;
    }

    return (hash & 0x7FFFFFFF) % size;
}


uint32_t hashcode_create(char *key, int len)
{
    int32_t seed = 131;
    int32_t hash = 0;
    char *str = key;

    while (len-- > 0)
        hash = hash * seed + (*str++);

    return (hash & 0x7FFFFFFF);
}


/* hashmap_link - insert element into map */

void hashmap_link(hashmap_t *map, hashmap_entry_t *elm)
{
    hashmap_entry_t **etr = map->data + hashcode_index((elm)->key, (elm)->key_len , map->size);

    //如果*etr是空的，就证明这个入口点没有链表
    if (((elm)->next = *etr) != NULL)
        (*etr)->prev = elm;

    map->used++;

    *etr = elm;
}

/* hashmap_entry_init - allocate and initialize hash map */
static int hashmap_entry_init(hashmap_t *map, unsigned size)
{
    hashmap_entry_t **h;

    map->data = h = (hashmap_entry_t **) M_alloc(size * sizeof(hashmap_entry_t *));

    M_cvril(h, "malloc map entry, size:%zu", size * sizeof(hashmap_entry_t *));

    map->size = size;
    map->max = (int)(size * 0.72);
    map->used = 0;

    return MRT_SUC;
}

/* hashmap_create - create initial hash map */
int hashmap_create(int size, hashmap_t **rmap)
{
    int iret = 0;
    hashmap_t *map = NULL;

    map = (hashmap_t *) M_alloc(sizeof(hashmap_t));

    M_cvril(map, "malloc map error");

    for(map->idx = 0; map->idx < sizeof(primes) / sizeof(int32_t); map->idx++)
        if(size < primes[map->idx])
            break;

    if(map->idx > sizeof(primes) /  sizeof(int32_t))
        iret = hashmap_entry_init(map, size);
    else
        iret = hashmap_entry_init(map, primes[map->idx]);

    if(iret == MRT_ERR)
    {
        log_error("hashmap entry init error.");
        M_free(map);
        return MRT_ERR;
    }

    *rmap = map;

    return MRT_SUC;
}


int hashmap_init(hashmap_t *map, int size)
{
    int iret = 0;

    M_cpvril(map);

    for(map->idx = 0; map->idx < sizeof(primes) / sizeof(int32_t); map->idx++)
        if(size < primes[map->idx])
            break;

    if(map->idx > sizeof(primes) /  sizeof(int32_t))
        iret = hashmap_entry_init(map, size);
    else
        iret = hashmap_entry_init(map, primes[map->idx]);

    if(iret == MRT_ERR)
    {
        log_error("hashmap entry init error.");
        return MRT_ERR;
    }

    return MRT_SUC;
}

/* hashmap_grow - extend existing map */
static void hashmap_grow(hashmap_t *map)
{
    hashmap_entry_t *ht;
    hashmap_entry_t *next;
    unsigned old_size = map->size;
    hashmap_entry_t **h = map->data;
    hashmap_entry_t **old_entries = h;

    hashmap_entry_init(map, primes[++map->idx]);

    while (old_size-- > 0) {
        for (ht = *h++; ht; ht = next) {
            next = ht->next;
            hashmap_link(map, ht);
        }
    }

    M_free((char *) old_entries);
}

/* hashmap_insert - enter (key, value) pair */

hashmap_entry_t *hashmap_insert(hashmap_t *map, char *key, int32_t key_len, void *value)
{
    hashmap_entry_t *ht = NULL;

    if (map->used > map->max)
        hashmap_grow(map);
    ht = (hashmap_entry_t *) M_alloc(sizeof(hashmap_entry_t));

    M_cvrvl(ht, "malloc new entry error");

    ht->key = key;
    ht->key_len = key_len;
    ht->value = value;
    ht->hash_code = hashcode_create(key, key_len);
    hashmap_link(map, ht);

    return (ht);
}


/* hashmap_find - lookup value */

void *hashmap_find(hashmap_t *map, char *key, int key_len)
{
    hashmap_entry_t *ht;
    int32_t idx = hashcode_index(key, key_len , map->size);
    int32_t hc = hashcode_create(key, key_len);

    if (map)
    {
        for (ht = map->data[idx]; ht; ht = ht->next)
        {
            if (key_len == ht->key_len && hc == ht->hash_code
                && (memcmp(key, ht->key, ht->key_len) == 0))
                return (ht->value);
        }
    }

    return NULL;
}

/* hashmap_locate - lookup entry */

hashmap_entry_t *hashmap_locate(hashmap_t *map, char *key, int key_len)
{
    hashmap_entry_t *ht;
    int32_t idx = hashcode_index(key, key_len, map->size);
    uint32_t hc = hashcode_create(key, key_len);

    if (map)
    {
        for (ht = map->data[idx]; ht; ht = ht->next)
        {
            if (key_len == ht->key_len && hc == ht->hash_code && (memcmp(key, ht->key, ht->key_len) == 0))
                return (ht);
        }
    }

    return (0);
}

/* hashmap_delete - delete one entry */

void hashmap_delete(hashmap_t *map, char *key, int key_len, void (*free_fn) (char *))
{
    if (map != 0)
    {
        hashmap_entry_t **h = map->data + hashcode_index(key, key_len, map->size);
        hashmap_entry_t *ht = *h;
        uint32_t hc = hashcode_create(key, key_len);

        for (; ht; ht = ht->next)
        {
            if (key_len == ht->key_len
                && hc == ht->hash_code
                && !memcmp(key, ht->key, ht->key_len))
            {
                if (ht->next)
                    ht->next->prev = ht->prev;

                if (ht->prev)
                    ht->prev->next = ht->next;
                else
                    *h = ht->next;

                map->used--;
                M_free(ht->key);

                if (free_fn)
                    (*free_fn) (ht->value);

                M_free((char *) ht);

                return;
            }
        }
        log_error("unknown key: \"%s\"", key);
    }
}

/* hashmap_free - destroy hash map */

void hashmap_free(hashmap_t *map, void (*free_key) (void *), void (*free_value) (void *))
{
    if (map != 0)
    {
        unsigned i = map->size;
        hashmap_entry_t *ht;
        hashmap_entry_t *next;
        hashmap_entry_t **h = map->data;

        while (i-- > 0)
        {
            for (ht = *h++; ht; ht = next)
            {
                next = ht->next;

                if (free_key)
                    (*free_key) (ht->key);

                if (free_value)
                    (*free_value) (ht->value);

                M_free(ht);
            }
        }
        M_free(map->data);
        map->data = 0;
    }
}

/* hashmap_walk - iterate over hash map */

void hashmap_walk(hashmap_t *map, void (*action) (hashmap_entry_t *, char *), char *ptr)
{
    if (map)
    {
        unsigned i = map->size;
        hashmap_entry_t **h = map->data;
        hashmap_entry_t *ht;

        while (i-- > 0)
            for (ht = *h++; ht; ht = ht->next)
                (*action) (ht, ptr);
    }
}

/* hashmap_list - list all map members */
hashmap_entry_t **hashmap_list(hashmap_t *map)
{
    hashmap_entry_t **list;
    hashmap_entry_t *member;
    int     count = 0;
    int     i;

    if (map != 0)
    {
        list = (hashmap_entry_t **) M_alloc(sizeof(*list) * (map->used + 1));
        for (i = 0; i < map->size; i++)
            for (member = map->data[i]; member != 0; member = member->next)
                list[count++] = member;
    }
    else
    {
        list = (hashmap_entry_t **) M_alloc(sizeof(*list));
    }
    list[count] = 0;

    return (list);
}




#ifdef HASHMAP_TEST

void hash_test_func(hashmap_t *hmp)
{
    char *key = NULL;
    char *val = NULL;
    char buf[1024] = {0};
    int j=0;
    int i=0;

    for(; i< 90000; i++)
    {
        j = 0xffffff - i;

        sprintf(buf, "%x", j);

        key = str_newcpy(buf);

        sprintf(buf, "%x", i - j);

        val = str_newcpy(buf);

        hashmap_insert(hmp, key, strlen(key), val);

        //        printf("insert key:%s, val:%s\n", key, val);
    }


    for(i=0; i< 90000; i++)
    {
        j = 0xffffff - i;
        sprintf(buf, "%x", j);

        //      printf("find key:%s, val:%s\n", buf, hashmap_find(hmp, buf, strlen(buf)));

    }


}







int main(int argc, char *argv[])
{

    memory_pool_init();

    hashmap_t *hmp = hashmap_create(102400);

    hash_test_func(hmp);


    return 0;
}

#endif



