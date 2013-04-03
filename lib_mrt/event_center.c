#include <sys/time.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include "global.h"


//-------------------------
#define event_center_lock() pthread_mutex_lock(&(ec->mtx))
#define event_center_unlock() pthread_mutex_unlock(&(ec->mtx))


//从监听列表中删除一个任务的监听
static int event_delete(task_t *tsk);

//清理超时的连接
static int event_clear();

//接入连接
static int connect_accept();

//关闭连接
static int connect_close(task_t *tsk);

//-------------------------

static event_center_t *ec = NULL;
static event_center_t def_event_center;

int conn_num=0;

int event_insert(task_t *tsk)
{
    M_cpvril(ec);
    M_cpvril(tsk);
    int event = EVENT_NONE;

    struct epoll_event epev = {0, {0}};

    M_cpvril(tsk);

    event = tsk->event;
    tsk->event = EVENT_NONE;

    switch(event)
    {
    case EVENT_RECV:
        tsk->stat = TASK_READ_WAIT;
        epev.events = EPOLLIN|EPOLLET;
        break;

    case EVENT_SEND:
        tsk->stat = TASK_WRITE_WAIT;
        epev.events = EPOLLOUT|EPOLLET;
        break;

    case EVENT_TIME:
        //添加一个超时任务，只管查时间
        task_debug("only push");
        minheap_push(&ec->heap, tsk);
        return MRT_SUC;

    case EVENT_OVER:
        //将结束这个任务
        task_debug("will close");
        connect_close(tsk);
        return MRT_SUC;

    default:
        task_info("insert type:%u No Change.", event);
        return MRT_SUC;
    }

    epev.data.fd = tsk->file.fd;

    tsk->last = time(NULL) + ec->timeout;

    if(epoll_ctl(ec->epfd, EPOLL_CTL_ADD, tsk->file.fd, &epev) == MRT_ERR)
    {
        log_error("epoll_ctl Add error:%m, addr:(%s), fd:%d, event:%s",
                  tsk->file.from, tsk->file.fd, tsk->event == EVENT_RECV ? "RECV" : "SEND");
        return MRT_ERR;
    }

    minheap_push(&ec->heap, tsk);

    task_debug("set event:%s", (tsk->event==EVENT_RECV) ? "RECV" : "SEND");

    return MRT_OK;
}


static int event_delete(task_t *tsk)
{
    M_cpvril(ec);
    M_cpvril(tsk);

    task_info("del from heap");
    if(epoll_ctl(ec->epfd, EPOLL_CTL_DEL, tsk->file.fd, NULL))
        task_error("epoll_ctl error:%m");

    minheap_delete(&ec->heap, tsk);

    return MRT_OK;
}



//清理超时的连接
static int event_clear(event_center_t *ec)
{
    M_cpvril(ec);

    task_t *tsk = NULL;
    int tm_wait = -1;
    int num=0;
    time_t tm;

    while((tsk = minheap_min(&ec->heap)))
    {
        log_info("heap num:%d, tsk idx:%d", ec->heap.num, tsk->idx);
        tm = time(NULL);

        if(tsk->last > tm)
        {
            tm_wait = tsk->last - tm;
            break;
        }
        if(epoll_ctl(ec->epfd, EPOLL_CTL_DEL, tsk->file.fd, NULL))
            task_error("epoll_ctl error:%m");

        minheap_delete(&ec->heap, tsk);
        ec->proc.func(tsk, EVENT_OVER);
        num++;
    }

    log_debug("clear:%d online:%d wait time:%d", num, ec->online, tm_wait);

    return (tm_wait < 0) ? -1 : tm_wait * 1000;
}



//push进来的连接都是主动连接，连到远程服务器的
//不能在event_multi_loop里面用!!!!!!!!!!!
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

    socket_nonblock(sock);

    iaddr.sin_family      = AF_INET;
    iaddr.sin_addr.s_addr = inet_addr(addr);
    iaddr.sin_port        = htons((unsigned short) port);

    if(connect(sock, (const struct sockaddr *)&iaddr, sizeof(iaddr)) == -1)
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



