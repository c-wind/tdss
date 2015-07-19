#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

#include <stdint.h>


typedef struct
{
    uint8_t     stat;              //当前块属性
    int32_t     blst_id;           //所属块列表ID
    int64_t     mem_size;          //分配的内存大小
    void        *data;
#ifdef MEMORY_DEBUG
    char        func[64];
    int         line;
#endif
    list_node_t node;
}block_t;


typedef struct
{
    int32_t     blst_width;         //当前块列表中元素占内存实际大小
    int32_t     blst_id;            //当前块列表ID
    pthread_mutex_t    mtx;

    list_head_t head;

}block_list_t;


typedef struct
{
    int64_t     used;               //已用内存大小
    int64_t     free;               //剩余内存大小
    void        *data;              //16MB大小的真实内存块
    list_node_t node;
}memory_t;


typedef struct
{
    int32_t     size;               //真实内存块总数
    list_head_t head;

}memory_list_t;


struct memory_pool_s
{
    int64_t     size;               //总内存大小,包含所有申请的内存大小

    memory_t    *master;            //指向内存列表中的一个内存块，如果当前使用的内存块剩余空间小于128KB
    //就申请下一个可用内存块，但当前并不使用
    memory_t    *slave;             //指向内存列表中的下一个即将使用的内存块，只做备用内存用（抢占）
    //如果当前使用的内存块（data中的剩余空间小于32KB）就激活这块内存

    memory_list_t       *memory_list;       //真实内存列表，一个满了再申请一个，只做统计数据用

    block_list_t        *free_blst;         //空闲块列表

#ifdef MEMORY_DEBUG
    block_list_t        *used_blst;         //使用中的块列表
#endif

};




// ----------- 函数声明 ----------------

int memory_pool_init();
int memory_pool_destroy();

void memory_free(void *data, int, char *);

void *memory_alloc(int64_t size, int, char *);

void *memory_realloc(void *old_data, int64_t size, int, char *);
// ------------------------------------


#endif
