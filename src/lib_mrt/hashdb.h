#ifndef __HASHDB_H__
#define __HASHDB_H__

//2000以后开头的是不可恢复错误
#define HDB_ERR_NOMEM   -2000
#define HDB_ERR_FWRITE  -2001
#define HDB_ERR_FREAD   -2002
#define HDB_ERR_FFLUSH  -2003
#define HDB_ERR_FOPEN   -2004

#define HDB_OK   0

#define HDB_KEY_EXIST   -1000
#define HDB_KEY_NOFOUND -1001
#define HDB_ARG_INVAL   -1002

/* Structure of one hash table entry. */
typedef struct hashdb_entry_s hashdb_entry_t;

struct hashdb_entry_s
{
    char                *key;			/* lookup key */
    uint16_t            key_len;

    char                *val;
    uint16_t            val_len;
    uint8_t             val_type:1;       //1:是push进来的，删除时不需要释放

    uint32_t            hash_code;		/* hash code */

    hashdb_entry_t     *next;		    /* colliding entry */
    hashdb_entry_t     *prev;		    /* colliding entry */
};

/* Structure of one hash table. */
typedef struct hashdb_t
{
    int                 stat;           //启动成功后为1
    uint32_t            size;			/* length of entries array */
    uint16_t            used;			/* number of entries in table */
    uint8_t             idx;            /* primes id */
    hashdb_entry_t     **data;		    /* entries array, auto-resized */

    char                bname[128];     //操作日志文件名
    FILE                *bfp;           //记录操作日志
    char                fbuf[BUFSIZ];   //文件操作缓冲

}hashdb_t;

typedef struct
{
    uint8_t     type; //1:添加, 2:修改, 3:删除
    uint16_t    klen; //key长度
    uint16_t    vlen; //val长度
}hashdb_sec_t;

//参数：
//      fname: binlog文件名
//      phmp: 创建成功后返回的hashdb
//返回：
//      0：成功
//      -1：出错

int hashdb_init(char *fname, hashdb_t **phdb, int (*load)(uint8_t type, char *key, uint16_t klen, void *val, uint16_t vlen));

int hashdb_insert(hashdb_t *map, char *key, uint16_t key_len, void *val, uint16_t val_len);

int hashdb_find(hashdb_t *map, char *key, int key_len, void **val);

void hashdb_free(hashdb_t *table);

int hashdb_delete(hashdb_t *table, char *key, int key_len);

char *hashdb_code_to_string(int code);

#endif
