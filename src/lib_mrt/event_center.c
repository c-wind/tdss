#include <sys/time.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include "global.h"


//-------------------------
#define event_center_lock() pthread_mutex_lock(&(ec->mtx))
#define event_center_unlock() pthread_mutex_unlock(&(ec->mtx))




//从监听列表中删除一个任务的监听
static void event_delete(conn_t *t);

//清理超时的连接
static void event_clean();

//接入连接
static int connect_accept();

//关闭连接
static void connect_close(conn_t *t);

//-------------------------

static event_center_t *ec = NULL;
static event_center_t def_event_center;

static uint16_t id_inc = 0;

static conn_t *conn_create(int fd, addr_t addr, char *from)
{
    conn_t *c = M_alloc(sizeof(conn_t));
    c->id = (uint32_t)((time(NULL) << 16) + id_inc++);
    c->fd = fd;
    c->addr = addr;
    snprintf(c->addr_str, sizeof(c->addr_str), "%s", from);
    return c;
}

int event_insert(conn_t *conn)
{
    struct epoll_event epev = {EPOLLET, {0}};

    if(conn->wait & CONN_WAIT_RECV)
        epev.events |= EPOLLIN;

    if(conn->wait & CONN_WAIT_SEND)
        epev.events |= EPOLLOUT;

    if (epev.events == EPOLLET)
    {
        log_error("%x from:%s fd:%d insert type:(%d) error", conn->id, conn->addr_str, conn->fd, conn->wait);
        return MRT_SUC;
    }

    epev.data.ptr = conn;

    if(epoll_ctl(ec->epfd, EPOLL_CTL_ADD, conn->fd, &epev) == MRT_ERR)
    {
        log_error("epoll_ctl Add error:%m, addr:(%s), fd:%d, event:%d",
                  conn->addr_str, conn->fd, conn->wait);
        return MRT_ERR;
    }

    min_heap_elem_init(&conn->te, time(NULL) + ec->timeout, conn);
    min_heap_push(&ec->timer, &conn->te);

    log_debug("%x from:%s fd:%d set event:%d",
              conn->id, conn->addr_str, conn->fd, conn->wait);

    return MRT_OK;
}



static void event_delete(conn_t *conn)
{
    if(epoll_ctl(ec->epfd, EPOLL_CTL_DEL, conn->fd, NULL))
        log_error("%x from:%s fd:%d epoll_ctl error:%m", conn->id, conn->addr_str, conn->fd);

    min_heap_erase(&ec->timer, &conn->te);
}



//清理超时的连接
static void event_clean()
{
    timer_event_t *te;
    conn_t *conn = NULL;
    int num=0;
    time_t tm = time(NULL);
    time_t last = tm - ec->timeout;

    while((te = min_heap_top(&ec->timer)))
    {
        if(te->timeout > tm)
            break;

        conn = (conn_t *)te->data;
        if (conn->last > last)
        {
            min_heap_pop(&ec->timer);
            te->timeout = conn->last + ec->timeout;
            min_heap_push(&ec->timer, te);
            log_debug("repush %x timeout:%ld", conn->id, te->timeout);
            continue;
        }

        event_delete(conn);

        //设置事件为清理关闭
        conn->event = EVENT_TIMEOUT;

        //调用上层释放资源
        ec->on_close.func(conn);

        //连接在这里才真正的关闭
        connect_close(conn);

        num++;
    }

    //    log_debug("clean:%d online:%d", num, ec->online);
}


int connect_push(int fd, char *from, int bsize, int event, void *data)
{

    return 0;
}

