#ifndef __EVENT_WATCH_H__
#define __EVENT_WATCH_H__

#include <sys/stat.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <sys/queue.h>

#define epev struct epoll_event

#define TASK_TYPE_CONNNECT  1
#define TASK_TYPE_REQUEST   2
#define TASK_TYPE_LISTEN    3

#define TASK_WAIT_READ      1
#define TASK_WAIT_WRITE     2
#define TASK_WAIT_NOOP      3
#define TASK_WAIT_TIMER     4
#define TASK_WAIT_END       5

typedef struct inet_task_s inet_task_t;
struct inet_task_s
{
    int             id;         //唯一序列号
    int             idx;        //当任务进入队列后使用这个
    int             state;      //状态：TASK_WAIT_READ准备读，TASK_WAIT_WRITE准备写， TASK_WAIT_END准备断开
    int             type;       //类型：TASK_TYPE_CONNNECT连接到远程服务器的长连接，TASK_TYPE_REQUEST客户端请求 , TASK_TYPE_LISTEN当前监听的fd
    char            from[128];
    int             timeout;
    int             fd;

    void            *data;      //附加字段，可设置session之类

    int             (*func)(inet_task_t *);


//    TAILQ_ENTRY(inet_task_s) point;
};

typedef struct
{
    inet_task_t     **task;
    int             num;
    int             size;

}time_queue_t;


typedef struct
{
    int             epfd;
    int             lsfd;
    int             max_size;
    epev            *evs;
    int             timeout;    //所有连接强制使用相同超时时间

    int             (*task_init)(inet_task_t *);         //连接进入时的回调函数
//    int             (*proc)(inet_task_t *);         //这个是
    int             (*task_deinit)(inet_task_t *);       //连接断开时的回调函数

    time_queue_t    tq;
    //TAILQ_HEAD(, inet_task_s) head;

}inet_event_t;

//清空task内容 ,生成task的ID
void task_init(inet_task_t *it);

//生成task并生成ID
inet_task_t *task_create();

inet_event_t *inet_event_init(int max_event_size, int lsfd);

int inet_event_add(inet_event_t *ie, inet_task_t *it);

int timer_event_add(inet_event_t *ie, inet_task_t *it);

int inet_event_loop(inet_event_t *ie);

inet_task_t *inet_connect_init(char *ip, int port);

#endif
