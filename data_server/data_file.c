#include <sys/time.h>
#include <sys/sendfile.h>
#include <sys/vfs.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "hashdb.h"


extern name_server_conf_t ns_conf;
extern data_server_conf_t ds_conf;

#define MAX_FILE_BLOCK_SIZE     268435456


//全局变量, 所有打开的文件都在这里面
static hashdb_t *ofdb = NULL;



int ds_open_file(file_handle_t **fh, char *name)
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




int ds_close_file(file_handle_t *fh)
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


