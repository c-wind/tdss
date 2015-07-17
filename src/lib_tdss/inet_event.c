#include "global.h"
#include "inet_event.h"
#include "time_queue.h"


void __inet_task_free(inet_event_t *ie, inet_task_t *it);

static int task_id_inc = 0x1000000;

#define TASK_ID_GEN (task_id_inc++)

void task_init(inet_task_t *it)
{
    if(!it)
        return;

    memset(it, 0, sizeof(*it));

    it->id = TASK_ID_GEN;
}

inet_task_t *task_create()
{
    inet_task_t *it = M_alloc(sizeof(inet_task_t));
    if(!it)
        return NULL;

    task_init(it);

    return it;
}


inet_event_t *inet_event_init(int max_event_size, int lsfd)
{
    struct epoll_event ev = {0};
    inet_task_t *it = malloc(sizeof(inet_task_t));
    inet_event_t *ie = malloc(sizeof(*ie));

    if(!it || !ie)
    {
        log_error("malloc error:%m");
        return NULL;
    }

    p_zero(ie);

    ie->epfd = epoll_create(max_event_size);
    if(ie->epfd == -1)
    {
        log_error("epoll_create error:%m, max event size:%d", max_event_size);
        free(it);
        free(ie);
        return NULL;
    }

    ie->evs = calloc(sizeof(struct epoll_event), max_event_size);
    if(!ie->evs)
    {
        log_error("calloc error:%m");
        return NULL;
    }

    ie->max_size = max_event_size;
    ie->lsfd = lsfd;

    if(time_queue_init(&ie->tq, 1024) == MRT_ERR)
    {
        log_error("time_queue_init error");
        return NULL;
    }

    //    TAILQ_INIT(&ie->head);

    if(lsfd != -1)
    {
        it->type = TASK_TYPE_LISTEN;
        it->fd = lsfd;

        //把监听的fd放到event事件中，这个fd不需要删除事件
        ev.events = EPOLLIN|EPOLLET;
        ev.data.ptr = it;

        if(epoll_ctl(ie->epfd, EPOLL_CTL_ADD, lsfd, &ev) == -1)
        {
            log_error("epoll_ctl error:%m, epfd:%d fd:%d", ie->epfd, it->fd);
            free(it);
            free(ie);
            return NULL;
        }
    }

    return ie;
}

//客户端连接过来的连接
void __inet_task_create(inet_event_t *ie, inet_task_t *it)
{
    inet_task_t *nit = NULL;
    struct sockaddr_in sa = {0};
    socklen_t dummy = sizeof(sa);
    int nfd = -1;

    while((nfd = accept(ie->lsfd, (struct sockaddr *)&sa, &dummy)) > ie->lsfd)
    {
        nit = calloc(sizeof(*nit), 1);
        if(!nit)
        {
            log_error("fd:%d calloc task error:%m", nfd);
            close(nfd);
            return;
        }

        if(socket_nonblock(nfd) == -1)
        {
            log_error("fd:%d set NONBLOCK error:%m", nfd);
            close(nfd);
            continue;
        }

        snprintf(nit->from, sizeof(nit->from),  "%d.%d.%d.%d:%d", sa.sin_addr.s_addr & 0xFF, (sa.sin_addr.s_addr >> 8) & 0xFF,
                 (sa.sin_addr.s_addr >> 16)& 0xFF, (sa.sin_addr.s_addr >> 24)& 0xFF,
                 ntohs(sa.sin_port));
        nit->fd = nfd;
        nit->type =  TASK_TYPE_REQUEST;
        nit->id = TASK_ID_GEN;

        if(ie->task_init(nit) == MRT_ERR)
        {
            log_error("init task:%x from:(%s) error", nit->id, nit->from);
            __inet_task_free(ie, nit);
            continue;
        }

        nit->state = TASK_WAIT_READ;

        if(inet_event_add(ie, nit) == -1)
        {
            log_error("fd:%d inet_event_add error.", nit->fd);
            __inet_task_free(ie, nit);
        }

        log_info("new task from:%s", nit->from);
    }
}

void __inet_task_free(inet_event_t *ie, inet_task_t *it)
{

    //   log_backtrace();

    log_debug("will deinit task:%x", it->id);
    if(ie->task_deinit(it) == MRT_ERR)
    {
        log_error("deinit task:%x from:(%s) error", it->id, it->from);
    }
    else
    {
        log_debug("deinit task:%x from:(%s) success", it->id, it->from);
    }

    close(it->fd);

    M_free(it);
}



int timer_event_add(inet_event_t *ie, inet_task_t *it)
{
    if(time_queue_push(&ie->tq, it) == MRT_ERR)
    {
        log_error("time_queue_push error");
        return MRT_ERR;
    }

    return MRT_OK;
}

void timer_event_del(inet_event_t *ie, inet_task_t *it)
{
    time_queue_delete(&ie->tq, it);

    it->func(it);

    log_debug("delete timer task %x", it->idx);
}


