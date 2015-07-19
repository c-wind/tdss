#ifndef __FACTORY_H__
#define __FACTORY_H__

#include <pthread.h>
#include <stdint.h>

#include "minheap.h"

#define FACTORY_READY 0
#define FACTORY_START 1
#define FACTORY_OVER 2

#define WORKER_READY 0
#define WORKER_START 1
#define WORKER_OVER 2

//event type -------------------------

#define TASK_CONNECT 0
#define TASK_READ_WAIT 1
#define TASK_READ_OVER 2
#define TASK_WRITE_WAIT 4
#define TASK_WRITE_OVER 8

#define TASK_NEXT_NONE 0
#define TASK_NEXT_INET 1

// -----------------------------------


#define worker() (((worker_t *)pthread_getspecific(factory.key)))
#define worker_id() (((worker_t *)pthread_getspecific(factory.key))->idx)
#define worker_type() (((worker_t *)pthread_getspecific(factory.key))->type)

//线程内部错误传递
#define worker_set_error(fmt, ...) \
    snprintf((((worker_t *)pthread_getspecific(factory.key))->error_msg),  \
             sizeof((((worker_t *)pthread_getspecific(factory.key))->error_msg)), \
             fmt, ##__VA_ARGS__)

#define worker_get_error() (((worker_t *)pthread_getspecific(factory.key))->error_msg)
// -----------------------------------
//
#define callback_set(cb, f) \
    do { \
        cb.func = f; \
        snprintf(cb.name, sizeof(cb.name), "%s", #f); \
    }while(0)

#define conn_fatal(conn, fmt, ...) logger_write(MRT_FATAL, "FATAL", "%s %x %s fd:%d "fmt, __func__, conn->id, conn->addr_str, conn->fd, ##__VA_ARGS__)
#define conn_error(conn, fmt, ...) logger_write(MRT_ERROR, "ERROR", "%s %x %s fd:%d "fmt, __func__, conn->id, conn->addr_str, conn->fd, ##__VA_ARGS__)
#define conn_warning(conn, fmt, ...) logger_write(MRT_WARNING, "WARNING", "%s %x %s fd:%d "fmt, __func__, conn->id, conn->addr_str, conn->fd, ##__VA_ARGS__)
#define conn_info(conn, fmt, ...) logger_write(MRT_INFO, "INFO", "%s %x %s fd:%d "fmt, __func__, conn->id, conn->addr_str, conn->fd, ##__VA_ARGS__)
#define conn_debug(conn, fmt, ...) logger_write(MRT_DEBUG, "DEBUG", "%s %x %s fd:%d "fmt, __func__, conn->id, conn->addr_str, conn->fd, ##__VA_ARGS__)

typedef struct
{
    int               (*func)(void *);        //回调函数指针
    char              name[MAX_ID];           //函数名
}callback_t;

typedef struct sockaddr_in addr_t;

typedef struct {
    int               id;
    int               fd;
    int               stat;
    int               wait;       //需要监听的事件
    uint32_t          event;      //已有事件
    int               last;       //最后一次操作时间
    addr_t            addr;

    timer_event_t     te;

    void            *dat;

    char              addr_str[MAX_IP];
    list_head_t       recv_bufs;
    int               recv_size;
    list_head_t       send_bufs;
}conn_t;

typedef struct
{
    uint32_t          id;
    int               stat;

    void              *data;

    callback_t        thread_main;    //在多线程中执行的

    callback_t        on_return;      //在主线程中回调的, 这个回调要返回是否需要继续调用网络事件处理

    list_node_t       node;
}task_t;



typedef struct
{
    int8_t            state;
    time_t            start;

    pthread_t         idx;
    pthread_mutex_t   mtx;
    pthread_cond_t    cnd;

    char              error_msg[MAX_LINE];          //记录错误信息

    list_node_t       node;
}worker_t;






typedef struct
{
    int               state;          //1:启动, -1停止
    int               worker_num;     //应该为1
    int               worker_max;     //最大线程数也必须为1

    int               epfd;
    int               lsfd;
    int               online;         //当前在线连接数

    int               timeout;
    int               task_num;
    int               task_max;

    pthread_mutex_t   mtx;

    callback_t        on_accept;
    callback_t        on_request;
    callback_t        on_response;
    callback_t        on_close;

    int               max_conn;

    min_heap_t        timer;          //事件堆

    pthread_mutex_t   task_mtx;       //子线程回调锁
    list_head_t       task_head;      //所有子线程处理过的任务放到这里,等待回调

}event_center_t;



typedef struct
{
    int                 worker_num;
    int                 worker_max;
    int                 worker_min;

    int                 conn_max;
    int                 conn_timeout;

    int                 local_bind;             //0:不绑定本地端口,1:绑定
    char                local_host[MAX_IP];
    int                 local_port;

    int                 daemon;                 //0:不启用，1为启用daemon
    char                daemon_home[MAX_PATH];  //运行路径，如果为daemon的话会chroot到这个目录中，
    //日志在/log,运行在/run

    int                 logger;                 //0:不启用，1：启用. 不启用时输出为标准输出
    char                logger_name[MAX_ID];
    int                 logger_level;           //log级别

}server_config_t;


typedef struct
{
    int                 state;

    int                 worker_num;
    int                 worker_max;
    int                 worker_min;
    list_head_t         worker_head;

    int                 task_num;
    list_head_t         task_head;

    int                 busy;

    pthread_mutex_t     mtx;
    pthread_cond_t      cnd;

    pthread_mutex_t     task_mtx;
    pthread_cond_t      task_cnd;

    pthread_key_t       key; //记录线程错误信息用的worker_set_error

    worker_t            master;
}factory_t;

extern factory_t factory;

int process_center_init(int wkr_max, int wkr_min);

int factory_init(int wkr_max, int wkr_min);

int factory_start();



int process_center_main(worker_t *wkr);

//检测process center中任务数量，如果任务数大于线程数，就增加线程，直到最大线程数
int process_center_check();

//push进来的连接都是主动连接，连到远程服务器的
//不能在event_multi_loop里面用!!!!!!!!!!!
int connect_push(int fd, char *from, int bsize, int event, void *data);


//单进程单线程
int event_single_loop();

//单进程多线程
int event_multi_loop();

int connect_lock(int fd);
int connect_unlock(int fd);


#endif
