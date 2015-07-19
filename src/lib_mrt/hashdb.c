#include <syslog.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <errno.h>
#include "hashdb.h"
#include "global.h"

/*
#define log_emerg(fmt, ...) syslog(LOG_EMERG, "[EMERY] %s "fmt, __func__, ##__VA_ARGS__)
#define log_alert(fmt, ...) syslog(LOG_ALERT, "[ALERT] %s "fmt, __func__, ##__VA_ARGS__)
#define log_crit(fmt, ...) syslog(LOG_CRIT, "[CRIT] %s "fmt, __func__, ##__VA_ARGS__)
#define log_error(fmt, ...) syslog(LOG_ERR, "[ERROR] %s "fmt, __func__, ##__VA_ARGS__)
#define log_warning(fmt, ...) syslog(LOG_WARNING, "[WARNING] %s "fmt, __func__, ##__VA_ARGS__)
#define log_notice(fmt, ...) syslog(LOG_NOTICE, "[NOTICE] %s "fmt, __func__, ##__VA_ARGS__)
#define log_info(fmt, ...) syslog(LOG_INFO, "[INFO] %s "fmt, __func__, ##__VA_ARGS__)
#define log_debug(fmt, ...) syslog(LOG_DEBUG, "[DEBUG] %s "fmt, __func__, ##__VA_ARGS__)
*/


static int primes[] =
{
    433,   1201,   3469,   10093,  18253,  28813,  43201,  60493,  76801,
    106033, 129793, 169933, 209089, 245389, 280909, 326701, 397489, 534253,
    596749, 685453, 762049, 842701, 940801,
    1037233, 1138369, 1267501, 1403569, 1529389, 1642801, 1769473, 1910413,
    2036929, 2229133, 2376301, 2550253, 2707501, 2892973, 3060301, 3269809,
    3434701, 3616813, 3844273, 4036801, 4234033, 4435969, 4642609, 4869229,
    5101249, 5322673, 5548801, 5779633, 6015169, 6255409, 6553453, 6858433,
    7133293, 7394701, 7699213, 7970701, 8406829, 8690413, 9103693, 9612301,
    10002829, 10334209, 10693633, 11059201, 11524801, 12120301, 12607501,
    13029169, 13483201, 14022733, 14414593, 14838529, 15431473, 15842413,
    16286701, 16708801, 17222449, 17714701, 18184333, 18750001, 19263469,
    19783873, 20750701, 21258733, 21934849, 22489933, 23018701, 23654593,
    24367501, 24917773, 25579201, 26142913, 26892109, 27542701, 28237873,
    28978993, 29616493, 30337201, 31105201, 31804609, 32709613, 33627313,
    34476301, 35253553, 35997889, 36876109, 37935409, 38836813, 39879949,
    40715569, 41559853, 42367693, 43137793, 44144689, 45256369, 46146253,
    47044801, 47904049, 48722701, 49645873, 50479213, 51916801, 52869613,
    54391693, 55366849, 56298673, 57238273, 58132813, 59087533, 59996353,
    61671469, 62654701, 63645709, 64755949, 65819569, 66778573, 67744513,
    69004849, 70334893, 72088813, 73092289, 74162353, 75842353, 77480173,
    78581773, 80000689, 81120001, 82435693, 83571853, 84715789, 86060209,
    87220993, 88585069, 89828353, 91278769, 92474113, 94753201, 96038893,
    97401613, 98636269, 101198593
};

#define primes_size (sizeof(primes) / sizeof(int))


static void hashdb_append_binlog(hashdb_t *hdb, int operate, char *key, uint16_t klen, void *val, uint16_t vlen);
static hashdb_entry_t *hashdb_locate(hashdb_t *map, char *key, int key_len);


char *hashdb_code_to_string(int code)
{
    switch(code)
    {
        case HDB_OK:
            return "operate success";

        case HDB_ERR_NOMEM:
            return "no memory";
        case HDB_ERR_FWRITE:
            return "fwrite error";
        case HDB_ERR_FREAD:
            return "fread error";
        case HDB_ERR_FFLUSH:
            return "fflush error";
        case HDB_ERR_FOPEN:
            return "fopen error";

        case HDB_KEY_EXIST:
            return "key exist";
        case HDB_KEY_NOFOUND:
            return "key no found";
        case HDB_ARG_INVAL:
            return "argument error";
    }
    return "unknow error";
}


