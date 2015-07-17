#ifndef __LOG_FUNC_H__
#define __LOG_FUNC_H__

typedef struct S_logger
{
    char                path[MAX_PATH];
    char                prefix[MAX_ID];
    int                 level;
    char                otm[MAX_TIME];
    int                 nfd;
    pthread_mutex_t     mtx;

}S_logger;

typedef struct S_level
{
    uint16_t     level;
    char        *desc;
}S_level;

// ----------- 函数声明 ----------------

// 初始化日志系统
int logger_init(char *path, char *prefix, int level);

// 记录日志
int logger_write(int type, char *level, const char *fmt, ...) __attribute__((format(printf,3,4)));

// 结果日志记录
int logger_destroy();

void log_backtrace();
// -------------------------------------

#endif
