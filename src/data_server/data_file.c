#include <sys/time.h>
#include <sys/sendfile.h>
#include <sys/vfs.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "hashdb.h"
#include "data_server.h"
#include "data_file.h"


extern name_server_conf_t ns_conf;
extern data_server_conf_t ds_conf;

#define MAX_FILE_BLOCK_SIZE     268435456


static hashdb_t *ofdb = NULL;       //所有打开的文件都在这里面

static block_file_t *cbf = NULL;


int ds_file_open(file_handle_t **fh, char *name)
{
    file_handle_t *tmp = NULL;

    if(hashdb_find(ofdb, name, strlen(name), (void **)&tmp) == 0)
    {
        *fh = tmp;
        log_debug("find file:%s in ofdb", name);
        return 0;
    }

    log_debug("no found file:%s in ofdb", name);

    tmp = M_alloc(sizeof(file_handle_t));
    if(!tmp)
    {
        log_error("malloc error:%m");
        return -1;
    }

    snprintf(tmp->from, sizeof(tmp->from), "%s", name);

    if(file_open_read(tmp) == -1)
    {
        log_error("file:%s open error:%m", tmp->from);
        return -1;
    }

    log_debug("open tmp file:%s success.", tmp->from);

    *fh = tmp;

    return 0;
}



int ds_file_is_open(char *name)
{
    file_handle_t *tmp = NULL;

    if(hashdb_find(ofdb, name, strlen(name), (void **)&tmp) == 0)
    {
        log_debug("find file:%s in ofdb", name);
        return 0;
    }

    return -1;
}




int ds_file_close(file_handle_t *fh)
{
    close(fh->fd);

    if(hashdb_delete(ofdb, fh->from, strlen(fh->from)) == -1)
    {
        log_error("hashdb_insert error.");
        return -1;
    }

    return 0;
}


int ds_file_handle_init()
{
    if(hashdb_init(NULL, &ofdb) == -1)
    {
        log_error("hashdb init error.");
        return -1;
    }

    return 0;
}





//ds_info_t存放每个data_server当前状态信息
int disk_status(char *path, data_server_info_t *dsi)
{
    struct statfs st = {0};

//    int statvfs(const char *path, struct statvfs *buf);
    /*
       struct statfs {
       long    f_type;     // type of file system (see below)
       long    f_bsize;    // optimal transfer block size
       long    f_blocks;   // total data blocks in file system
       long    f_bfree;    // free blocks in fs
       long    f_bavail;   // free blocks avail to non-superuser
       long    f_files;    // total file nodes in file system
       long    f_ffree;    // free file nodes in fs
       fsid_t  f_fsid;     // file system id
       long    f_namelen;  // maximum length of filenames
       };
       */
    if(statfs(path, &st) == MRT_ERR)
    {
        log_error("statfs %s error:%m", path);
        return MRT_ERR;
    }


    dsi->disk_size = (uint32_t)(((uint64_t)st.f_bsize * st.f_blocks) >> (20));
    dsi->disk_free = (uint32_t)(((uint64_t)st.f_bsize * st.f_bavail) >> (20));

    dsi->inode_size = (uint32_t)(st.f_files);
    dsi->inode_free = (uint32_t)(st.f_ffree);


    return MRT_OK;
}


/*

#include <dirent.h>



int ds_file_merger(char *path, time_t tm)
{
    DIR *dir = NULL;
    struct dirent *elm = NULL;
    struct stat st = {0};
    time_t bt = time(NULL) - ds_conf.merger_date;
    char fpath[128], tpath[128];

    dir = opendir(path);
    if(!dir)
    {
        log_error("opendir %s error:%m", path);
        return MRT_ERR;
    }

    while((elm = readdir(dir)))
    {
        if(*elm->d_name == '.')
            continue;

        if(stat(elm->d_name, &st) == -1)
        {
            log_error("stat file:%s error:%m", elm->d_name);
            continue;
        }

        if(st.st_size > ds_conf.merger_size)
        {
            snprintf(fpath, sizeof(fpath), "%s/%s", path, elm->d_name);
            snprintf(tpath, sizeof(tpath), "merger/%s", path, elm->d_name);

            if(link(fpath, tpath) == -1)
            {
                log_error("link from:%s to:%s error:%m");
                continue;
            }

            log_info("");

        }

        log_debug("file:%s", elm->d_name);
    }

    return 0;
}
*/