/* hashdb_hash - hash a string
 * hashcode_index -- RShash
 * hashcode_create -- BKDRhash
 * */


inline uint32_t hashcode_index(char *key, int len, uint32_t size)
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

    return (uint32_t )(hash & 0x7FFFFFFF) % size;
}


inline uint32_t hashcode_create(char *key, int len)
{
    uint32_t seed = 131;
    uint32_t hash = 0;
    char *str = key;

    while (len-- > 0)
        hash = hash * seed + (*str++);

    return (hash & 0x7FFFFFFF);
}


/* hashdb_link - insert element into map */

void hashdb_link(hashdb_t *map, hashdb_entry_t *elm)
{
    hashdb_entry_t **etr = map->data + hashcode_index((elm)->key, (elm)->key_len , map->size);
    if (((elm)->next = *etr) != NULL)
        (*etr)->prev = elm;
    *etr = elm;
    map->used++;
}

/* hashdb_size - allocate and initialize hash map */
static int hashdb_size(hashdb_t *map, unsigned size)
{
    hashdb_entry_t **h;

    map->data = h = (hashdb_entry_t **) calloc(size, sizeof(hashdb_entry_t *));
    if(!h)
    {
        return -1;
    }
    map->size = size;
    map->used = 0;

    while (size-- > 0)
        *h++ = 0;

    return HDB_OK;
}

/* hashdb_create - create initial hash map */
int hashdb_create(hashdb_t **map, int size)
{
    int idx = 0;

    *map = calloc(sizeof(hashdb_t), 1);
    if(!*map)
    {
        return -1;
    }

    for(; idx < primes_size; idx++)
    {
        if(size < primes[idx])
        {
            break;
        }
    }

    if(idx >= primes_size)
        return -1;

    (*map)->idx = idx;

    return hashdb_size(*map, primes[idx]);
}

/* hashdb_grow - extend existing map */
static int hashdb_grow(hashdb_t *map)
{
    hashdb_entry_t *ht;
    hashdb_entry_t *next;
    unsigned old_size = map->size;
    hashdb_entry_t **h = map->data;
    hashdb_entry_t **old_entries = h;

    if(hashdb_size(map, primes[++map->idx]) == MRT_ERR)
    {
        return -1;
    }

    while (old_size-- > 0)
    {
        for (ht = *h++; ht; ht = next)
        {
            next = ht->next;
            hashdb_link(map, ht);
        }
    }

    free((char *) old_entries);
    return HDB_OK;
}

//push操作添加的val不重新申请内存
int hashdb_push(hashdb_t *map, char *key, uint16_t key_len, void *val, uint16_t val_len)
{
    hashdb_entry_t *he = NULL;

    if (!map || !key ||!key_len ||!val ||!val_len)
        return HDB_ARG_INVAL;

    if(hashdb_locate(map, key, key_len))
        return HDB_KEY_EXIST;

    if ((map->used >= map->size) && hashdb_grow(map)==-1)
    {
        log_error("hashdb_grow error:%m");
        return HDB_ERR_NOMEM;
    }

    he = (hashdb_entry_t *) calloc(1, sizeof(hashdb_entry_t));
    if(!he)
    {
        log_error("malloc hashdb entry error:%m");
        return HDB_ERR_NOMEM;
    }

    //申请key内存
    he->key_len = key_len;
    if(!(he->key =  malloc(key_len)))
    {
        log_error("malloc hashdb key error:%m");
        free(he);
        return HDB_ERR_NOMEM;
    }
    memcpy(he->key, key, key_len);

    he->val_len = val_len;
    he->val = val;
    he->val_type = 1;

    he->hash_code = hashcode_create(key, key_len);

    hashdb_link(map, he);

    //log_debug("insert ok key:%s, len:%d, val:%s, len:%d", key, key_len, val, val_len);

    if(map->stat == 1)
    {
        hashdb_append_binlog(map, 1, key, key_len, val, val_len);
    }
    //printf("key:%s, val:%s, klen:%d\n", key, value, key_len);

    return HDB_OK;
}
/* hashdb_insert - enter (key, value) pair */

