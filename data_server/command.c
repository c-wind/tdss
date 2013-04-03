#include <sys/time.h>
#include <sys/sendfile.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "data_file.h"
#include "data_server.h"
#include "name_server_api.h"


extern data_server_conf_t ds_conf;


int mail_save(inet_task_t *it);


#define slog_info(fmt, ...) log_info("%x"fmt, ss->id, ##__VA_ARGS__)
#define slog_error(fmt, ...) log_error("%x"fmt, ss->id, ##__VA_ARGS__)
#define slog_debug(fmt, ...) log_debug("%x"fmt, ss->id, ##__VA_ARGS__)






static int __file_save_loop_recv(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    ssize_t ssize = 0;
    char line[MAX_LINE]  =  {0};

    while(ss->fb.size > ss->tmp_file->size)
    {
        ssize = read(it->fd, line, sizeof(line));
        if(ssize == -1)
        {
            if(errno == EAGAIN)
                break;
            if(errno == EINTR)
                continue;
            slog_error("read from fd:%d error:%m", it->fd);
            return -1;
        }
        if(file_buffer_write(ss->tmp_file, line, ssize))
        {
            slog_error("file_buffer_write error.");
            return -1;
        }
        ss->tmp_file->size += ssize;
    }

    if(ss->fb.size > ss->tmp_file->size)
    {
        slog_debug("recv size:%jd expect size:%d", ss->tmp_file->size, ss->fb.size);
        return NEXT_READ;
    }

    file_close(ss->tmp_file);

    slog_info("recv mid:%s to:%s over, size:%jd", ss->fb.name, ss->tmp_file->from, ss->tmp_file->size);
    snprintf(line, sizeof(line), "data/%s", ss->fb.name);
    if(link(ss->tmp_file->from, line) == -1)
    {
        slog_error("link from:%s to:%s error:%m", ss->tmp_file->from, line);
        ss->next = SESSION_BEGIN;
        session_return("%d link file error:%m", ERR_LINK_FILE);
    }

    //删除失败只记录不返回出错
    if(unlink(ss->tmp_file->from) == -1)
    {
        slog_error("unlink file:%s error:%m", ss->tmp_file->from);
    }


    ss->next = SESSION_BEGIN;

    if(ns_file_info_add(it)  == MRT_ERR)
    {
        slog_error("ns_save_mid error");
        session_return("%d name server connect error", ERR_NS_CONN);
    }

    ss->state = SESSION_WAIT;
    //session_return("%d save success", OPERATE_SUCCESS);
    return 0;
}

static int __file_read_loop_send(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    ssize_t ssize = 0;

    while(ss->fb.size > ss->send_size)
    {
        ssize = sendfile(it->fd, ss->src_file->fd, &ss->offset, ss->fb.size - ss->send_size);
        if(ssize == -1)
        {
            if(errno == EAGAIN)
                break;
            if(errno == EINTR)
                continue;
            slog_error("sendfile from fd:%d to fd:%d error:%m", ss->src_file->fd, it->fd);
            return -1;
        }
        ss->send_size += ssize;
    }

    if(ss->fb.size > ss->send_size)
    {
        slog_debug("send size:%d expect size:%d", ss->send_size, ss->fb.size);
        return NEXT_WRITE;
    }

    ds_close_file(ss->src_file);

    slog_info("send mid:%s to:%s size:%d success", ss->fb.name, it->from, ss->send_size);

    return NEXT_READ;
}




int cmd_nofound(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;

    slog_error("command error");

    session_return("%d command no found", ERR_CMD_NOFOUND);
}





int file_save(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    rq_arg_t rq[] = {{RQ_TYPE_STR, "mid", ss->fb.name, 33}, {RQ_TYPE_INT, "size", (void *)&ss->fb.size, 0}};

    if(ss->state == SESSION_READ)
    {
        slog_debug("will loop recv mail body.");
        return __file_save_loop_recv(it);
    }

    if(request_parse(&ss->input, rq, 2) == -1)
    {
        slog_error("request_parse error, recv:%s", ss->input.str);
        session_return("%d", ERR_CMD_ARG);
    }

    slog_info("%x mid:%s, size:%d", ss->id, ss->fb.name, ss->fb.size);

    if(ss->tmp_file)
    {
        slog_debug("clear old tmp file var.");
        p_zero(ss->tmp_file);
    }
    else
    {
        if(!(ss->tmp_file = M_alloc(sizeof(file_handle_t))))
        {
            slog_error("M_alloc file handle error:%m");
            session_return("%d malloc error:%m", ERR_NOMEM);
        }
    }

    if(file_open_temp("./tmp/", ss->tmp_file) == -1)
    {
        slog_error("open_tmp_file error.");
        session_return("%d open temp file error:%m", ERR_OPEN_FILE);
        return NEXT_WRITE;
    }

    if(file_buffer_init(ss->tmp_file, M_8KB))
    {
        slog_error("file_buffer_init error.");
        session_return("%d file buffer init error.", ERR_OPEN_FILE);
    }

    slog_info("create temp file:%s", ss->tmp_file->from);

    ss->next = SESSION_READ;

    session_return("%d temp file:%s create", OPERATE_SUCCESS, ss->tmp_file->from);
}


int file_read(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    rq_arg_t rq[] = {
        {RQ_TYPE_STR, "mid", ss->fb.name, 33},
        {RQ_TYPE_INT, "offset", (void *)&ss->offset, 0},
        {RQ_TYPE_INT, "size", (void *)&ss->fb.size, 0}
    };

    if(ss->state == SESSION_WRITE)
    {
        slog_debug("will loop send mail body.");
        return __file_read_loop_send(it);
    }

    if(request_parse(&ss->input, rq, 3) == -1)
    {
        slog_error("request_parse error.");
        session_return("%d", ERR_CMD_ARG);
    }

    slog_info("%x mid:%s, size:%d", ss->id, ss->fb.name, ss->fb.size);

    if(ds_open_file(&ss->src_file, ss->fb.name) == -1)
    {
        slog_error("open file:%s error:%m", ss->fb.name);
        session_return("%d open file%s error:%m", ERR_OPEN_FILE, ss->fb.name);
    }

    slog_info("create temp file:%s", ss->src_file->from);

    ss->next = SESSION_WRITE;

    session_return("%d open file:%s success", OPERATE_SUCCESS, ss->src_file->from);
}


int status(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    data_server_info_t dsi = {0};

    disk_status(".", &dsi);

    session_return("%d server_id=%d disk_id=%d disk_size=%d disk_free=%d inode_size=%d inode_free=%d",
                   OPERATE_SUCCESS, ds_conf.server_id, ds_conf.disk_id,
                   dsi.disk_size, dsi.disk_free, dsi.inode_size, dsi.inode_free);
}


#define CMD_DEFINE_FLUSH(f1) {#f1, f1}

command_t cmd_list[] = {
    CMD_DEFINE_FLUSH(file_save),
    CMD_DEFINE_FLUSH(file_read),
    CMD_DEFINE_FLUSH(status),
    {NULL, cmd_nofound}
};