//打开一个文件准备写入, 要指定写入大小，此时可能用多个文件往同一个块中写
static int block_file_create(int size)
{
    block_file_t *bf = NULL;
    int pre = time(NULL), end = 0;
    static uint16_t i=0;


    M_cvril((bf = M_alloc(sizeof(block_file_t))), "Malloc block_file_t error:%m");

    bf->state = 0;

    while(end++ < 10000)
    {
        snprintf(bf->name, sizeof(bf->name), "%s/%x.%x", ds_conf.data_path, pre, i++);
        bf->fd = open(bf->name, O_RDWR|O_EXCL|O_CREAT, 0666);
        if(bf->fd != -1)
            break;

        log_debug("file %s %m", bf->name);
    }

    if(bf->fd == -1)
    {
        log_error("can't create block file:%m");
        return MRT_ERR;
    }

    if(ftruncate(bf->fd, size) == -1)
    {
        log_error("ftruncate %s %m", bf->name);
        return MRT_ERR;
    }

    log_debug("open file:%s success.", bf->name);

    bf->state = 1;
    bf->num = 1;
    cbf = bf;

    return MRT_OK;
}

//打开一个文件准备读
int block_file_open(block_file_t **rbf, char *fname)
{
    block_file_t *bf = NULL;
    int pre = time(NULL), end = 0;
    static uint16_t i=0;

    M_cvril((bf = M_alloc(sizeof(block_file_t))), "Malloc block_file_t error:%m");

    bf->state = 0;
    snprintf(bf->name, sizeof(bf->name) -1, "%s/%s", ds_conf.data_path, fname);

    bf->fd = open(bf->name, O_RDONLY);
    if(bf->fd == -1)
    {
        log_error("open file:%s %m", bf->name);
        return MRT_ERR;
    }

    log_debug("open file:%s success.", bf->name);

    bf->state = 1;
    bf->num = 1;
    *rbf = bf;

    return MRT_OK;
}


//status
//      -1:需要删除这个块
//      0:正常关闭
//
void block_file_close(block_file_t *bf, off_t pos, int size, int status)
{
    if(status == -1)
    {
        log_info("BLOCK DELETE name:%s offset:%u, size:%u", bf->name, pos, size);
    }

    //当状态为2的时候是当前文件大小已经到达指定上限了
    if(bf->num == 0 && bf->state == 2)
    {
        log_debug("close block file:%s", bf->name);
        close(bf->fd);
        M_free(bf);
        return;
    }

    bf->num--;

    log_debug("dec block file:%s num:%d", bf->name, bf->num);
}



//打开一个文件准备写入, 要指定写入大小，此时可能用多个文件往同一个块中写
int block_file_append(block_file_t **bf, int size, off_t *pos)
{
    if(!cbf)
    {
        log_debug("+++++ no block file, create");
        //如果没有打开的文件，就创建一个
        if(block_file_create(size) == MRT_ERR)
        {
            log_fatal("can't create file");
            return MRT_ERR;
        }

        *bf = cbf;
        return MRT_OK;
    }

    if((cbf->size + size) >= ds_conf.max_block_size)
    {
        log_debug("+++++ block file size:%d max:%d, create", cbf->size, ds_conf.max_block_size);
        cbf->state = 2;
        //如果打开的文件加上当前要写的内容超过了最大文件大小, 也重新创建一个
        if(block_file_create(size) == MRT_ERR)
        {
            log_fatal("can't create file");
            return MRT_ERR;
        }
        *bf = cbf;
        return MRT_OK;
    }

    log_debug("+++++ use exist, size:%d", cbf->size);
    //如果有并发文件在打开状态并且文件大小在可扩展范围内
    if(ftruncate(cbf->fd, cbf->size + size) == -1)
    {
        log_error("ftruncate %s %m, new size:%u", cbf->name, cbf->size + size);
        return MRT_ERR;
    }

    *pos = (off_t)cbf->size;
    cbf->size += size;
    cbf->num++;

    *bf = cbf;

    return MRT_OK;
}



