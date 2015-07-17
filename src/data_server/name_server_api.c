#include <openssl/conf.h>
#include <libgen.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "data_server.h"


extern data_server_conf_t ds_conf;


static int __name_server_recv(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    char line[MAX_LINE] = {0};
    int i;

    while((i = read(it->fd, line, sizeof(line))))
    {
        if(i == -1)
        {
            if(errno == EAGAIN)
                break;
            if(errno == EINTR)
                continue;
            log_error("read from:%s fd:%d error:%m", it->from, it->fd);
            ss->name_server.finish(it);
            return -1;
        }

        string_catb(&ss->input, line, i);
    }

    ss->state = SESSION_PROC;

    ss->name_server.finish(it);

    return 0;
}


static int __name_server_send(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    char *pstr = NULL;
    int plen = 0, ssize = 0;

    pstr = ss->output.idx;
    plen = ss->output.len - (ss->output.idx - ss->output.str);

    while((plen > 0) && (ssize = write(it->fd, pstr, plen)))
    {
        if(ssize == -1)
        {
            if(errno == EINTR)
                continue;
            if(errno == EAGAIN)
                break;
            log_error("task:(%s) write error:%m", it->from);
            ss->name_server.finish(it);
            return -1;
        }
        pstr += ssize;
        plen -= ssize;
    }

    //如果ssize为0，说明远程端口关闭了
    if(ssize == 0)
    {
        log_info("remote:%s close.", it->from);
        ss->name_server.finish(it);
        return -1;
    }

    //如果plen为0说明发完了
    if(plen != 0)
    {
        ss->output.idx = pstr;
        it->state = TASK_WAIT_WRITE;
        return MRT_SUC;
    }

    it->func = __name_server_recv;
    it->state = TASK_WAIT_READ;

    ss->state = SESSION_READ;

    return 0;
}





int __ns_process_finish(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    inet_task_t *pit = ss->parent;
    session_t *pss = (session_t *)pit->data;
    int ec = OPERATE_SUCCESS;

    switch(ss->state)
    {
    case SESSION_WRITE:
        ec = ERR_NS_WRITE;
        goto child_fail_return;
    case SESSION_READ:
        ec = ERR_NS_READ;
        goto child_fail_return;
    }

    if(ss->input.len < 4)
    {
        log_error("%x task:(%s) return error.", ss->id, it->from);
        ec = ERR_NS_RETURN;
        goto child_fail_return;
    }

    if(strncmp(ss->input.str, "1000", 4))
    {
        log_error("%x save name:%s to:(%s) recv:%s",
                  ss->id, ss->fb_info.name, it->from, ss->input.str);
        ec = ERR_NS_PROC;
        goto child_fail_return;
    }

    log_info("%x save name:%s to:(%s) success", ss->id, pss->fb_info.name, it->from);

child_fail_return:

    if(ec == OPERATE_SUCCESS)
    {
        string_printf(&pss->output, "1000 success name:%s", pss->fb_info.name);
    }
    else
    {
        string_printf(&pss->output, "%d save to name server error", ec);
    }
    pit->state = TASK_WAIT_WRITE;
    pss->state = SESSION_REPLY;
    if(inet_event_add(ds_conf.ie, pit) == -1)
    {
        log_error("event add task:(%s) error", pit->from);
    }

    it->state = TASK_WAIT_END;

    s_zero(pss->fb_info);

    return MRT_SUC;
}


int ns_file_info_add(inet_task_t *it)
{
    int iret = 0, port = 0;
    char *ip = NULL;
    inet_task_t *nit = NULL;
    session_t *nss = NULL, *ss = (session_t *)it->data;


    iret = ns_get_master_addr(ss->fb_info.name, strlen(ss->fb_info.name),  &ip, &port);
    if(iret == MRT_ERR)
    {
        log_error("can't change name:(%s) to addr.", ss->fb_info.name);
        return MRT_ERR;
    }

    nit = inet_connect_init(ip, port);
    if(nit == NULL)
    {
        log_error("connect to:%s:%d error", ip, port);
        return -1;
    }

    nit->func = __name_server_send;
    nit->state = TASK_WAIT_WRITE;
    nss = M_alloc(sizeof(session_t));
    if(!nss)
    {
        log_error("M_alloc session error:%m");
        nit->state =  TASK_WAIT_END;
    }
    else
    {
        nss->name_server.state = 0;
        nss->name_server.finish = __ns_process_finish;
        nss->parent = it;
        nss->state = SESSION_BEGIN;

        string_printf(&nss->output, "file_info_add server=%d name=%s file=%s offset=%u size=%d\r\n",
                      ds_conf.server_id, ss->fb_info.name, basename(ss->fb_info.file), ss->fb_info.offset, ss->fb_info.size);
        log_info("call `%s`, file:%s", nss->output.str, ss->fb_info.file);
        nit->data = nss;
    }

    if(inet_event_add(ds_conf.ie, nit) == -1)
    {
        log_error("inet_event_add error");
        return MRT_ERR;
    }

    nss->state = SESSION_WRITE;

    return 0;
}




int ns_file_info_set(inet_task_t *it)
{
    int iret = 0, port = 0;
    char *ip = NULL;
    inet_task_t *nit = NULL;
    session_t *nss = NULL, *ss = (session_t *)it->data;

    iret = ns_get_master_addr(ss->fb_info.name, strlen(ss->fb_info.name), &ip, &port);
    if(iret == MRT_ERR)
    {
        log_error("can't change name:(%s) to addr.", ss->fb_info.name);
        return MRT_ERR;
    }

    nit = inet_connect_init(ip, port);
    if(nit == NULL)
    {
        log_error("connect to:%s:%d error", ip, port);
        return -1;
    }

    nit->func = __name_server_send;
    nit->state = TASK_WAIT_WRITE;
    nss = M_alloc(sizeof(session_t));
    if(!nss)
    {
        log_error("M_alloc session error:%m");
        nit->state =  TASK_WAIT_END;
    }
    else
    {
        nss->name_server.state = 0;
        nss->name_server.finish = __ns_process_finish;
        nss->parent = it;
        nss->state = SESSION_BEGIN;

        string_printf(&nss->output, "file_info_set server=%d name=%s file=%s offset=%u size=%d\r\n",
                      ds_conf.server_id, ss->fb_info.name, basename(ss->fb_info.file), ss->fb_info.offset, ss->fb_info.size);
        nit->data = nss;
    }

    if(inet_event_add(ds_conf.ie, nit) == -1)
    {
        log_error("inet_event_add error");
        return MRT_ERR;
    }

    nss->state = SESSION_WRITE;

    return 0;
}
