#include <openssl/conf.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "master.h"


extern data_server_conf_t ds_conf;
extern name_server_conf_t ns_conf;
extern master_server_conf_t ms_conf;




int __server_check_loop(inet_task_t *it);

void session_free(session_t *ss)
{
    string_free(&ss->node_server.cmd);

    string_free(&ss->input);

    string_free(&ss->output);

    memset(ss, 0, sizeof(*ss));

    M_free(ss);
}

/* ds_info_t存放每个data_server当前状态信息
typedef struct
{
    int                 state;
    ip4_addr_t          addr;

    uint16_t            server_id;

    uint32_t            disk_size;      //单位为MB
    uint32_t            disk_free;      //单位为MB

    uint32_t            inode_size;     //单位为KB
    uint32_t            inode_free;     //单位为KB

}data_server_info_t;
*/

int __data_server_report(session_t *ss)
{
    data_server_info_t *dsi = &ms_conf.data_server.info[ms_conf.data_server.id];

    rq_arg_t rq[] = {
        {RQ_TYPE_INT,   "disk_size",    &dsi->disk_size,    0},
        {RQ_TYPE_INT,   "disk_free",    &dsi->disk_free,    0},
        {RQ_TYPE_INT,   "inode_size",   &dsi->inode_size,   0},
        {RQ_TYPE_INT,   "inode_free",   &dsi->inode_free,   0}
    };

    if(request_parse(&ss->input, rq, 4) == MRT_ERR)
    {
        log_error("report error, recv:%s", ss->input.str);
        dsi->state = SERVER_OFFLINE;
        return MRT_ERR;
    }

    dsi->state = SERVER_ONLINE;

    log_info("server:(%s:%d) disk size:%uMB free:%uMB inode size:%u free:%u",
             dsi->addr.ip, dsi->addr.port,
             dsi->disk_size, dsi->disk_free, dsi->inode_size, dsi->inode_free);

    return MRT_OK;
}


/*name_server_info_t存放每个name_server(主服务)当前状态信息
typedef struct
{
    int                 state;
    ip4_addr_t          addr;

    uint16_t            server_id;
    uint32_t            size;           //存储了多少条记录
    uint32_t            used;           //存储了多少条记录

}name_server_info_t;

*/


int __name_server_report(session_t *ss)
{
    name_server_info_t *nsi = &ms_conf.name_server.info[ms_conf.name_server.id];

    rq_arg_t rq[] = {
        {RQ_TYPE_INT,   "size",    &nsi->size,    0},
        {RQ_TYPE_INT,   "used",    &nsi->used,    0}
    };

    if(request_parse(&ss->input, rq, 2) == MRT_ERR)
    {
        log_error("report error, recv:%s", ss->input.str);
        nsi->state = SERVER_OFFLINE;
        return MRT_ERR;
    }

    nsi->state = SERVER_ONLINE;

    log_info("server:(%s:%d) name size:%u used:%u",
             nsi->addr.ip, nsi->addr.port, nsi->size, nsi->used);

    return MRT_OK;
}


int __server_report(session_t *ss)
{
    if(ss->server_type == SERVER_TYPE_DATA)
        return __data_server_report(ss);

    return __name_server_report(ss);
}



int __server_recv(inet_task_t *it)
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
//            ss->node_server.finish(it);
            return -1;
        }

        string_catb(&ss->input, line, i);
    }

    ss->state = SESSION_PROC;

//    ss->node_server.finish(it);

    return -1;
}


