#include <sys/time.h>
#include <sys/sendfile.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "data_server.h"
#include "data_file.h"
#include "name_server_api.h"


extern data_server_conf_t ds_conf;


int mail_save(inet_task_t *it);


#define slog_info(fmt, ...) log_info("%x"fmt, ss->id, ##__VA_ARGS__)
#define slog_error(fmt, ...) log_error("%x"fmt, ss->id, ##__VA_ARGS__)
#define slog_debug(fmt, ...) log_debug("%x"fmt, ss->id, ##__VA_ARGS__)


int session_write_file(session_t *ss, char *str, int size)
{
    int lsize, csize;

    if(ss->proc_size + size > ss->fb_info.size)
    {
        slog_error("recv size:%jd expect size:%d", ss->proc_size + size, ss->fb_info.size);
        return MRT_ERR;
    }

    if(lseek(ss->bf->fd, ss->fb_info.offset + ss->proc_size, SEEK_SET) == -1)
    {
        log_error("lseek file:%s offset:%u error:%m", ss->bf->name, ss->proc_size);
        return MRT_ERR;
    }

    log_info("+++++ str size:%d, offset:%u", size,  ss->fb_info.offset +ss->proc_size);
    while(size > 0)
    {
        lsize = sizeof(ss->bf_buf) - ss->bf_size;
        csize = lsize > size ? size : lsize;

        memcpy(ss->bf_buf + ss->bf_size, str, csize);

        ss->bf_size += csize;
        size -= csize;
        str += csize;
//        ss->proc_size += csize;

        //如果当前缓冲满了，或者当前处理的大小加上缓冲的大小正好和文件大小相同
        if(ss->bf_size == sizeof(ss->bf_buf) || (ss->fb_info.size == (ss->proc_size + ss->bf_size)))
        {
            if(write(ss->bf->fd, ss->bf_buf, ss->bf_size) == -1)
            {
                log_error("write file:%s error:%m", ss->bf->name);
                return MRT_ERR;
            }
            ss->proc_size += ss->bf_size;
            ss->bf_size = 0;
        }
        log_debug("bf_size:%d, proc_size:%d", ss->bf_size, ss->proc_size);
    }

    return MRT_OK;
}




static int __file_add_loop_recv(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    ssize_t ssize = 0;
    char line[MAX_LINE]  =  {0};

    while(1)
    {
        ssize = read(it->fd, line, sizeof(line));
        if(ssize == -1)
        {
            if(errno == EAGAIN)
                break;
            if(errno == EINTR)
                continue;
            slog_error("read from fd:%d error:%m", it->fd);
            block_file_close(ss->bf, ss->fb_info.offset, ss->fb_info.size, -1);
            return MRT_ERR;
        }

        if(session_write_file(ss, line, ssize))
        {
            slog_error("session_write_file error.");
            block_file_close(ss->bf, ss->fb_info.offset, ss->fb_info.size, -1);
            return MRT_ERR;
        }
    }

    if(ss->fb_info.size != ss->proc_size)
    {
        log_info("file:%s recv:%u size:%u", ss->fb_info.name, ss->proc_size, ss->fb_info.size);
        return NEXT_READ;
    }

    block_file_close(ss->bf, 0, 0, 0);
    ss->proc_size = 0;
    memset(ss->bf_buf, 0, sizeof(ss->bf_buf));
    ss->bf_size = 0;



    ss->next = SESSION_BEGIN;

    if(ns_file_info_add(it)  == MRT_ERR)
    {
        slog_error("ns_save_mid error");
        session_return("%d name server connect error", ERR_NS_CONN);
    }

//    ss->state = SESSION_WAIT;
    //session_return("%d save success", OPERATE_SUCCESS);
    return SESSION_WAIT;
}

