#ifndef __FBLOCK_MEM_H__
#define __FBLOCK_MEM_H__

//内存中使用的文件信息
typedef struct
{
    uint16_t        ref;        //引用次数
    uint16_t        server;     //服务器ID, 最多65536台服务器
    uint64_t        name[2];    //块文件名由两部分组成一个128位的文件名, 如果是合并之后的块文件只使用name[0]
    uint32_t        size;       //真实文件大小（不是块文件大小）

}fblock_mem_t;




int fblock_ref_inc(char *fid);

int fblock_ref_dec(char *fid);

int fblock_to_mem(fblock_t *fb, fblock_mem_t *fbm);

int fblock_mem_add(fblock_t *fb);

int fblock_mem_set(fblock_t *fb);

int fblock_mem_status(name_server_info_t *nsi);

int fblock_mem_init(char *fname);

#endif

