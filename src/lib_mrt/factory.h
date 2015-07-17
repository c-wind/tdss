#ifndef __FACTORY_H__
#define __FACTORY_H__

#include <pthread.h>
#include <stdint.h>

#define FACTORY_READY 0
#define FACTORY_START 1
#define FACTORY_OVER 2

#define WORKER_READY 0
#define WORKER_START 1
#define WORKER_OVER 2

//event type -------------------------

#define EVENT_NONE 0
#define EVENT_CONN 0
#define EVENT_RECV  1
#define EVENT_SEND  2
#define EVENT_PROC  4
#define EVENT_TIME  8
#define EVENT_OVER  32

#define EVENT_ERROR -1
#define EVENT_TIMEOUT -2
#define EVENT_EINTR -4


#define TASK_CONNECT 0
#define TASK_READ_WAIT 1
#define TASK_READ_OVER 2
#define TASK_WRITE_WAIT 4
#define TASK_WRITE_OVER 8

#define TASK_PROC_WAIT 16
#define TASK_PROC_OVER 32

// -----------------------------------

// operation discription --------------

#define EVENT_ADD 2610
#define EVENT_MOD 2611
#define EVENT_DEL 2612


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
#define callback_set(cb, f, a) \
    do { \
    cb.state = 1; \
    cb.func = f; \
    cb.argv = a; \
    snprintf(cb.name, sizeof(cb.name), "%s", #f); \
    }while(0)

#define task_callback_set(cb, f) \
    do { \
    cb.state = 1; \
    cb.func = f; \
    cb.argv = NULL; \
    snprintf(cb.name, sizeof(cb.name), "%s", #f); \
    }while(0)

#define task_fatal(fmt, ...) logger_write(MRT_FATAL, "FATAL", "%s remote:(%s) fd:%d "fmt, \
                                          __func__, tsk->file.from, tsk->file.fd, ##__VA_ARGS__)
#define task_error(fmt, ...) logger_write(MRT_ERROR, "ERROR", "%s  remote:(%s) fd:%d "fmt, \
                                          __func__, tsk->file.from, tsk->file.fd, ##__VA_ARGS__)
#define task_warning(fmt, ...) logger_write(MRT_WARNING, "WARNING", "%s  remote:(%s) fd:%d "fmt, \
                                          __func__, tsk->file.from, tsk->file.fd, ##__VA_ARGS__)
#define task_info(fmt, ...) logger_write(MRT_INFO, "INFO", "%s  remote:(%s) fd:%d "fmt, \
                                          __func__, tsk->file.from, tsk->file.fd, ##__VA_ARGS__)
#define task_debug(fmt, ...) logger_write(MRT_DEBUG, "DEBUG", "%s  remote:(%s) fd:%d "fmt, \
                                          __func__, tsk->file.from, tsk->file.fd, ##__VA_ARGS__)

#define FACTORY_SINGLE 1
#define FACTORY_MULTI 2

typedef struct worker_s worker_t;
typedef struct task_s task_t;

typedef struct
{
    task_t      **task;
    int         num;
    int         size;

}minheap_t;

typedef struct
{
    int                     state;                  //启用1，未启用0
    void                    *argv;                  //参数
    int                     (*func)();              //回调函数指针
    char                    name[MAX_ID];           //函数名
}callback_t;

typedef struct
{
    int                     state;                  //启用1，未启用0
    void                    *argv;                  //参数
    int                     (*func)(task_t *);      //回调函数指针, 参数为任务
    char                    name[MAX_ID];           //函数名
}task_callback_t;

struct worker_s
{
    int8_t                  state;
    time_t                  start;

    pthread_t               idx;
    pthread_mutex_t         mtx;
    pthread_cond_t          cnd;

    callback_t              proc;
    char                    error_msg[MAX_LINE];          //记录错误信息

    M_list_entry(worker_t);
};



struct task_s
{
    uint32_t                id;             //ID是循环使用的，只保存在32位之内不重复就可以了, 连接进入时生成
    int                     idx;            //最小堆中的索引，每个任务都有进入最小堆的时候。。。
    int                     stat;           //当前任务执行的状态TASK_READ_(WAIT/OVER), TASK_WRITE_(WAIT/OVER), TASK_PROC_(WAIT/OVER)
    time_t                  last;
    int                     event;

    file_handle_t           file;

//    buffer_t                *buf;
    task_t                  *child;

//    task_callback_t         proc;

    void                    *data;          //由调用函数自行处理, 可存放session信息

    M_list_entry(task_t);
};



typedef struct
{
    int                     state;          //1:启动, -1停止
    int                     worker_num;     //应该为1
    int                     worker_max;     //最大线程数也必须为1

    int                     epfd;
    int                     lsfd;
    int                     online;         //当前在线连接数

    int                     timeout;
    int                     task_num;
    int                     task_max;
    task_t                  *task_array;
    uint32_t                id_inc;         //ID自增变量

    callback_t              proc;           //有事件就调用这个

    pthread_mutex_t         mtx;

    callback_t              accept_before;      //连接接入之前
    callback_t              accept_after;       //连接接入之后

    callback_t              read_before;        //连接可读
    callback_t              read_after;         //连接已读完

    callback_t              write_before;       //连接可写
    callback_t              write_after;        //连接已写完

    callback_t              close_before;       //连接将要关闭
    callback_t              close_after;        //连接已关闭

    callback_t              init;       //用于初始化当前任务资源
    callback_t              deinit;     //用于释放当前任务资源

    minheap_t               heap;       //事件堆

}event_center_t;


typedef struct
{
    int                     worker_num;
    int                     worker_max;
    int                     worker_min;

    int                     task_num;
    int                     busy;

    pthread_mutex_t         mtx;
    pthread_cond_t          cnd;

    M_list_head(task_t);

    callback_t      proc;      //处理请求

}process_center_t;


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
    pthread_key_t       key;

    server_config_t     conf;


    pthread_mutex_t     worker_mutex;
    worker_t            master;

    event_center_t      event_center;

    M_list_head(worker_t);

}factory_t;

extern factory_t factory;

//不管函数执行成功失败都会清空ec
int event_center_init(int max_conn, int timeout, char *host, int port,
                      callback_t accept_before, callback_t accept_after,
                      callback_t read_before,   callback_t read_after,
                      callback_t write_before,  callback_t write_after,
                      callback_t close_before,  callback_t close_after);

int process_center_init(int wkr_max, int wkr_min, callback_t proc);

int factory_init(server_config_t sconf, callback_t proc);

int factory_start();


//将任务添加到监听列表
int event_insert(task_t *tsk);

int process_center_loop();

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