//功能：
//      接入新的连接，直到没有新的连接
//参数：
//     无
//返回：
//     0:成功, -1:出错
int connect_accept()
{
    M_cpvril(ec);

    struct sockaddr_in sa;
    socklen_t dummy = sizeof(sa);
    int nfd= -1;
    char from[MAX_IP] = {0};
    task_t *tsk = NULL;

    while((nfd = accept(ec->lsfd, (struct sockaddr *)&sa, &dummy)) > ec->lsfd)
    {
        if(socket_nonblock(nfd) == MRT_ERR)
        {
            log_error("socket set nonblocking error, error:%m");
            close(nfd);
            continue;
        }

        snprintf(from, sizeof(from), "%d.%d.%d.%d:%d",
                 sa.sin_addr.s_addr & 0xFF, (sa.sin_addr.s_addr >> 8) & 0xFF,
                 (sa.sin_addr.s_addr >> 16)& 0xFF, (sa.sin_addr.s_addr >> 24)& 0xFF,
                 ntohs(sa.sin_port));

        tsk = &ec->task_array[nfd];
        tsk->id = ec->id_inc++;

        if(ec->accept_before.state == 1)
        {
            if(ec->accept_before.func(tsk) == MRT_ERR)
            {
                log_error("remote:(%s) fd:%d accept before error.", from, nfd);
                close(nfd);
                continue;
            }
        }

        if(file_handle_init(&tsk->file, nfd, FILE_HANDLE_OPEN, FD_TYPE_SOCKET | FD_TYPE_PASSIVE, from, M_4KB) == MRT_ERR)
        {
            log_error("remote:(%s) fd:%d init socket handle error", from, nfd);
            close(nfd);
            continue;
        }

        if(ec->accept_after.state == 1)
        {
            if(ec->accept_after.func(tsk) == MRT_ERR)
            {
                log_error("remote:(%s) fd:%d accept after error.", from, nfd);
                close(nfd);
                continue;
            }
        }

        if(event_insert(tsk))
        {
            task_error("event add error");
            connect_close(tsk);
            continue;
        }

        ec->online++;
    }

    return MRT_SUC;
}

int connect_close(task_t *tsk)
{
    M_cpvril(ec);
    M_cpvril(tsk);

    if(ec->close_before.state == 1)
    {
        ec->close_before.func(tsk);
    }

    task_info("connect close");

    file_close(&tsk->file);

    ec->online--;

    if(ec->close_after.state == 1)
    {
        ec->close_after.func(tsk);
    }

    /*
    if(tsk->child)
        connect_close(tsk->child);
        */

    return MRT_SUC;
}


int connect_read(task_t *tsk)
{
    M_cpvril(tsk);

    file_handle_t *file = &tsk->file;
    buffer_t *buf = file->buffer;
    ssize_t rlen = 0;

    if(ec->read_before.state == 1)
    {
        if(ec->read_before.func(tsk) == MRT_ERR)
        {
            task_error("read_before error");
            return MRT_ERR;
        }
    }


    while(buf->len < buf->size)
    {
        if((rlen = recv(file->fd, buf->data + buf->len, buf->size - buf->len, 0)) == -1)
        {
            if(errno == EAGAIN)
                break;

            if(errno == EINTR)
                continue;

            task_error("recv error:(%d:%m)", errno);
            return MRT_ERR;
        }

        if(rlen == 0)
        {
            task_info("close recv:%d", buf->len);
            tsk->event = EVENT_OVER;
            break;
        }
        buf->len += rlen;
    }

    task_debug("recv:%d", buf->len);

    tsk->stat = TASK_READ_OVER;

    if(ec->read_after.state == 1)
    {
        if(ec->read_after.func(tsk) == MRT_ERR)
        {
            task_error("read_after error");
            return MRT_ERR;
        }
    }

    return MRT_SUC;
}



int connect_write(task_t *tsk)
{
    M_cpvril(tsk);

    file_handle_t *file = &tsk->file;
    buffer_t *buf = file->buffer;
    int32_t ssize = 0, asize = 0;

    if(ec->write_before.state == 1)
    {
        if(ec->write_before.func(tsk) == MRT_ERR)
        {
            task_error("write_before error");
            return MRT_ERR;
        }

    }

    while(buf->len != asize)
    {
        if((ssize = send(file->fd, buf->str + asize, buf->len - asize, 0)) == -1)
        {
            if((errno == EAGAIN) || (errno == EINTR))
                continue;

            task_error("send error:(%d:%m)", errno);
            return MRT_ERR;
        }

        if(ssize == 0)
        {
            task_info("close send:%d", asize);
            return MRT_ERR;
        }

        asize += ssize;
    }

    task_debug("send:%d", asize);

    buffer_clear(buf);

    tsk->stat = TASK_WRITE_OVER;

    if(ec->write_after.state == 1)
    {
        if(ec->write_after.func(tsk) == MRT_ERR)
        {
            task_error("write_after error");
            return MRT_ERR;
        }

    }

    return MRT_SUC;
}



//不管函数执行成功失败都会清空ec
int event_center_init(int max_conn, int timeout, char *host, int port,
                      callback_t accept_before, callback_t accept_after,
                      callback_t read_before,   callback_t read_after,
                      callback_t write_before,  callback_t write_after,
                      callback_t close_before,  callback_t close_after)
{
    struct epoll_event epev = {0, {0}};

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

    ec->task_array = M_alloc(sizeof(task_t) * max_conn);
    ec->task_array[ec->lsfd].file.fd = ec->lsfd;
    ec->timeout = timeout;

    ec->accept_before = accept_before;
    ec->accept_after = accept_after;

    ec->read_before = read_before;
    ec->read_after = read_after;

    ec->write_before = write_before;
    ec->write_after = write_after;

    ec->close_before = close_before;
    ec->close_after = close_after;

    minheap_init(&ec->heap, M_8KB);

    epev.data.fd = ec->lsfd;
    epev.events = EPOLLIN|EPOLLET;

    if(epoll_ctl(ec->epfd, EPOLL_CTL_ADD, ec->lsfd, &epev) == MRT_ERR)
    {
        log_error("epoll_ctl add listen fd error:%m");
        return MRT_ERR;
    }

    log_info("init success bind:(%s:%d) epfd:%d lsfd:%d.",
             host, port, ec->epfd, ec->lsfd);

    ec->state = 1;

    return MRT_SUC;
}


