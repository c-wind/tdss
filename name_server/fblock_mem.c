#include <sys/time.h>
#include <sys/sendfile.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "hashdb.h"
#include "name_server.h"
#include "fblock_mem.h"


//全局变量, 所有打开的文件都在这里面
static hashdb_t *fbdb = NULL;




int fblock_mem_delete(fblock_mem_t *fbm, char *name)
{
    int iret = 0;

    log_info("Del name:[%s] file:[%x.%x] ref:[%u] server:[%u] offset:[%u] size:[%u]",
             name, fbm->prefix, fbm->suffix, fbm->ref, fbm->server, fbm->offset, fbm->size);

    iret = hashdb_delete(fbdb, name, strlen(name));
    if(iret != HDB_OK)
    {
        log_error("hashdb delete name:%s error:%s",
                  name, hashdb_code_to_string(iret));

        return MRT_ERR;
    }

    return MRT_SUC;
}


//引用记数+1
//返回：
//      -1:系统错误
//      0:没找到文件
//      1:修改成功
int fblock_ref_inc(char *name)
{
    fblock_mem_t *fbm = NULL;

    switch(hashdb_find(fbdb, name, strlen(name), (void **)&fbm))
    {
    case HDB_KEY_NOFOUND:
        log_info("hashdb no found file:%s", name);
        return 0;
    case HDB_OK:
        log_debug("hashdb find %s", name);
        break;
    default:
        log_error("hashdb find %s error", name);
        return MRT_ERR;
    }

    fbm->ref++;

    log_debug("name:%s ref:%d.", name, fbm->ref);

    return 1;
}


//引用记数-1
//返回：
//      -1:系统错误
//      0:没找到name
//      1:修改成功
int fblock_ref_dec(char *name)
{
    fblock_mem_t *fbm = NULL;

    if(hashdb_find(fbdb, name, strlen(name), (void **)&fbm) == HDB_KEY_NOFOUND)
    {
        log_info("no found file:%s", name);
        return 0;
    }

    log_debug("find file:%s in mem", name);

    fbm->ref--;

    log_debug("file:%s ref:%d.", name, fbm->ref);

    if(fbm->ref < 1)
    {
        fblock_mem_delete(fbm, name);
    }

    return 1;
}



int fblock_to_mem(fblock_t *fb, fblock_mem_t *fbm)
{
    if(sscanf(fb->file, "%x.%x", &fbm->prefix, &fbm->suffix) != 2)
    {
        log_error("fblock name:(%s) format error", fb->file);
        return MRT_ERR;
    }

    fbm->offset = (uint32_t)fb->offset;
    fbm->server = (uint16_t)fb->server;
    fbm->size = (uint32_t)fb->size;
    fbm->ref = 1;

    log_debug("fbm name:%s file:%x.%x offset:%d server:%d size:%d",
              fb->name, fbm->prefix, fbm->suffix, fbm->offset, fbm->server, fbm->size);
    return MRT_OK;
}



int fblock_mem_add(fblock_t *fb)
{
    fblock_mem_t fbm = {0};
    int iret = HDB_OK;

    if(fblock_to_mem(fb, &fbm) == -1)
    {
        log_error("block error");
        return MRT_ERR;
    }

    iret = hashdb_insert(fbdb, fb->name, strlen(fb->name), &fbm, sizeof(fbm));
    switch(iret)
    {
    case HDB_KEY_EXIST:
        log_debug("hashdb exist name:%s, will inc ref", fb->name);
        return (fblock_ref_inc(fb->name) == 1) ? MRT_OK : MRT_ERR;
    case HDB_OK:
        log_debug("hashdb insert ok, name:%s", fb->name);
        return MRT_OK;
    }

    log_error("hashdb_insert error:%s key:%s", hashdb_code_to_string(iret), fb->name);
    return MRT_ERR;
}



int fblock_mem_get(fblock_t *fb)
{
    M_cpvril(fb);
    fblock_mem_t *fbm = NULL;

    if(hashdb_find(fbdb, fb->name, strlen(fb->name), (void **)&fbm) != HDB_OK)
    {
        log_info("hashdb no found name:%s", fb->name);
        return MRT_ERR;
    }

    snprintf(fb->file, sizeof(fb->file) -1, "%x.%x", fbm->prefix, fbm->suffix);
    fb->server = fbm->server;
    fb->offset = fbm->offset;
    fb->size = fbm->size;
    fb->ref = fbm->ref;

    return MRT_OK;
}

int fblock_mem_set(fblock_t *fb)
{
    fblock_mem_t *fbm = NULL;
    fblock_mem_t fbml = {0};
    int ref = 0;

    if(fblock_to_mem(fb, &fbml) == -1)
    {
        log_error("block error");
        return MRT_ERR;
    }

    if(hashdb_find(fbdb, fb->name, strlen(fb->name), (void **)&fbm) == 0)
    {
        log_info("no found file:%s", fb->name);
        return 0;
    }

    ref = fbm->ref;

    memcpy(fbm, &fbml, sizeof(fbm));

    fbm->ref = ref;

    return MRT_OK;
}

/* ns_info_t存放每个name_server当前状态信息
   typedef struct
   {
   int                 state;
   ip4_addr_t          server;
   uint8_t             server_id;
   uint8_t             server_type;
   uint32_t            size;           //存储了多少条记录
   uint32_t            used;           //存储了多少条记录

   }name_server_info_t;

*/

int fblock_mem_status(name_server_info_t *nsi)
{
    if(!fbdb)
    {
        log_error("fblock hashdb is null");
        return MRT_ERR;
    }

    nsi->size = fbdb->size;
    nsi->used = fbdb->used;

    return MRT_OK;
}



int fblock_mem_init(char *fname)
{
    int iret = 0;

    iret = hashdb_init(fname, &fbdb);
    if(iret != HDB_OK)
    {
        log_error("hashdb_init error:%s file:%s", hashdb_code_to_string(iret), fname);
        return MRT_ERR;
    }

    return MRT_OK;
}

