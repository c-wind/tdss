#include "global.h"
#include "binmap.h"

factory_t factory;


static int64_t primes[] =
{
    3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89,
    107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
    1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419,
    10103, 12143, 14591, 17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523,
    108631, 130363, 156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897,
    1162687, 1395263, 1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369
};




/* binmap_link - insert element into map */

void binmap_link(binmap_t *map, binmap_entry_t *elm)
{
    binmap_entry_t **etr = map->data + ((elm)->key % map->size);

    if (((elm)->next = *etr) != NULL)
        (*etr)->prev = elm;
    *etr = elm;
    map->used++;
}

/* binmap_size - allocate and initialize hash map */
static void binmap_size(binmap_t *map, unsigned size)
{
    binmap_entry_t **h;

    size |= 1;

    map->data = h = (binmap_entry_t **) M_alloc(size * sizeof(binmap_entry_t *));
    map->size = size;
    map->used = 0;

    while (size-- > 0)
        *h++ = 0;
}

/* binmap_create - create initial hash map */
binmap_t *binmap_create(int size)
{
    binmap_t *map;
    uint8_t idx = 0;

    map = (binmap_t *) M_alloc(sizeof(binmap_t));

    for(; idx < sizeof(primes) / sizeof(int64_t); idx++)
    {
        if(size < primes[idx])
        {
            break;
        }
    }

    if(idx >= sizeof(primes) /  sizeof(int64_t))
        return NULL;

    map->idx = idx;

    binmap_size(map, primes[idx]);

    return (map);
}

/* binmap_grow - extend existing map */
static void binmap_grow(binmap_t *map)
{
    binmap_entry_t *be;
    binmap_entry_t *next;
    unsigned old_size = map->size;
    binmap_entry_t **h = map->data;
    binmap_entry_t **old_entries = h;

    binmap_size(map, primes[++map->idx]);

    while (old_size-- > 0)
    {
        for (be = *h++; be; be = next)
        {
            next = be->next;
            binmap_link(map, be);
        }
    }

    M_free((char *) old_entries);
}

/* binmap_insert - enter (key, value) pair */

binmap_entry_t *binmap_insert(binmap_t *map, int64_t key, void *value)
{
    binmap_entry_t *be = NULL;

    if (map->used >= map->size)
        binmap_grow(map);
    be = (binmap_entry_t *) M_alloc(sizeof(binmap_entry_t));
    be->key = key;
    be->value = value;
    binmap_link(map, be);

    return (be);
}


/* binmap_find - lookup value */

void *binmap_find(binmap_t *map, int64_t key)
{
    binmap_entry_t *be = NULL;
    int64_t idx = (key % map->size);

    if (map)
    {
        for (be = map->data[idx]; be; be = be->next)
        {
            if (key == be->key)
                return (be->value);
        }
    }

    return NULL;
}

/* binmap_locate - lookup entry */

binmap_entry_t *binmap_locate(binmap_t *map, int64_t key)
{
    binmap_entry_t *be = NULL;
    int64_t idx = (key % map->size);

    if (map)
    {
        for (be = map->data[idx]; be; be = be->next)
        {
            if (key == be->key)
                return (be);
        }
    }

    return NULL;
}

/* binmap_delete - delete one entry */

int binmap_delete(binmap_t *map, int64_t key, void (*free_fn) (char *))
{
    if (map != 0)
    {
        binmap_entry_t **h = map->data + (key % map->size);
        binmap_entry_t *be = *h;

        for (; be; be = be->next)
        {
            if (key == be->key)
            {
                if (be->next)
                    be->next->prev = be->prev;

                if (be->prev)
                    be->prev->next = be->next;
                else
                    *h = be->next;

                map->used--;

                if (free_fn)
                    (*free_fn) (be->value);

                M_free((char *) be);

                return 0;
            }
        }
        log_error("unknown key: \"%jd\"", key);
        return -1;
    }
    return 0;
}

/* binmap_free - destroy hash map */

void binmap_free(binmap_t *map, void (*free_value) (void *))
{
    if (map != 0)
    {
        unsigned i = map->size;
        binmap_entry_t *be;
        binmap_entry_t *next;
        binmap_entry_t **h = map->data;

        while (i-- > 0)
        {
            for (be = *h++; be; be = next)
            {
                next = be->next;

                if (free_value)
                    (*free_value) (be->value);

                M_free(be);
            }
        }
        M_free(map->data);
        map->data = 0;
        M_free(map);
    }
}

/* binmap_walk - iterate over hash map */

void binmap_walk(binmap_t *map, void (*action) (binmap_entry_t *, char *), char *ptr)
{
    if (map)
    {
        unsigned i = map->size;
        binmap_entry_t **h = map->data;
        binmap_entry_t *be;

        while (i-- > 0)
            for (be = *h++; be; be = be->next)
                (*action) (be, ptr);
    }
}

/* binmap_list - list all map members */
binmap_entry_t **binmap_list(binmap_t *map)
{
    binmap_entry_t **list;
    binmap_entry_t *member;
    int     count = 0;
    int     i;

    if (map != 0)
    {
        list = (binmap_entry_t **) M_alloc(sizeof(*list) * (map->used + 1));
        for (i = 0; i < map->size; i++)
            for (member = map->data[i]; member != 0; member = member->next)
                list[count++] = member;
    }
    else
    {
        list = (binmap_entry_t **) M_alloc(sizeof(*list));
    }
    list[count] = 0;

    return (list);
}




#ifdef BINMAP_TEST

void hash_test_func(binmap_t *hmp)
{
    int64_t key;
    char *val = NULL;
    char buf[1024] = {0};
    int i=0;

    for(; i< 90000; i++)
    {
        key = i;
        sprintf(buf, "key:%d", i);
        val = M_alloc(strlen(buf) + 1);
        sprintf(val, "%s", buf);

        binmap_insert(hmp, key, val);

        printf("insert key:%u, val:%s\n", key, val);
    }


    for(i=0; i< 90000; i++)
    {
        printf("find key:%u, val:%s\n", i, (char *)binmap_find(hmp, i));
    }

}







int main(int argc, char *argv[])
{

    memory_pool_init();

    binmap_t *hmp = binmap_create(102400);

    hash_test_func(hmp);

    return 0;
}

#endif