int __server_send(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    char *pstr = NULL;
    int plen = 0, ssize = 0;
    //inet_task_t *pit = ss->parent;

//    log_info("### parent task:%x cur task:%x", pit->id, it->id);

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
            log_error("task:%x from:(%s) write error:%m", it->id, it->from);
           // ss->node_server.finish(it);
            return -1;
        }
        pstr += ssize;
        plen -= ssize;
    }

    //如果ssize为0，说明远程端口关闭了
    if(ssize == 0)
    {
        log_info("remote:%s close.", it->from);
//        ss->node_server.finish(it);
        return -1;
    }

    //如果plen为0说明发完了
    if(plen != 0)
    {
        ss->output.idx = pstr;
        it->state = TASK_WAIT_WRITE;
        return MRT_SUC;
    }

    it->func = __server_recv;
    it->state = TASK_WAIT_READ;

    ss->state = SESSION_READ;

    return 0;
}





int __server_finish(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;
    inet_task_t *pit = ss->parent;

    switch(ss->state)
    {
    case SESSION_WRITE:
        log_error("task:%x session:%x server:(%s) write erorr", it->id, ss->id, it->from);
        goto child_fail_return;
    case SESSION_READ:
        log_error("task:%x session:%x server:(%s) read erorr", it->id, ss->id, it->from);
        goto child_fail_return;
    }

    if(ss->input.len < 4)
    {
        log_error("%x server:(%s) return error.", ss->id, it->from);
        goto child_fail_return;
    }

    if(strncmp(ss->input.str, "1000", 4))
    {
        log_error("%x server:(%s) recv:%s", ss->id, it->from, ss->input.str);
        goto child_fail_return;
    }

    log_info("task:%x session:%x server:%s recv success", it->id, ss->id, it->from);

    __server_report(ss);

    __server_check_loop(pit);

    return MRT_SUC;

child_fail_return:

    if(ss->server_type == SERVER_TYPE_DATA)
        ms_conf.data_server.info[ms_conf.data_server.id].state = SERVER_OFFLINE;
    else
        ms_conf.name_server.info[ms_conf.data_server.id].state = SERVER_OFFLINE;

    it->state = TASK_WAIT_END;

    log_info("task:%x session:%x server:%s fail", it->id, ss->id, it->from);

    __server_check_loop(pit);

    return MRT_SUC;
}



//返回：
//      -1：添加超时任务
//      0：添加连接任务
int __data_server_next(ip4_addr_t *addr)
{
    int next_id = (ms_conf.data_server.id + 1) % ms_conf.data_server.count;
    log_info("======= next_id:%d, start:%u time:%u", next_id, ms_conf.data_server.start,time(NULL));
    if(next_id == 0)
    {
        if(ms_conf.data_server.start == time(NULL))
            return MRT_ERR;

        ms_conf.data_server.start = time(NULL);
    }
    ms_conf.data_server.id = next_id;
    memcpy(addr, &ms_conf.data_server.info[next_id].addr, sizeof(*addr));

    return MRT_OK;
}


int __name_server_next(ip4_addr_t *addr)
{
    int next_id = (ms_conf.name_server.id + 1) % ms_conf.name_server.count;
    if(next_id == 0)
    {
        if(ms_conf.name_server.start == time(NULL))
            return MRT_ERR;
        ms_conf.name_server.start = time(NULL);

    }

    ms_conf.name_server.id = next_id;

    memcpy(addr, &ms_conf.name_server.info[ms_conf.name_server.id].addr, sizeof(*addr));

    return MRT_OK;
}


//功能：
//      根据type返回下一个要检测的服务器地址
//返回：
//      -1:需要休息一秒
//      0:继续检测
//      如果本次循环检测在一秒之内完成就休息一秒再继续检测
int __server_next(int type, ip4_addr_t *addr)
{
    if(type == SERVER_TYPE_DATA)
    {
        return __data_server_next(addr);
    }

    return __name_server_next(addr);
}