int event_single_loop()
{
    M_cpvril(ec);

    int wait = 0;
    int res = 0, i=0, nfd = 0;
    struct epoll_event events[M_1KB];
    task_t *tsk = NULL;

    while(ec->state != -1)
    {
        wait = event_clear(ec);

        if(!(res = epoll_wait(ec->epfd, events, M_1KB, wait)))
        {
            log_info("wait timeout used %d sec", wait/1000);
            continue;
        }

        if(res < 0)
        {
            if(errno == EINTR)
                continue;
            log_fatal("epoll_wait return:%d error:[%d|%m]", res, errno);
            continue;
        }

        for (i = 0; i < res; i++)
        {
            nfd = events[i].data.fd;

            if(nfd == ec->lsfd)
            {
                connect_accept(ec);
                continue;
            }

            //一个fd是一个任务
            tsk = &ec->task_array[nfd];

            //把任务的监听去掉
            if(event_delete(tsk))
            {
                task_error("event delete error");
                continue;
            }

            if(events[i].events & EPOLLIN)
            {
                if(connect_read(tsk) == MRT_ERR)
                {
                    task_error("read message error");
                    connect_close(tsk);
                    continue;
                }
            }
            else if(events[i].events & EPOLLOUT)
            {
                if(connect_write(tsk) == MRT_ERR)
                {
                    task_error("write message error");
                    connect_close(tsk);
                    continue;
                }
            }
            else
            {
                task_error("exception:%m");
                connect_close(tsk);
                continue;
            }

            //不管是接收还是发送完成之后都要调用一下事件处理函数
            /*
            if(tsk->proc.func(tsk) == MRT_ERR)
            {
                task_error("proc message error");
                connect_close(tsk);
                continue;
            }
            */

            if(tsk->child && tsk->child->event)
            {
                if(event_insert(tsk->child) == MRT_ERR)
                {
                    task_error("child event insert error");
                    connect_close(tsk->child);
                }
            }

            if(event_insert(tsk) == MRT_ERR)
            {
                task_error("event insert error");
                connect_close(tsk);
            }

        }
    }

    log_info("loop over.");

    return MRT_SUC;
}


int event_multi_loop()
{
    M_cpvril(ec);
    int wait = 0, res = 0;
    struct epoll_event epev;
    task_t *tsk = NULL;

    //第一步，找一个任务, 在加锁中进行, 同一时刻只有一个线程可以找任务
    event_center_lock();
    while(ec->worker_num != 0)
    {
        wait = event_clear(ec);

        if(!(res = epoll_wait(ec->epfd, &epev, 1, wait)))
        {
            log_info("wait timeout used %d sec", wait/1000);
            continue;
        }

        if(res < 0)
        {
            if(errno == EINTR)
                continue;
            log_fatal("epoll_wait return:%d error:[%d|%m]", res, errno);
            continue;
        }

        if(epev.data.fd == ec->lsfd)
        {
            connect_accept(ec);
            continue;
        }

        //一个fd是一个任务
        tsk = &ec->task_array[epev.data.fd];

        //把任务的监听去掉
        if(event_delete(tsk))
        {
            task_error("event delete error");
            continue;
        }

        if(epev.events & EPOLLIN)
        {
            if(connect_read(tsk) == MRT_ERR)
            {
                task_error("read message error");
                connect_close(tsk);
                continue;
            }
            break;
        }

        if(epev.events & EPOLLOUT)
        {
            if(connect_write(tsk) == MRT_ERR)
            {
                task_error("write message error");
                connect_close(tsk);
                continue;
            }
            break;
        }
        task_error("exception:%m");
        connect_close(tsk);
    }
    //这里找到一个任务后就释放这个锁了，让其它线程进来找任务
    event_center_unlock();

    //第二步，去处理这个任务, 这里不需要锁了
    //不管是接收还是发送完成之后都要调用一下事件处理函数
    /*
    if(tsk->proc.func(tsk) == MRT_ERR)
    {
        task_error("proc message error");
        connect_close(tsk);
        return MRT_ERR;
    }
    */

    //第三步，将处理完的任务重新添加事件监听, 此处应该有锁
    event_center_lock();
    if(event_insert(tsk) == MRT_ERR)
    {
        task_error("event insert error");
        connect_close(tsk);
        return MRT_ERR;
    }
    event_center_unlock();

    return MRT_SUC;
}



int event_center_deinit()
{
    M_cpvril(ec);

    task_t *tsk = NULL;

    ec->state = -1;

    sleep(1);

    while((tsk = minheap_min(&ec->heap)))
    {
        task_info("will close.");
        connect_close(tsk);
        minheap_delete(&ec->heap, tsk);
    }

    minheap_deinit(&ec->heap);

    pthread_mutex_destroy(&ec->mtx);

    close(ec->lsfd);
    close(ec->epfd);

    M_free(ec->task_array);

    log_info("event center close.");

    return MRT_SUC;
}