int inet_event_add(inet_event_t *ie, inet_task_t *it)
{
    struct epoll_event ev = {0};

    switch(it->state)
    {
    case TASK_WAIT_READ:
        ev.events = EPOLLIN|EPOLLET;
        break;
    case TASK_WAIT_WRITE:
        ev.events = EPOLLOUT|EPOLLET;
        break;
    case TASK_WAIT_NOOP:
        log_debug("task:%x from:(%s) wait child", it->id, it->from);
        return MRT_OK;
    case TASK_WAIT_END:
        log_info("task:%x from:(%s) fd:%d close.", it->id, it->from, it->fd);
        __inet_task_free(ie, it);
        return MRT_OK;
    }

    ev.data.ptr = it;
    it->timeout = time(NULL) + ie->timeout;

    log_debug("task:%x fd:%d from:%s wait time:%u", it->id, it->fd, it->from, it->timeout);

    if(time_queue_push(&ie->tq, it) == MRT_ERR)
    {
        log_error("time_queue_push error");
        return MRT_ERR;
    }

    if(epoll_ctl(ie->epfd, EPOLL_CTL_ADD, it->fd, &ev) == -1)
    {
        log_error("epoll_ctl error:%m, epfd:%d fd:%d", ie->epfd, it->fd);
        time_queue_delete(&ie->tq, it);
        return MRT_ERR;
    }


    //    TAILQ_INSERT_TAIL(&ie->head, it, point);

    log_debug("task:%x add fd:%d, from:%s wait %s.",
              it->id, it->fd, it->from, it->state == TASK_WAIT_READ ? "read" : "write");

    return 0;
}




void inet_event_del(inet_event_t *ie, inet_task_t *it)
{
    time_queue_delete(&ie->tq, it);

    if(it->state == TASK_WAIT_TIMER)
    {
        log_debug("delete timer task:%x", it->id);
        return ;
    }

    if(epoll_ctl(ie->epfd, EPOLL_CTL_DEL, it->fd, NULL) == -1)
    {
        log_error("epoll_ctl delete task:%x fd:%d error:%m", it->id, it->fd);
    }

    //    TAILQ_REMOVE(&ie->head, it, point);
    log_info("epoll_ctl del task:%x fd:%d", it->id, it->fd);
}


//连接到远程服务器的任务
inet_task_t *inet_connect_init(char *ip, int port)
{
    inet_task_t *nit = NULL;
    int nfd = -1;

    nfd = socket_connect_nonblock(ip, port);
    if(nfd == -1)
    {
        log_error("connect error:%m");
        return NULL;
    }

    nit = calloc(sizeof(*nit), 1);
    if(!nit)
    {
        log_error("fd:%d calloc task error:%m", nfd);
        close(nfd);
        return NULL;
    }
    nit->fd = nfd;
    snprintf(nit->from, sizeof(nit->from),  "%s:%d", ip, port);
    nit->state = TASK_WAIT_WRITE;
    nit->id = TASK_ID_GEN;

    log_debug("new task:%x", nit->id);

    return nit;
}


int inet_event_clear(inet_event_t *ie)
{
    time_t tm = time(NULL);
    inet_task_t *it = NULL;

    while((it = minheap_min(&ie->tq)))
    {
        if(tm < it->timeout)
        {
            log_debug("next wait:%lu sec.", (int)it->timeout - tm);
            return (it->timeout - tm) * 1000;
        }
        log_debug("curr time:%u, it timeout:%u", (uint32_t)tm, (uint32_t)it->timeout);
        if(it->state == TASK_WAIT_TIMER)
        {
            timer_event_del(ie, it);
            continue;
        }

        inet_event_del(ie, it);
        log_info("client:(%s) timeout.", it->from);
        __inet_task_free(ie, it);
    }

    log_debug("no found inet task.");
    return -1;
}



int inet_event_loop(inet_event_t *ie)
{
    int i, cnum, timeout = 0;
    inet_task_t *it = NULL;

    while(1)
    {
        timeout = inet_event_clear(ie);
        log_debug("will wait %d sec", timeout);
        cnum = epoll_wait(ie->epfd, ie->evs, ie->max_size, timeout);
        if(cnum == -1)
        {
            if(errno == EINTR)
                continue;
            log_error("epoll_wait error:%m");
            return -1;
        }

        if(cnum == 0)
        {
            log_debug("epoll_wait timeout:%m");
            continue;
        }

        log_debug("find event %d", cnum);

        for(i = 0; i< cnum; i++)
        {
            it = ie->evs[i].data.ptr;
            //如果任务类型是监听任务直接去接连接
            if(it->type == TASK_TYPE_LISTEN)
            {
                log_debug("will create new task");
                __inet_task_create(ie, it);
            }
            else
            {   //其它任务，调用task_init里绑定的回调函数执行(如返回错误断开连接)
                inet_event_del(ie, it);
                log_debug("will proc task");
                if(it->func(it) == MRT_ERR)
                {
                    log_error("task:%x from:(%s) proc error", it->id, it->from);
                    __inet_task_free(ie, it);
                    continue;
                }
                log_debug("proc task success");
                if(inet_event_add(ie, it) == MRT_ERR)
                {
                    log_error("inet_event_add task:%x from:(%s) error", it->id, it->from);
                    __inet_task_free(ie, it);
                }
                log_debug("readd task event");
            }
            log_debug("proc event %d, ok", i);
        }
    }

    return -1;
}








#ifdef TEST_WATCH

int server_conf_init()
{
    sprintf(server.home, "./test_dir/");
    sprintf(server.todo_path, "todo");
    sprintf(server.message_path, "message");
    sprintf(server.bounce_path, "bounce");
    sprintf(server.local_path, "local");
    sprintf(server.remote_path, "remote");

    return 0;
}

int  main(int argc, char *argv[])
{
    int ifd = 0, wfd = 0;


    memset(&server, 0, sizeof(server));

    openlog("server_queue", LOG_PID, LOG_MAIL);

    server_conf_init();

    if(chdir(server.home) == -1)
    {
        printf("can't chdir to:%s error:%m\n", server.home);
        return -1;
    }

    if(watchdir_init(server.todo_path, &ifd, &wfd) == -1)
    {
        printf("watchdir_init error, path:%s\n", server.todo_path);
        return -1;
    }

    if(watchdir_loop(ifd) == -1)
    {
        printf("watchdir_loop error\n");
    }

    watchdir_deinit(ifd, wfd);

    return 0;
}

#endif

