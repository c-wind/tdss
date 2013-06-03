#include <sys/time.h>
#include <sys/sendfile.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "master.h"


extern data_server_conf_t ds_conf;
extern name_server_conf_t ns_conf;
extern master_server_conf_t ms_conf;



#define slog_info(fmt, ...) log_info("%x"fmt, ss->id, ##__VA_ARGS__)
#define slog_error(fmt, ...) log_error("%x"fmt, ss->id, ##__VA_ARGS__)
#define slog_debug(fmt, ...) log_debug("%x"fmt, ss->id, ##__VA_ARGS__)






int cmd_nofound(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;

    slog_error("command error");

    session_return("%d command no found", ERR_CMD_NOFOUND);
}



int server_find(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    char fname[33] = {0};
    uint32_t size = 0, i = 0, nsize = 0, ns_id = 0;
    data_server_info_t *dsi = NULL;

    rq_arg_t rq[] = {
        {RQ_TYPE_STR,   "name",     fname,  33},
        {RQ_TYPE_INT,   "size",     &size,  0}
    };

    if(request_parse(&ss->input, rq, 2) == MRT_ERR)
    {
        log_error("command error.");
        session_return("%d request parse error", ERR_CMD_ARG);
    }

    log_debug("recv name:%s size:%d", fname, size);

    //从字节变成MB + 1024
    nsize = 1024 + (size >> 20);

    //如果返回-1说明这个文件名不是MD5文件之后的16进制结果
    if(ns_get_server_id(fname, (int *)&ns_id) == MRT_ERR)
    {
        log_error("fname:%s format error", fname);
        session_return("%d file name must is hex string", ERR_NS_FNAME);
    }

    //根据文件计算出来的name server服务器存在问题
    if(ms_conf.name_server.info[ns_id].state == SERVER_OFFLINE)
    {
        log_error("name server:(%s:%d) unavailable, file:%s",
                  ms_conf.name_server.info[ns_id].addr.ip, ms_conf.name_server.info[ns_id].addr.port, fname);
        session_return("%d name server Unavailable", ERR_NS_OFFLINE);
    }

    //循环检测所有data server
    for(i=0; i<ms_conf.data_server.count; i++)
    {
        ms_conf.data_server.work_id = (ms_conf.data_server.work_id + 1) % ms_conf.data_server.count;

        dsi = &ms_conf.data_server.info[ms_conf.data_server.work_id];

        //如果服务存在问题就跳过
        if(dsi->state == SERVER_OFFLINE)
        {
            log_info("jump offline server:(%s:%d)",
                     dsi->addr.ip, dsi->addr.port);
            continue;
        }

        //如果可用空间小于1GB+当前要存储的文件大小就跳过
        if(dsi->disk_free < nsize)
        {
            log_info("jump Insufficient Storage server:(%s:%d) disk size:%uMB free:%uMB file size:%uMB",
                     dsi->addr.ip, dsi->addr.port, dsi->disk_size, dsi->disk_free, size);
            continue;
        }

        //如果inode少于1024也跳过
        if(dsi->inode_free < 1024)
        {
            log_info("jump Insufficient Inode server:(%s:%d) inode size:%u free:%u",
                     dsi->addr.ip, dsi->addr.port, dsi->inode_size, dsi->inode_free);
            continue;
        }

        log_info("server:(%s:%d) will recv file:%s, size:%u", dsi->addr.ip, dsi->addr.port, fname, size);

        session_return("%d server=%s port=%d", OPERATE_SUCCESS, dsi->addr.ip, dsi->addr.port);
    }

    log_error("no found active server, fname:%s size:%u", fname, size);

    session_return("%d Try again later", ERR_DS_FULL);
}



int status(inet_task_t *it)
{
    string_t res = {0};
    int i=0;
    char line[MAX_LINE] = {0};
    session_t *ss = (session_t *)it->data;

    for(i=0; i<ms_conf.data_server.count; i++)
    {
        snprintf(line, sizeof(line) - 1,
                 "\ndata_server[%d]: \n\tstatus:%d\tdisk size:%uMB free:%uMB\tinode size:%u free:%u",
                 i, ms_conf.data_server.info[i].state, ms_conf.data_server.info[i].disk_size, ms_conf.data_server.info[i].disk_free,
                 ms_conf.data_server.info[i].inode_size, ms_conf.data_server.info[i].inode_free);
        string_cats(&res, line);
    }

    for(i=0; i<ms_conf.name_server.count; i++)
    {
        snprintf(line, sizeof(line) - 1,
                 "\nname_server[%d]: \n\tstatus:%d\tsize:%u\tused:%u\n",
                 i, ms_conf.name_server.info[i].state, ms_conf.name_server.info[i].size, ms_conf.name_server.info[i].used);
        string_cats(&res, line);
    }

    session_return("%d success%s", OPERATE_SUCCESS, res.str);
}




#define CMD_DEFINE_FLUSH(f1) {#f1, f1}

command_t cmd_list[] = {
    CMD_DEFINE_FLUSH(server_find),
    CMD_DEFINE_FLUSH(status),
    {NULL, cmd_nofound}
};