static int __file_get_loop_send(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    ssize_t ssize = 0;
    off_t of = ss->fb_info.offset;

    if(lseek(ss->bf->fd, of, SEEK_SET) == -1)
    {
        log_error("lseek file:%s offset:%u error:%m", ss->bf->name, ss->proc_size);
        return MRT_ERR;
    }

    while(ss->fb_info.size > ss->proc_size)
    {
        log_info("1111111  fd1:%d fd2:%d off:%u size:%u",it->fd, ss->bf->fd, of, ss->fb_info.size - ss->proc_size);
        ssize = sendfile(it->fd, ss->bf->fd, &of, ss->fb_info.size - ss->proc_size);
        if(ssize == -1)
        {
            if(errno == EAGAIN)
                break;
            if(errno == EINTR)
                continue;
            slog_error("sendfile from fd:%d to fd:%d error:%m", ss->bf->fd, it->fd);
            session_return("%d sendfile error:%m", ERR_OPEN_FILE);
        }
        log_info("22222 fd1:%d fd2:%d off:%u size:%u, ssize:%d",it->fd, ss->bf->fd, of, ss->fb_info.size - ss->proc_size, ssize);
        ss->proc_size += ssize;
    }

    if(ss->fb_info.size != ss->proc_size)
    {
        slog_debug("send name:%s to:%s all size:%u send_size:%u",
                   ss->fb_info.name, it->from, ss->fb_info.size, ss->proc_size);
        return NEXT_WRITE;
    }

    slog_info("send over name:%s to:%s size:%d success, will close file", ss->fb_info.name, it->from, ss->proc_size);

    block_file_close(ss->bf, 0, 0, 0);

    slog_info("send name:%s to:%s size:%d success, close file over", ss->fb_info.name, it->from, ss->proc_size);

    ss->state = SESSION_BEGIN;
    ss->proc_size = 0;
    s_zero(ss->fb_info);
    /////////////////
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    /////

    return SESSION_BEGIN;
}




int cmd_nofound(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;

    slog_error("command error");

    session_return("%d command no found", ERR_CMD_NOFOUND);
}





int file_add(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    rq_arg_t rq[] = {{RQ_TYPE_STR, "name", ss->fb_info.name, 33}, {RQ_TYPE_INT, "size", (void *)&ss->fb_info.size, 0}};

    if(ss->state == SESSION_READ)
    {
        slog_debug("will loop recv mail body.");
        return __file_add_loop_recv(it);
    }

    if(request_parse(&ss->input, rq, 2) == -1)
    {
        slog_error("request_parse error, recv:%s", ss->input.str);
        session_return("%d", ERR_CMD_ARG);
    }


    if(block_file_append(&ss->bf, ss->fb_info.size, &ss->fb_info.offset) == MRT_ERR)
    {
        slog_error("block_file_append error:%m");
        session_return("%d open file error:%m", ERR_OPEN_FILE);
    }

    snprintf(ss->fb_info.file, sizeof(ss->fb_info.file) - 1, "%s", ss->bf->name);

    memset(ss->bf_buf, 0, sizeof(ss->bf_buf));

    slog_info("%x name:%s file:%s offset:%u size:%d",
              ss->id, ss->fb_info.name, ss->fb_info.file, ss->fb_info.offset, ss->fb_info.size);

    ss->next = SESSION_READ;

    session_return("%d file:%s create", OPERATE_SUCCESS, ss->bf->name);
}


int file_get(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    rq_arg_t rq[] = {
        {RQ_TYPE_STR, "name", ss->fb_info.name,     sizeof(ss->fb_info.name)},
        {RQ_TYPE_STR, "file", ss->fb_info.file,     sizeof(ss->fb_info.file)},
        {RQ_TYPE_INT, "offset", (void *)&ss->fb_info.offset, 0},
        {RQ_TYPE_INT, "size", (void *)&ss->fb_info.size, 0}
    };

    if(ss->state == SESSION_WRITE)
    {
        slog_debug("will loop send file body.");
        return __file_get_loop_send(it);
    }

    if(request_parse(&ss->input, rq, 4) == -1)
    {
        slog_error("request_parse error.");
        session_return("%d", ERR_CMD_ARG);
    }

    slog_info("%x name:%s file:%s offset:%u size:%d",
              ss->id, ss->fb_info.name, ss->fb_info.file, ss->fb_info.offset, ss->fb_info.size);

    if(block_file_open(&ss->bf, ss->fb_info.file) == MRT_ERR)
    {
        slog_error("block_file_open file:%s error:%m", ss->fb_info.name);
        session_return("%d open file%s error:%m", ERR_OPEN_FILE, ss->fb_info.name);
    }

    ss->proc_size = 0;
    snprintf(ss->fb_info.file, sizeof(ss->fb_info.file) - 1, "%s", ss->bf->name);
    ss->next = SESSION_WRITE;

    log_info("name:%s file:%s offset:%u size:%u", ss->fb_info.name, ss->fb_info.file, ss->fb_info.offset, ss->fb_info.size);

    session_return("%d open file:%s success", OPERATE_SUCCESS, ss->bf->name);
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
    CMD_DEFINE_FLUSH(file_add),
    CMD_DEFINE_FLUSH(file_get),
    CMD_DEFINE_FLUSH(status),
    {NULL, cmd_nofound}
};