//push进来的连接都是主动连接，连到远程服务器的
//不能在event_multi_loop里面用!!!!!!!!!!!
/*
   int connect_push(int fd, char *from, int bsize, int event, void *data)
   {
   M_cpvril(ec);
   M_cpvril(data);

   if(file_handle_init(&ec->task_array[fd].file,
   fd, FILE_HANDLE_OPEN, FD_TYPE_SOCKET|FD_TYPE_ACTIVE, from, bsize) == MRT_ERR)
   {
   log_error("remote:(%s) fd:%d init socket handle error", from, fd);
   close(fd);
   return MRT_ERR;
   }

   ec->task_array[fd].event = event;
//附加数据，可能是session之类
ec->task_array[fd].data = data;

event_center_lock();

if(event_insert(&ec->task_array[fd]))
{
log_error("event add error, will close ip:(%s) fd:%d.", from, fd);
file_close(&ec->task_array[fd].file);
return MRT_ERR;
}

event_center_lock();

return MRT_SUC;
}



int connect_create(char *addr, int port, task_t *old_task)
{
M_cpvril(ec);
M_cpsril(addr);

struct sockaddr_in iaddr;
int sock = -1;
task_t *tsk = NULL;
char from[MAX_IP] = {0};

if((sock = socket(AF_INET,SOCK_STREAM,0)) < 0)
{
log_error("socket error:%m remote:(%s:%d)", addr, port);
return MRT_ERR;
}

SOCKET_NONBLOCK(sock);

iaddr.sin_family      = AF_INET;
iaddr.sin_addr.s_addr = inet_addr(addr);
iaddr.sin_port        = htons((unsigned short) port);

if(connect(sock, (const struct sockaddr *)&iaddr, sizeof(iaddr)) == MRT_ERR)
{
if(errno != EINPROGRESS)
{
log_error("socket connect error:%m remote:(%s:%d)", addr, port);
return MRT_ERR;
}
}

snprintf(from, sizeof(from), "%s:%d", addr, port);

tsk = &ec->task_array[sock];
tsk->id = old_task->id;
tsk->child = old_task;
tsk->child->event = EVENT_PROC;


if(file_handle_init(&tsk->file, sock, FILE_HANDLE_OPEN, FD_TYPE_SOCKET | FD_TYPE_ACTIVE, from, M_4KB) == MRT_ERR)
{
    log_error("remote:(%s) fd:%d init socket handle error", from, sock);
    close(sock);
    return MRT_ERR;
}

if(ec->accept_before.state == 1)
{
    if(ec->accept_before.func(tsk) == MRT_ERR)
    {
        log_error("remote:(%s) fd:%d accept before error.", from, sock);
        connect_close(tsk);
        return MRT_ERR;
    }
}

event_center_lock();
if(event_insert(tsk))
{
    task_error("event add error");
    connect_close(tsk);
    event_center_unlock();
    return MRT_ERR;
}

ec->online++;
event_center_unlock();

return MRT_SUC;
}
*/



//功能：
//      接入新的连接，直到没有新的连接
//参数：
//     无
//返回：
//     0:成功, -1:出错
int connect_accept()
{
    struct sockaddr_in sa;
    socklen_t dummy;
    int nfd= -1;
    char from[MAX_IP] = {0};
    conn_t *conn = NULL;

    while(1)
    {
        dummy = sizeof(sa);
        nfd = accept(ec->lsfd, (struct sockaddr *)&sa, &dummy);
        if (nfd == MRT_ERR)
        {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN)
                break;
            log_error("accept error:%m");
            return MRT_ERR;
        }
        snprintf(from, sizeof(from), "%d.%d.%d.%d:%d",
                 sa.sin_addr.s_addr & 0xFF, (sa.sin_addr.s_addr >> 8) & 0xFF,
                 (sa.sin_addr.s_addr >> 16)& 0xFF, (sa.sin_addr.s_addr >> 24)& 0xFF,
                 ntohs(sa.sin_port));

        SOCKET_NONBLOCK(nfd);

        conn = conn_create(nfd, sa, from);
        if (!conn)
        {
            log_error("conn_create error, from:%s, fd:%d now close", from, nfd);
            close(nfd);
            continue;
        }

        if(ec->on_accept.func(conn) == MRT_ERR)
        {
            log_error("remote:%s fd:%d on_accept error.", from, nfd);
            close(nfd);
            continue;
        }
        conn->wait = CONN_WAIT_RECV|CONN_WAIT_SEND;

        if(event_insert(conn))
        {
            log_error("event add error");
            connect_close(conn);
            continue;
        }

        ec->online++;
    }

    return MRT_SUC;
}

void connect_close(conn_t *conn)
{
    min_heap_erase(&ec->timer, &conn->te);

    while(1)
    {
        if (!close(conn->fd))
            break;

        if (errno != EINTR)
            break;

        log_info("%x from:%s fd:%d close on EINTR", conn->id, conn->addr_str, conn->fd);
    }

    ec->online--;

    log_debug("%x close, from:%s fd:%d, now online:%d", conn->id, conn->addr_str, conn->fd, ec->online);
}


