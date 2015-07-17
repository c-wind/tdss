#include <sys/time.h>
#include <sys/sendfile.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "name_server.h"
#include "fblock_mem.h"


extern data_server_conf_t ds_conf;
extern name_server_conf_t ns_conf;



#define slog_info(fmt, ...) log_info("%x"fmt, ss->id, ##__VA_ARGS__)
#define slog_error(fmt, ...) log_error("%x"fmt, ss->id, ##__VA_ARGS__)
#define slog_debug(fmt, ...) log_debug("%x"fmt, ss->id, ##__VA_ARGS__)






int cmd_nofound(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;

    slog_error("command error");

    session_return("%d command no found", ERR_CMD_NOFOUND);
}



int file_info_add(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    rq_arg_t rq[] = {
        {RQ_TYPE_INT,   "server",   &ss->fb.server, 0},
        {RQ_TYPE_STR,   "name",     ss->fb.name,    sizeof(ss->fb.name)},
        {RQ_TYPE_STR,   "file",     ss->fb.file,    sizeof(ss->fb.file)},
        {RQ_TYPE_INT,   "offset",   &ss->fb.offset, 0},
        {RQ_TYPE_INT,   "size",     &ss->fb.size,   0}
    };

    if(request_parse(&ss->input, rq, 5) == MRT_ERR)
    {
        log_error("command error, recv:%s.", ss->input.str);
        session_return("%d request parse error", ERR_CMD_ARG);
    }

    if(fblock_mem_add(&ss->fb) == MRT_ERR)
    {
        slog_error("fblock_mem_add error");
        session_return("%d mem add error", ERR_FB_ADD);
    }

    log_debug("server_type:%d", ns_conf.server_type);

    if(ns_conf.server_type == 2)
    {
        session_return("%d ok", OPERATE_SUCCESS);
    }
    else
    {
        if(name_server_sync(it) == MRT_ERR)
        {
            slog_error("name_server_sync error");
            session_return("%d mem sync error", ERR_FB_ADD);
        }
    }

    return SESSION_WAIT;
}

int file_info_set(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    rq_arg_t rq[] = {
        {RQ_TYPE_INT,   "server",   &ss->fb.server, 0},
        {RQ_TYPE_STR,   "name",     ss->fb.name,    sizeof(ss->fb.name)},
        {RQ_TYPE_STR,   "file",     ss->fb.file,    sizeof(ss->fb.file)},
        {RQ_TYPE_INT,   "offset",   &ss->fb.offset, 0},
        {RQ_TYPE_INT,   "size",     &ss->fb.size,   0}
    };

    if(request_parse(&ss->input, rq, 5) == MRT_ERR)
    {
        log_error("command error, recv:%s.", ss->input.str);
        session_return("%d request parse error", ERR_CMD_ARG);
    }

    if(fblock_mem_set(&ss->fb) == MRT_ERR)
    {
        slog_error("fblock_mem_add error");
        session_return("%d mem add error", ERR_FB_ADD);
    }

    log_debug("server_type:%d", ns_conf.server_type);

    if(ns_conf.server_type == 2)
    {
        session_return("%d ok", OPERATE_SUCCESS);
    }
    else
    {
        if(name_server_sync(it) == MRT_ERR)
        {
            slog_error("name_server_sync error");
            session_return("%d mem sync error", ERR_FB_ADD);
        }
    }

    return SESSION_WAIT;
}



int file_info_get(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    fblock_mem_t *fbm = NULL;
    rq_arg_t rq[] = {
        {RQ_TYPE_STR,   "name",     ss->fb.name,    33}
    };

    if(request_parse(&ss->input, rq, 1) == MRT_ERR)
    {
        log_error("command error.");
        session_return("%d request parse error", ERR_CMD_ARG);
    }

    if(fblock_mem_get(&ss->fb) == MRT_ERR)
    {

        slog_error("fblock_mem_get error");
        session_return("%d mem get error", ERR_FB_GET);
    }

    session_return("%d server=%d file=%s name=%s offset=%u size=%u ref=%u",
                   OPERATE_SUCCESS, ss->fb.server, ss->fb.file, ss->fb.name, ss->fb.offset, ss->fb.size, ss->fb.ref);
}


int file_ref_dec(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    rq_arg_t rq[] = {
        {RQ_TYPE_STR, "name", ss->fb.name, 33},
    };

    if(request_parse(&ss->input, rq, 7) == MRT_ERR)
    {
        log_error("command error.");
        session_return("%d request parse error", ERR_CMD_ARG);
    }

    if(fblock_ref_dec(ss->fb.name) == MRT_ERR)
    {
        slog_error("fblock_ref_dec error");
        session_return("%d ref dec error", ERR_FB_REF_DEC);
    }

    if(ns_conf.server_type == 2)
    {
        session_return("%d ok", OPERATE_SUCCESS);
    }

    if(name_server_sync(it) == MRT_ERR)
    {
        slog_error("name_server_sync error");
        session_return("%d mem sync error", ERR_FB_ADD);
    }

    return SESSION_WAIT;
}

int file_ref_inc(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    rq_arg_t rq[] = {
        {RQ_TYPE_STR, "name", ss->fb.name, 33},
    };

    if(request_parse(&ss->input, rq, 7) == MRT_ERR)
    {
        log_error("command error.");
        session_return("%d request parse error", ERR_CMD_ARG);
    }

    if(fblock_ref_inc(ss->fb.name) == MRT_ERR)
    {
        slog_error("fblock_ref_inc error");
        session_return("%d ref inc error", ERR_FB_REF_DEC);
    }

    if(ns_conf.server_type == 2)
    {
        session_return("%d ok", OPERATE_SUCCESS);
    }

    if(name_server_sync(it) == MRT_ERR)
    {
        slog_error("name_server_sync error");
        session_return("%d mem sync error", ERR_FB_ADD);
    }

    return SESSION_WAIT;
}


int status(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;

    name_server_info_t nsi = {0};

    if(fblock_mem_status(&nsi) == MRT_ERR)
    {
        log_error("fblock_mem_status return error.");
        session_return("%d mem status error", ERR_FB_STATUS);
    }

    session_return("%d server_id=%d server_type=%d size=%u used=%u",
                   OPERATE_SUCCESS, ns_conf.server_id, ns_conf.server_type, nsi.size, nsi.used);
}




#define CMD_DEFINE_FLUSH(f1) {#f1, f1}

command_t cmd_list[] = {
    CMD_DEFINE_FLUSH(file_info_add),
    CMD_DEFINE_FLUSH(file_info_set),
    CMD_DEFINE_FLUSH(file_info_get),
    CMD_DEFINE_FLUSH(file_ref_dec),
    CMD_DEFINE_FLUSH(file_ref_inc),
    CMD_DEFINE_FLUSH(status),
    {NULL, cmd_nofound}
};