int hashdb_insert(hashdb_t *map, char *key, uint16_t key_len, void *val, uint16_t val_len)
{
    hashdb_entry_t *he = NULL;

    if (!map || !key ||!key_len ||!val ||!val_len)
        return HDB_ARG_INVAL;

    if(hashdb_locate(map, key, key_len))
        return HDB_KEY_EXIST;

    if ((map->used >= map->size) && hashdb_grow(map)==-1)
    {
        log_error("hashdb_grow error:%m");
        return HDB_ERR_NOMEM;
    }

    he = (hashdb_entry_t *) calloc(1, sizeof(hashdb_entry_t));
    if(!he)
    {
        log_error("malloc hashdb entry error:%m");
        return HDB_ERR_NOMEM;
    }

    //申请key内存
    he->key_len = key_len;
    if(!(he->key =  malloc(key_len)))
    {
        log_error("malloc hashdb key error:%m");
        free(he);
        return HDB_ERR_NOMEM;
    }
    memcpy(he->key, key, key_len);

    //申请val内存
    he->val_len = val_len;
    if(!(he->val =  malloc(val_len)))
    {
        log_error("malloc hashdb val error:%m");
        free(key);
        free(he);
        return HDB_ERR_NOMEM;
    }
    memcpy(he->val, val, val_len);

    he->hash_code = hashcode_create(key, key_len);

    hashdb_link(map, he);

    //log_debug("insert ok key:%s, len:%d, val:%s, len:%d", key, key_len, val, val_len);

    if(map->stat == 1)
    {
        hashdb_append_binlog(map, 1, key, key_len, val, val_len);
    }
    //printf("key:%s, val:%s, klen:%d\n", key, value, key_len);

    return HDB_OK;
}

/* hashdb_find - lookup value */

int hashdb_find(hashdb_t *map, char *key, int key_len, void **val)
{
    hashdb_entry_t *ht;
    uint32_t idx;
    uint32_t hc;

    if (!map || !key ||!key_len ||!val)
        return HDB_ARG_INVAL;

    idx = hashcode_index(key, key_len , map->size);
    hc = hashcode_create(key, key_len);

    for (ht = map->data[idx]; ht; ht = ht->next)
    {
        if (key_len == ht->key_len && hc == ht->hash_code && (memcmp(key, ht->key, ht->key_len) == 0))
        {
            *val = ht->val;
            return HDB_OK;
        }
    }

    return HDB_KEY_NOFOUND;
}


/* hashdb_locate - lookup entry */