int connect_read(conn_t *conn)
{
    buffer_t *buf = NULL;
    char block[BUFSIZ] = {0};
    int rlen = 0, rsize = 0;

    while(1)
    {
        if((rlen = recv(conn->fd, block, sizeof(block), 0)) == MRT_ERR)
        {
            if(errno == EAGAIN)
            {
                conn->event &= ~EVENT_RECV;
                break;
            }

            if(errno == EINTR)
                continue;

            log_error("%x from:%s fd:%d, recv error:(%d:%m)", conn->id, conn->addr_str, conn->fd, errno);
            return MRT_ERR;
        }

        if(rlen == 0)
        {
            log_info("%x from:%s fd:%d close recv:%d", conn->id, conn->addr_str, conn->fd, conn->recv_size);
            conn->event = EVENT_OVER;
            if (rsize > 0)
                break;
            else
                return MRT_ERR;
        }
        rsize += rlen;

        if (buffer_create(&buf, rlen) == MRT_ERR)
        {
            log_error("%x from:%s fd:%d recv size:%d, buffer_create error", conn->id, conn->addr_str, conn->fd, rlen);
            return MRT_ERR;
        }

        buffer_push(buf, block, &rlen);
        LIST_INSERT_TAIL(conn, recv_bufs, buf, node);
        conn->recv_bufs.size += buf->len;
    }

    return MRT_SUC;
}



int connect_write(conn_t *conn)
{
    buffer_t *buf = NULL;
    int ssize = 0, asize = 0;

    while((buf = LIST_FIRST(conn, send_bufs)))
    {
DO_SEND_AGAIN:
        asize = buf->wpos - buf->rpos;
        if((ssize = write(conn->fd, buf->rpos, asize)) == MRT_ERR)
        {
            if(errno == EINTR)
                continue;
            if (errno == EAGAIN)
            {
                conn->event &= ~EVENT_SEND;
                break;
            }

            log_error("%x from:%s fd:%d send error:(%d:%m)", conn->id, conn->addr_str, conn->fd, errno);
            return MRT_ERR;
        }

        if(ssize == 0)
        {
            log_info("%x from:%s fd:%d send return 0", conn->id, conn->addr_str, conn->fd);
            return MRT_ERR;
        }

        buf->rpos += ssize;

        if (buf->wpos > buf->rpos)
            goto DO_SEND_AGAIN;

        log_debug("%x from:%s fd:%d send data size:%d", conn->id, conn->addr_str, conn->fd, ssize);

        LIST_REMOVE_HEAD(conn, send_bufs);

        buffer_deinit(buf);
    }

    return MRT_SUC;
}



//不管函数执行成功失败都会清空ec
int event_center_init(int max_conn, int timeout, char *host, int port,
                      callback_t on_accept,     //在接收完连接时调用
                      callback_t on_request,    //在接收到用户数据之后调用
                      callback_t on_response,   //在向用户发送完数据之后调用，不管当前发送缓冲区有没有需要发送的数据，都会调用
                      callback_t on_close      //在关闭连接之前调用
                     )
{
    struct epoll_event epev = {0, {0}};
    addr_t addr;

    M_cpiril(max_conn);
    M_cpsril(host);
    M_cpiril(port);

    s_zero(def_event_center);
    ec = &def_event_center;

    if((ec->epfd = epoll_create(max_conn)) == MRT_ERR)
    {
        log_error("epoll_create error:%m, max:%d.", max_conn);
        return MRT_ERR;
    }

    if((ec->lsfd = socket_bind(host, port)) == MRT_ERR)
    {
        log_error("bind services sock error:%m");
        return MRT_ERR;
    }

    ec->timeout = timeout;
    ec->max_conn = max_conn;

    ec->on_accept = on_accept;
    ec->on_request = on_request;
    ec->on_response = on_response;
    ec->on_close = on_close;

    s_zero(ec->timer);

    conn_t *conn = conn_create(ec->lsfd, addr, "");
    if (!conn)
    {
        log_error("conn_create error");
        return MRT_ERR;
    }
    sprintf(conn->addr_str, "%s:%d", host, port);

    epev.data.ptr = conn;
    epev.events = EPOLLIN|EPOLLET;
    if(epoll_ctl(ec->epfd, EPOLL_CTL_ADD, ec->lsfd, &epev) == MRT_ERR)
    {
        log_error("epoll_ctl add listen fd error:%m");
        return MRT_ERR;
    }

    log_info("init success bind:(%s) epfd:%d lsfd:%d.", conn->addr_str, ec->epfd, ec->lsfd);

    ec->state = 1;

    return MRT_SUC;
}

