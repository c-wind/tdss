#include <sys/time.h>
#include <sys/sendfile.h>
#include "global.h"
#include "tdss_config.h"
#include "master.h"


extern data_server_conf_t ds_conf;

extern command_t cmd_list[];



int32_t session_id_create()
{
    static int32_t sid = 0;
    return  sid++;
}


int request_parse(string_t *src, rq_arg_t *arg, int asize)
{
    int i = 0, c, state = 0, num = 0, idx = 0, k = 0;
    char key[128] = { 0 }, val[1024] = { 0};

    for(k=0; k<src->len; k++)
    {
        c = src->str[k];
        switch (c)
        {
        case '=':
            state = 1;
            idx = 0;
            break;
        case '\r':
        case '\n':
        case ' ':
            state = 2;
            idx = 0;
            break;
        default:
            if (state == 0)
            {
                if(idx < sizeof(key))
                    key[idx++] = c;
            }
            else
            {
                if(idx < sizeof(val))
                    val[idx++] = c;
            }
            break;
        }
        if (state == 2)
        {
            if (strlen(key) > 0 && strlen(val) > 0)
            {
                for(i=0; i<asize; i++)
                {
                    if (!strcasecmp(arg[i].key, key))
                    {
                        switch(arg[i].type)
                        {
                        case RQ_TYPE_U8:
                            *(uint8_t *)arg[i].val = (uint8_t)atoi(val);
                            break;
                        case RQ_TYPE_U16:
                            *(uint16_t *)arg[i].val = (uint16_t)atoi(val);
                            break;
                        case RQ_TYPE_U32:
                            *(uint32_t *)arg[i].val = (uint32_t)atoi(val);
                            break;
                        case RQ_TYPE_U64:
                            *(uint64_t *)arg[i].val = (uint64_t)atoll(val);
                            break;
                        case RQ_TYPE_STR:
                            snprintf((char *)arg[i].val, arg[i].vsize, "%s", val);
                            break;
                        }
                        num++;
                        break;
                    }
                }

                if (num == asize)
                {
                    log_debug("find argc=2");
                    return 0;
                }
            }
            memset(key, 0, sizeof(key));
            memset(val, 0, sizeof(val));
            state = 0;
        }
    }
    log_error("find argc:%d", num);
    return -1;
}




//功能：
//      统一的返回消息, 这东西自动调用,不要显示调用
static int __session_reply(conn_t *conn)
{
    int i=0, max = 0;
    session_t *ss = (session_t *)conn->dat;
    char *pb = ss->output.idx;

    max = ss->output.len - (ss->output.idx - ss->output.str);

    while(max > 0)
    {
        i = write(conn->fd, pb + i, max);
        if(i == -1)
        {
            if(errno == EAGAIN)
                break;
            if(errno == EINTR)
                continue;
        }

        if(i == 0)
        {
            log_info("write to %s error:%m", conn->addr_str);
            return -1;
        }

        pb += i;
        max -= i;

        if(max == 0)
            break;
    }

    if(max > 0)
    {
        ss->output.idx = pb;
        return NEXT_WRITE;
    }

    log_debug("%x reply:%s", ss->id, ss->output.str);

    ss->state = ss->next ?: SESSION_BEGIN;
//    log_debug("444444444 from:%s state:%d", conn->addr_str, ss->state);
    string_zero(&ss->input);
    string_zero(&ss->output);

    return ss->state;
}

//读取用户命令并调用相应函数
int request_command(conn_t *conn)
{
    int i = 0;
    char c = 0;
    session_t *ss = (session_t *)conn->dat;
    char line[1024] = {0};
    char cmd[64] = {0};

    if(!ss->input.len)
    {
        log_info("client:(%s) close", conn->addr_str);
        return SESSION_END;
    }

    log_info("recv:[%s]", ss->input.str);

    for(i=0; i<ss->input.len; i++)
    {
        c = ss->input.str[i];
        if(isspace(c))
            if(i > 0)
                break;
        cmd[i] = c;
    }

    if(i == ss->input.len || !*cmd)
    {
        log_error("command error, recv:%s.", ss->input.str);
        return 0;
    }

    for (i = 0; cmd_list[i].name; ++i)
        if (!strcasecmp(cmd_list[i].name, cmd))
            break;

    ss->id = session_id_create();
    ss->proc = cmd_list[i].func;

    return cmd_list[i].func(conn);
}


int on_accept(void *dat)
{
    conn_t *conn = (conn_t *)dat;
    session_t *ss = NULL;

    ss = M_alloc(sizeof(session_t));
    if(!ss)
    {
        log_error("M_alloc error:%m");
        return MRT_ERR;
    }

    conn->dat = ss;

    ss->state = SESSION_BEGIN;

    conn_debug("session stat:%d", ss->state);
    return MRT_SUC;
}

//用于主动连接，当对方返回数据时调用这个
int on_active_response()
{

}


int on_request(void *dat)
{
    conn_t *conn = (conn_t *)dat;
    int iret = 0;
    session_t *ss = (session_t *)conn->dat;
    buffer_t *buf;

    //如果是主动连接，此时为对方返回数据，调用on_active_response
    if (conn->type == CONN_ACTIVE)
    {
        return on_active_response();
    }

    LIST_FOREACH(buf, node, conn, recv_bufs)
    {
        if (string_catb(&ss->input, buf->rpos, buf->len) == MRT_ERR)
        {
            log_info("%x string_catb size:%d error", conn->id, buf->len);
            return MRT_ERR;
        }
    }

    request_command(conn);

    log_debug("fd:%d from:%s state:%d", conn->fd, conn->addr_str, ss->state);
    return 0;
}


//用于主动连接，向对方发数据时调用
int on_active_request()
{



}

int on_response(void *dat)
{
    conn_t *conn = (conn_t *)dat;

    //如果是主动连接，此时向对方发送数据，调用on_active_request
    if (conn->type == CONN_ACTIVE)
    {
        return on_active_request();
    }

    return MRT_OK;
}

int on_close(void *dat)
{
    conn_t *conn = (conn_t *)dat;
    session_t *ss = (session_t *)conn->dat;

    if(ss)
    {
        if(ss->node_server.finish)
        {
            ss->node_server.finish(conn);
        }

        string_free(&ss->input);
        string_free(&ss->output);
        string_free(&ss->node_server.cmd);
        log_info("task:%x session:(%x) over", conn->id, ss->id);
        M_free(ss);
    }

    return MRT_SUC;
}