hashdb_entry_t *hashdb_locate(hashdb_t *map, char *key, int key_len)
{
    hashdb_entry_t *ht;
    uint32_t idx = hashcode_index(key, key_len, map->size);
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



//如果是push进来的内存需要注意，修改会释放原内存
int hashdb_update(hashdb_t *map, char *key, int key_len, void *val, uint16_t val_len)
{
    hashdb_entry_t *he = NULL;
    void *nval = NULL;
    uint32_t idx, hc;

    if (!map || !key ||!key_len ||!val ||!val_len)
        return HDB_ARG_INVAL;

    idx = hashcode_index(key, key_len , map->size);
    hc = hashcode_create(key, key_len);

    for (he = map->data[idx]; he; he = he->next)
    {
        if (key_len == he->key_len && hc == he->hash_code && (memcmp(key, he->key, he->key_len) == 0))
        {
            //申请val内存
            if(!(nval =  malloc(val_len)))
            {
                log_error("malloc hashdb val error:%m");
                return HDB_ERR_NOMEM;
            }
            memcpy(nval, val, val_len);
            free(he->val);
            he->val = nval;
            he->val_len = val_len;

            if(map->stat == 1)
            {
                //log_debug("val len%d", val_len);
                hashdb_append_binlog(map, 2, key, key_len, val, val_len);
            }
            return HDB_OK;
        }
    }

    return HDB_KEY_NOFOUND;
}

/* hashdb_delete - delete one entry */

int hashdb_delete(hashdb_t *map, char *key, int key_len)
{
    hashdb_entry_t **head;
    hashdb_entry_t *he;
    uint32_t hc;

    if (!map || !key ||!key_len)
        return HDB_ARG_INVAL;

    head = map->data + hashcode_index(key, key_len, map->size);
    he = *head;
    hc = hashcode_create(key, key_len);

    for (; he; he = he->next)
    {
        if (key_len == he->key_len && hc == he->hash_code && !memcmp(key, he->key, he->key_len))
        {
            if (he->next)
                he->next->prev = he->prev;

            if (he->prev)
                he->prev->next = he->next;
            else
                *head = he->next;

            map->used--;
            free(he->key);
            //当val_type为0时需要释放
            if(!he->val_type)
                free(he->val);
            free(he);

            if(map->stat == 1)
            {
                hashdb_append_binlog(map, 1, key, key_len, NULL, 0);
            }

            return HDB_OK;
        }
    }

    log_error("no found key: `%s`", key);

    return HDB_KEY_NOFOUND;
}

/* hashdb_free - destroy hash map */

void hashdb_free(hashdb_t *map)
{
    if (map != 0)
    {
        unsigned i = map->size;
        hashdb_entry_t *ht;
        hashdb_entry_t *next;
        hashdb_entry_t **h = map->data;

        while (i-- > 0)
        {
            for (ht = *h++; ht; ht = next)
            {
                next = ht->next;

                free(ht->key);
                //当val_type为0时需要释放
                if(!ht->val_type)
                    free(ht->val);
                free(ht);
            }
        }
        free(map->data);
        map->data = 0;
        free(map);
    }
}

/* hashdb_walk - iterate over hash map */

void hashdb_walk(hashdb_t *map, void (*action) (hashdb_entry_t *, void *), void *ptr)
{
    if (!map)
        return;

    uint32_t i = map->size;
    hashdb_entry_t **h = map->data;
    hashdb_entry_t *ht;

    while (i-- > 0)
        for (ht = *h++; ht; ht = ht->next)
            (*action) (ht, ptr);
}

/* hashdb_list - list all map members */
hashdb_entry_t **hashdb_list(hashdb_t *map)
{
    hashdb_entry_t **list;
    hashdb_entry_t *member;
    int     count = 0;
    int     i;

    if (map != 0)
    {
        list = (hashdb_entry_t **) calloc((map->used + 1), sizeof(*list));
        if(!list)
        {
            log_error("calloc error:%m");
            return NULL;
        }

        for (i = 0; i < map->size; i++)
            for (member = map->data[i]; member != 0; member = member->next)
                list[count++] = member;
    }
    else
    {
        list = (hashdb_entry_t **) calloc(1, sizeof(*list));
        if(!list)
        {
            log_error("calloc error:%m");
            return NULL;
        }

    }
    list[count] = 0;

    return (list);
}

//参数：
//      fname: binlog文件名
//      phdb: 创建成功后返回的hashdb
//返回：
//      0：成功
//      -1：出错

int hashdb_init(char *fname, hashdb_t **phdb, int (*load)(uint8_t type, char *key, uint16_t klen, void *val, uint16_t vlen))
{
    hashdb_sec_t sec = {0};
    hashdb_t *hdb = NULL;
    char *key = NULL, *val = NULL;
    int iret, ok_num = 0, err_num =0;

    if(hashdb_create(&hdb, 108631) == MRT_ERR)
    {
        log_error("create hashdb error:%m");
        return -1;
    }

    if(!fname || !*fname)
    {
        *phdb =  hdb;
        return 0;
    }

    hdb->bfp = fopen(fname, "a+");
    if(!hdb->bfp)
    {
        log_error("fopen %s error:%m", fname);
        goto hashdb_load_error;
    }

    setbuf(hdb->bfp, hdb->fbuf);

    while((fread(&sec, sizeof(sec), 1, hdb->bfp) == 1))
    {
        key = realloc(key, sec.klen + 1);
        key[sec.klen] = 0;
        if(fread(key, sec.klen, 1, hdb->bfp) != 1)
        {
            log_error("fread %s key error:%m", fname);
            goto hashdb_load_error;
        }

        //删除的记录没有val
        if(sec.vlen > 0)
        {
            val = realloc(val, sec.vlen + 1);
            val[sec.vlen] = 0;
            if(fread(val, sec.vlen, 1, hdb->bfp) != 1)
            {
                log_error("fread %s val error:%m", fname);
                goto hashdb_load_error;
            }
        }

        //如果回调函数返回1证明这个key不需要导入了
        if(load && (load(sec.type, key, sec.klen, val, sec.vlen) == 1))
        {
            log_info("no load key:%s", key);
            continue;
        }

        switch(sec.type)
        {
            case 1:
                ok_num++;

                if((iret = hashdb_insert(hdb, key, sec.klen, val, sec.vlen)))
                {
                    log_error("hashdb_insert key:%s error:%s", key, hashdb_code_to_string(iret));
                    //                  goto hashdb_load_error;
                }
                else
                    log_debug("load insert key:%s", key);
                break;
            case 2:
                ok_num++;
                if((iret = hashdb_update(hdb, key, sec.klen, val, sec.vlen)))
                {
                    log_error("hashdb_update key:%s error:%s", key, hashdb_code_to_string(iret));
                    //                   goto hashdb_load_error;
                }
                else
                    log_debug("load update key:%s,len:%d, val:%s, len:%d", key, sec.klen, val, sec.vlen);
                break;
            case 3:
                ok_num++;
                if(hashdb_delete(hdb, key, sec.klen))
                {
                    log_error("hashdb_delete key:%s error:%s", key, hashdb_code_to_string(iret));
                    //                    goto hashdb_load_error;
                }
                else
                    log_debug("load delete key:%s", key);
                break;
            default:
                err_num++;
                log_error("operate unknow type:%d", sec.type);
                break;
        }
    }

    //如能上一次写文件时意外退出导致不完整
    if(!feof(hdb->bfp))
    {
        log_error("load file:%s read error:%m", fname);
        //        goto hashdb_load_error;
    }

    log_debug("load success:%d error:%d", ok_num, err_num);

    if(*phdb)
        hashdb_free(*phdb);

    *phdb = hdb;

    snprintf(hdb->bname, sizeof(hdb->bname), "%s", fname);

    hdb->stat = 1;

    log_info("load hashdb success, file:%s", fname);

    if(key)
        free(key);

    if(val)
        free(val);

    return HDB_OK;

hashdb_load_error:
    if(hdb)
    {
        if(hdb->bfp)
            fclose(hdb->bfp);
        hashdb_free(hdb);
    }

    if(key)
        free(key);
    if(val)
        free(val);

    return -1;
}


static void __hashdb_flush_entry(hashdb_entry_t *he, void *dat)
{
    FILE *fp = (FILE *)dat;
    hashdb_sec_t sec = {0};

    sec.type = 1;
    sec.klen = he->key_len;
    sec.vlen = he->val_len;

    if(fwrite(&sec, sizeof(sec), 1, fp) != 1)
    {
        log_error("fwrite sec error:%m, key:%s", he->key);
        return;
    }

    if(fwrite(he->key, he->key_len, 1, fp) != 1)
    {
        log_error("fwrite key:%s error:%m", he->key);
        return;
    }

    if(fwrite(he->val, he->val_len, 1, fp) != 1)
    {
        log_error("fwrite val error:%m, key:%s", he->key);
        return;
    }
}

//功能：
//      备份操作，会创建临时文件，保存成功后再link回来
//返回：
//      0：成功
//      -1：出错
int hashdb_flush_binlog(hashdb_t *hdb)
{
    FILE *sfp = NULL;
    char fbuf[BUFSIZ] = {0};
    char tmp_name[128] = {0};
    int i=0;

    if(!hdb)
        return -1;

    for(i=0; i< 100;i++)
    {
        snprintf(tmp_name, sizeof(tmp_name), "%.05x%.02x.db", getpid(), i);
        if(access(tmp_name, F_OK))
            break;
        log_error("tmp file:%s %m", tmp_name);
    }

    if(i == 100)
    {
        log_error("can't open file...");
        return -1;
    }

    sfp = fopen(tmp_name, "w+");
    if(!sfp)
    {
        log_error("fopen %s error:%m", tmp_name);
        return -1;
    }

    setbuf(sfp, fbuf);

    log_debug("will dump to %s", tmp_name);

    hashdb_walk(hdb, __hashdb_flush_entry, (void *)sfp);

    if(fflush(sfp))
    {
        log_error("fflush %s error:%m", tmp_name);
        fclose(sfp);
        return -1;
    }

    log_debug("dump to %s success", tmp_name);

    if(unlink(hdb->bname))
    {
        log_error("unlink old binlog %s error:%m", hdb->bname);
        fclose(sfp);
        return -1;
    }
    fclose(hdb->bfp);
    log_debug("unlink %s success", hdb->bname);


    if(link(tmp_name, hdb->bname))
    {
        log_error("link tmp file:%s to %s, error:%m", tmp_name, hdb->bname);
        return -1;
    }
    log_debug("link %s to %s success", tmp_name, hdb->bname);

    if(unlink(tmp_name))
    {
        log_error("unlink tmp binlog %s error:%m", tmp_name);
    }
    log_debug("unlink %s success", tmp_name);

    hdb->bfp = sfp;

    setbuf(hdb->bfp, hdb->fbuf);

    //    rewind(hdb->bfp);

    return HDB_OK;
}

//如果这个函数写出错当前进程直接退出
static void hashdb_append_binlog(hashdb_t *hdb, int operate, char *key, uint16_t klen, void *val, uint16_t vlen)
{
    hashdb_sec_t sec = {0};

    //sec中的type是8位的，防止编译时忽略警告导致溢出
    switch(operate)
    {
        case 1:
            sec.type = 1;
            break;
        case 2:
            sec.type = 2;
            break;
        case 3:
            sec.type = 3;
            break;
        default:
            log_error("unknow operate:%d", operate);
            return;
    }

    sec.klen = klen;
    sec.vlen = vlen;

    if(fwrite(&sec, sizeof(sec), 1, hdb->bfp) != 1)
    {
        log_error("fwrite sec error:%m, file:%s key:%s", hdb->bname, key);
        _exit(-1);
    }

    if(fwrite(key, klen, 1, hdb->bfp) != 1)
    {
        log_error("fwrite key:%s error:%m, file:%s", key, hdb->bname);
        _exit(-1);
    }

    if(vlen > 0)
    {
        if(fwrite(val, vlen, 1, hdb->bfp) != 1)
        {
            log_error("fwrite val error:%m, key:%s", key);
            _exit(-1);
        }
    }

    if(fflush(hdb->bfp))
    {
        log_error("fflush error:%m, file:%s", hdb->bname);
        _exit(-1);
    }
}



#ifdef HASHDB_TEST


void hash_test_func(hashdb_t *hdb)
{
    char key[1024], val[1024], *pval = NULL;
    int i=0, j, ret;

    printf("begin\n");


    printf("hashdb_flush_binlog return:%d\n", hashdb_flush_binlog(hdb));

    for(; i< 10000; i++)
    {
        j = 0xffffff - i;
        snprintf(key, sizeof(key), "key_%05d%05x", i, j);
        snprintf(val, sizeof(val), "val_%05d%05x", i, j);

        ret = hashdb_insert(hdb, key, strlen(key), val, strlen(val));
        if(ret)
        {
            printf("insert error:%s, key:%s val:%s\n", hashdb_code_to_string(ret), key, val);
        }
    }
    printf("insert ok\n");

    for(i=0; i< 10000; i++)
    {
        j = 0xffffff - i;
        snprintf(key, sizeof(key), "key_%05d%05x", i, j);
        snprintf(val, sizeof(val), "val_%05d%05x", i, j);

        ret =  hashdb_find(hdb, key, strlen(key),  (void **)&pval);
        if(ret)
        {
            printf("find %s error:%s\n", key, hashdb_code_to_string(ret));
        }
        else if(memcmp(pval, val, strlen(val)))
        {
            printf("cache error, key:%s, pval:%s val:%s\n", key, pval, val);
        }
    }
    printf("insert check over\n");

    //更新测试
    for(i=0; i< 10000; i++)
    {
        j = 0xffffff - i;
        snprintf(key, sizeof(key), "key_%05d%05x", i, j);
        snprintf(val, sizeof(val), "val_%05d%05x", i, i);

        ret = hashdb_update(hdb, key, strlen(key), val, strlen(val));
        if(ret)
        {
            printf("insert error:%s, key:%s val:%s\n", hashdb_code_to_string(ret), key, val);
        }
    }

    printf("update ok\n");

    for(i=0; i< 10000; i++)
    {
        j = 0xffffff - i;
        snprintf(key, sizeof(key), "key_%05d%05x", i, j);
        snprintf(val, sizeof(val), "val_%05d%05x", i, i);

        ret =  hashdb_find(hdb, key, strlen(key),  (void **)&pval);
        if(ret)
        {
            printf("find %s error:%s\n", key, hashdb_code_to_string(ret));
        }
        else if(memcmp(pval, val, strlen(val)))
        {
            printf("cache error, key:%s, pval:%s val:%s\n", key, pval, val);
        }
    }
    printf("update check over\n");


}







int main(int argc, char *argv[])
{
    hashdb_t *map = NULL;

    openlog("hashdb", LOG_PID, LOG_MAIL);

    if(hashdb_init("./test.db", &map) == MRT_ERR)
    {
        printf("create hashdb error:%m\n");
        return -1;
    }


    hash_test_func(map);


    return HDB_OK;
}

#endif