int inet_socket_proc(conn_t *conn)
{
    log_info("%x event:%d", conn->id, conn->event);
    if (conn->event & EVENT_ERROR)
    {
        log_info("%x from:%s fd:%d EPOLLERR|EPOLLHUP", conn->id, conn->addr_str, conn->fd);
        return MRT_ERR;
    }

    if(conn->event & EVENT_RECV)
    {
        log_info("%x from:%s fd:%d, on read", conn->id, conn->addr_str, conn->fd);
        if (connect_read(conn) == MRT_ERR)
        {
            log_info("%x from:%s fd:%d, read message error", conn->id, conn->addr_str, conn->fd);
            return MRT_ERR;
        }

        if (ec->on_request.func(conn) == MRT_ERR)
        {
            log_info("%x from:%s fd:%d, on_request error", conn->id, conn->addr_str, conn->fd);
            return MRT_ERR;
        }
    }

    if(conn->event & EVENT_SEND)
    {
        log_info("%x from:%s fd:%d, on write", conn->id, conn->addr_str, conn->fd);
        if(connect_write(conn) == MRT_ERR)
        {
            log_info("%x from:%s fd:%d, write message error", conn->id, conn->addr_str, conn->fd);
            return MRT_ERR;
        }
        if(ec->on_response.func(conn) == MRT_ERR)
        {
            log_info("%x from:%s fd:%d, on_response error", conn->id, conn->addr_str, conn->fd);
            return MRT_ERR;
        }
    }

    return MRT_OK;
}

#define MAX_WAIT_TIME 10

int inet_event_proc()
{
    struct epoll_event events[M_1KB];
    conn_t *conn = NULL;
    int res, i;

    if(!(res = epoll_wait(ec->epfd, events, M_1KB, MAX_WAIT_TIME)))
    {
        //        log_debug("wait timeout used %d msec", MAX_WAIT_TIME);
        return res;
    }

    if(res < 0)
    {
        if(errno == EINTR)
            return inet_event_proc();
        log_fatal("epoll_wait return:%d error:[%d|%m]", res, errno);
        return res;
    }

    log_info("event num:%d", res);

    for (i = 0; i < res; i++)
    {
        conn = events[i].data.ptr;
        if(conn->fd == ec->lsfd)
        {
            connect_accept();
            continue;
        }

        conn->last = time(NULL);

        if (events[i].events & (EPOLLERR|EPOLLHUP))
            conn->event |= EVENT_ERROR;
        if(events[i].events & EPOLLIN)
            conn->event |= EVENT_RECV;
        if(events[i].events & EPOLLOUT)
            conn->event |= EVENT_SEND;

        if (inet_socket_proc(conn) == MRT_ERR)
        {
            ec->on_close.func(conn);
            connect_close(conn);
        }

    }

    return MRT_OK;
}

void worker_return(task_t *tsk)
{
    pthread_mutex_lock(&ec->task_mtx);
    LIST_INSERT_TAIL(ec, task_head, tsk, node);
    pthread_mutex_unlock(&ec->task_mtx);
}

void thread_event_proc()
{
    pthread_mutex_lock(&ec->task_mtx);
    task_t *tsk;
    conn_t *conn;

    while((tsk = LIST_FIRST(ec, task_head)))
    {
        //如果只是普通定时任务就算了
        //如果是网络请求中的回调，有可能下一步需要处理网络收发
        if (tsk->on_return.func(tsk) == TASK_NEXT_INET)
        {
            conn = (conn_t *)tsk->data;
            if (conn)
            {
                if (inet_socket_proc(conn) == MRT_ERR)
                {
                    ec->on_close.func(conn);
                    connect_close(conn);
                }
            }
        }
        LIST_REMOVE_HEAD(ec, task_head);
    }

    pthread_mutex_unlock(&ec->task_mtx);
}


int event_loop()
{
    time_t tm = 0, cur = 0;
    M_cpvril(ec);

    while(ec->state != -1)
    {
        cur = time(NULL);
        if (tm != cur)
        {
            tm = cur;
            event_clean(ec);
        }

        //处理网络请求
        inet_event_proc();

        //处理子线程任务
        thread_event_proc();
    }

    log_info("loop over.");

    return MRT_SUC;
}





int event_center_deinit()
{
    M_cpvril(ec);
    timer_event_t *te;
    conn_t *conn;

    ec->state = -1;

    sleep(1);

    while((te = min_heap_top(&ec->timer)))
    {
        conn = (conn_t *)te->data;
        log_info("will close.");
        connect_close(conn);
        min_heap_erase(&ec->timer, te);
    }

    pthread_mutex_destroy(&ec->mtx);

    close(ec->lsfd);
    close(ec->epfd);

    log_info("event center close.");

    return MRT_SUC;
}


