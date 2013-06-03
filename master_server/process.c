#include <sys/time.h>
#include <sys/sendfile.h>
#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "master.h"


extern data_server_conf_t ds_conf;

extern command_t cmd_list[];

int mail_save(inet_task_t *it);


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
static int __session_reply(inet_task_t *it)
{
    int i=0, max = 0;
    session_t *ss = (session_t *)it->data;
    char *pb = ss->output.idx;

    max = ss->output.len - (ss->output.idx - ss->output.str);

    while(max > 0)
    {
        i = write(it->fd, pb + i, max);
        if(i == -1)
        {
            if(errno == EAGAIN)
                break;
            if(errno == EINTR)
                continue;
        }

        if(i == 0)
        {
            log_info("write to %s error:%m", it->from);
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
//    log_debug("444444444 from:%s state:%d", it->from, ss->state);
    string_zero(&ss->input);
    string_zero(&ss->output);

    return ss->state;
}

//读取用户命令并调用相应函数
int request_command(inet_task_t *it)
{
    int i = 0;
    char c = 0;
    session_t *ss = (session_t *)it->data;
    char line[1024] = {0};
    char cmd[64] = {0};

    string_zero(&ss->input);
    ss->next = SESSION_BEGIN;
    ss->state = SESSION_BEGIN;

    while((i = read(it->fd, line, sizeof(line))))
    {
        if(i == -1)
        {
            if(errno == EAGAIN)
                break;
            if(errno == EINTR)
                continue;
            log_error("read from:%s fd:%d error:%m", it->from, it->fd);
            return SESSION_END;
        }

        string_catb(&ss->input, line, i);
    }

    if(!ss->input.len)
    {
        log_info("client:(%s) close", it->from);
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

    return cmd_list[i].func(it);
}


int request_init(inet_task_t *it)
{
    session_t *ss = NULL;

    ss = M_alloc(sizeof(session_t));
    if(!ss)
    {
        log_error("M_alloc error:%m");
        return MRT_ERR;
    }
    it->data = ss;
    it->func = request_process;

    string_zero(&ss->input);
    string_zero(&ss->output);
    ss->next = SESSION_BEGIN;
    ss->state = SESSION_BEGIN;

    log_debug("11111111 from:%s state:%d", it->from, ss->state);
    return MRT_SUC;
}




int request_process(inet_task_t *it)
{
    int iret = 0;
    session_t *ss = (session_t *)it->data;


//    log_debug("222222 from:%s state:%d", it->from, ss->state);
    //SESSION_LOOP继续上次命令处理
    //SESSION_REPLY要返回消息
    switch(ss->state)
    {
    case SESSION_READ:
        log_debug("session state is read.");
        iret = ss->proc(it);
        break;
    case SESSION_WRITE:
        log_debug("session state is write.");
        iret = ss->proc(it);
        break;
    case SESSION_REPLY:
        log_debug("session state is reply.");
        iret = __session_reply(it);
        break;
    default:
        log_debug("session no state.");
        iret = request_command(it);
        break;
    }

    //如果返回值是-1要断开连接
    switch(iret)
    {
    case SESSION_END:
        it->state = TASK_WAIT_END;
        break;
    case SESSION_BEGIN:
        it->state = TASK_WAIT_READ;
        break;
    case SESSION_READ:
        it->state = TASK_WAIT_READ;
        break;
    case SESSION_WAIT:
        it->state = TASK_WAIT_NOOP;
        break;
    case SESSION_REPLY:
    case SESSION_WRITE:
        it->state = TASK_WAIT_WRITE;
        break;
    }

    log_debug("fd:%d from:%s state:%d", it->fd, it->from, ss->state);
    return 0;
}

int request_deinit(inet_task_t *it)
{
    session_t *ss = (session_t *)it->data;

    if(ss)
    {
        if(ss->node_server.finish)
        {
            ss->node_server.finish(it);
        }

        string_free(&ss->input);
        string_free(&ss->output);
        string_free(&ss->node_server.cmd);
        log_info("task:%x session:(%x) over", it->id, ss->id);
        M_free(ss);
    }

    return MRT_SUC;
}