int __server_check_loop(inet_task_t *it)
{
    inet_task_t *nit = NULL;
    session_t *nss = NULL, *ss = (session_t *)it->data;
    ip4_addr_t sip;

    s_zero(sip);

    if(__server_next(ss->server_type, &sip) == MRT_ERR)
    {
        log_info("loop check next wait");
        goto loop_next_return;
    }

    nit = inet_connect_init(sip.ip, sip.port);
    if(nit == NULL)
    {
        log_error("connect to:%s:%d error:%m", sip.ip, sip.port);
        return MRT_ERR;
    }

    nit->func = __server_send;
    nit->state = TASK_WAIT_WRITE;
    nss = M_alloc(sizeof(session_t));
    if(!nss)
    {
        log_error("M_alloc session error:%m");
        return MRT_ERR;
    }

    nss->node_server.state = 0;
    nss->node_server.finish = __server_finish;
    nss->parent = it;
    nss->state = SESSION_BEGIN;
    nss->server_type = ss->server_type;
    string_copys(&nss->output, "status\r\n");
    nit->data = nss;

    if(inet_event_add(ms_conf.ie, nit) == -1)
    {
        log_error("inet_event_add error");
        return MRT_ERR;
    }

    nss->state = SESSION_WRITE;

    return MRT_OK;

loop_next_return:
    it->timeout = time(NULL) + ms_conf.interval;

    it->state = TASK_WAIT_TIMER;

    it->func = __server_check_loop;

    //log_backtrace();

    if(timer_event_add(ms_conf.ie, it) == MRT_ERR)
    {
        log_error("timer_event_add server check error");
        return MRT_ERR;
    }

    return MRT_OK;
}



int server_check_init()
{
    int i=0;
    session_t *ss_data = NULL, *ss_name = NULL;

   // memset(&ms, 0, sizeof(ms));

    if(!(ms_conf.data_server.info = M_alloc(ds_conf.server_count * sizeof(data_server_info_t))))
    {
        log_error("M_alloc data_server_info error");
        goto error_return;
    }
    ms_conf.data_server.count = ds_conf.server_count;

    if(!(ms_conf.name_server.info = M_alloc(ns_conf.server_count * sizeof(name_server_info_t))))
    {
        log_error("M_alloc data_server_info error");
        goto error_return;
    }
    ms_conf.name_server.count = ns_conf.server_count;

    for(i=0; i<ms_conf.data_server.count; i++)
    {
        memcpy(&ms_conf.data_server.info[i].addr, &ds_conf.server[i].server, sizeof(ds_conf.server[i].server));
    }

    for(i=0; i<ms_conf.name_server.count; i++)
    {
        memcpy(&ms_conf.name_server.info[i].addr, &ns_conf.server[i].master, sizeof(ns_conf.server[i].master));
    }

    ss_data = M_alloc(sizeof(session_t));
    if(ss_data == NULL)
    {
        log_error("M_alloc session error");
        goto error_return;
    }
    p_zero(ss_data);
    ss_data->server_type = SERVER_TYPE_DATA;

    ss_name = M_alloc(sizeof(session_t));
    if(ss_name == NULL)
    {
        log_error("M_alloc session error");
        goto error_return;
    }
    p_zero(ss_name);
    ss_name->server_type = SERVER_TYPE_NAME;

    task_init(&ms_conf.data_server.task);
    ms_conf.data_server.task.data = ss_data;

    task_init(&ms_conf.name_server.task);
    ms_conf.name_server.task.data = ss_name;

    if(__server_check_loop(&ms_conf.data_server.task) == MRT_ERR)
    {
        log_error("__server_check_loop data error");
        return MRT_ERR;
    }

    log_info("start data server loop");

    if(__server_check_loop(&ms_conf.name_server.task) == MRT_ERR)
    {
        log_error("__server_check_loop name error");
        return MRT_ERR;
    }

    log_info("start name server loop");

    return MRT_OK;

error_return:
    if(ms_conf.data_server.info)
        M_free(ms_conf.data_server.info);

    if(ms_conf.name_server.info)
        M_free(ms_conf.name_server.info);

    if(ss_data)
        M_free(ss_data);

    if(ss_name)
        M_free(ss_name);

    return MRT_ERR;
}




