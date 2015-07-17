#ifndef __FBLOCK_MEM_H__
#define __FBLOCK_MEM_H__

/*
typedef struct
{
    uint16_t    server;     //服务器ID, 最多65536台服务器
    uint32_t    prefix;     //前缀邮时间生成
    uint16_t    suffix;     //后缀由随机序号生成
    uint32_t    offset;     //偏移
}data_node_t;
*/

//内存中使用的文件信息
typedef struct
{
    uint16_t    server;     //服务器ID, 最多65536台服务器
    uint32_t    ref;        //引用次数
    uint32_t    size;       //真实文件大小（不是块文件大小）
    uint32_t    prefix;     //前缀邮时间生成
    uint16_t    suffix;     //后缀由随机序号生成
    uint32_t    offset;     //偏移
}fblock_mem_t;





int fblock_ref_inc(char *fid);

int fblock_ref_dec(char *fid);

int fblock_to_mem(fblock_t *fb, fblock_mem_t *fbm);

int fblock_mem_add(fblock_t *fb);

//在内存中查找文件索引信息
int fblock_mem_get(fblock_t *fb);

int fblock_mem_set(fblock_t *fb);

int fblock_mem_status(name_server_info_t *nsi);

int fblock_mem_init(char *fname);

#endif

