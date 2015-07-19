#include "global.h"
#include "macro_const.h"
#include "macro_func.h"
#include "comm_func.h"
#include <syslog.h>
#include <stdarg.h>
#include <execinfo.h>


extern char *error_msg;

S_logger logger;

S_level log_level_ary[] = {
    {.level = MRT_DEBUG,    .desc = "DEBUG"},
    {.level = MRT_INFO,     .desc = "INFO"},
    {.level = MRT_WARNING,  .desc = "WARNING"},
    {.level = MRT_ERROR,    .desc = "ERROR"},
    {.level = MRT_FATAL,    .desc = "FATAL"}
};

static int logger_start = 0;

static int send_error(const char *fmt, ...)
{
    char err_buf[MID_STR] = {0};
    va_list ap;

    va_start (ap, fmt);
    (void) vsnprintf (err_buf, sizeof(err_buf), fmt, ap);
    va_end (ap);

    snprintf(err_buf + strlen(err_buf), sizeof(err_buf) - strlen(err_buf), "\n");

    syslog(LOG_ERR, "%s", err_buf);

    return MRT_OK;
}




static inline int level_check(int err_code)
{
    return (err_code < logger.level ) ? MRT_ERR : MRT_OK;
}


/*
static char *level_to_emsg(uint8_t level)
{
    return (level < 1 || level > 5) ? NULL : log_level_ary[level - 1].desc;
}
*/


/*
static int lmsg_to_level(char *msg)
{
    int i=0;

    for(; i< 5; i++)
        if(strcasecmp(log_level_ary[i].desc, msg) == 0)
            return log_level_ary[i].level;

    send_error("no found log level:%s.", msg);

    return MRT_ERR;
}
*/


static int path_check(char *path)
{
    if(access(path, W_OK) == MRT_ERR)
    {
        send_error("%s path:%s can't write, %m.", __func__, path);
        return MRT_ERR;
    }

    return MRT_OK;
}




int open_log()
{
    time_t now;
    struct tm *ntm;
    mode_t  old_umask;
    int logfd = -1;
    char ctm[MAX_TIME] = {0};

    time(&now);
    ntm = localtime(&now);

    strftime(ctm, sizeof(ctm), "%Y%m%d", ntm);

    if((logger.nfd > 0) && !strcmp(logger.otm, ctm))
    {
        logfd = logger.nfd;
    }
    else
    {
        if(logger.nfd > 0) close(logger.nfd);

        char nstr[MAX_PATH] = {0};


        sprintf(nstr, "%s/%s_%s.log",
                logger.path, logger.prefix, ctm);

        old_umask = umask(0111);

        if ((logfd = open(nstr, O_WRONLY|O_APPEND|O_CREAT, 0666)) == MRT_ERR )
        {
            send_error("%s open file:[%s] error, %m.", __func__, nstr);
            umask(old_umask);
            return MRT_ERR;
        }

        s_zero(logger.otm);
        sprintf(logger.otm, "%s", ctm);
        logger.nfd = logfd;
        umask(old_umask);
    }


    return MRT_OK;
}



int logger_init(char *path, char *prefix, int level)
{
//    char hostname[MAX_USER] = {0};

    if(!path || !*path || !prefix || !*prefix)
    {
        send_error("%s parameter error.\n", __func__);
        return MRT_ERR;
    }

    s_zero(logger);

    M_cirinl(path_check(path));
    logger.level = level;

    /*
    if(gethostname(hostname, sizeof(hostname) - 1) == MRT_ERR)
    {
        send_error("%s get host name error, %m.\n", __func__);
        return MRT_ERR;
    }

    */

    strcpy(logger.path, path);
    //snprintf(logger.prefix, sizeof(logger.prefix), "%s_%s", hostname, prefix);
    snprintf(logger.prefix, sizeof(logger.prefix), "%s", prefix);

    if((open_log() == MRT_ERR))
    {
        send_error("%s open logfile error.\n", __func__);
        return MRT_ERR;
    }

    logger_start = 1;

    return MRT_OK;
}


void line_format(char *src)
{
    char *ps = src;

    while(*ps)
    {
        if(*ps == 10 || *ps == 13)
            *ps = '.';
        ps++;
    }

}

int logger_write(int type, char *level, const char *fmt, ...)
{
    va_list ap;
    char tbuf[MAX_LINE * 4] = {0};
    char *pbuf = tbuf;
    time_t now;
    uint16_t len = 0;
    struct tm ctime;

    M_cirinl(level_check(type));

    time(&now);
    s_zero(ctime);
    localtime_r(&now, &ctime);

    snprintf(tbuf, sizeof(tbuf), "%.2d:%.2d:%.2d [%u] [%s] ",
            ctime.tm_hour, ctime.tm_min, ctime.tm_sec, getpid(), level);

    pbuf = tbuf + strlen(tbuf);
    va_start (ap, fmt);
    (void) vsnprintf (pbuf, sizeof(tbuf), fmt, ap);
    va_end (ap);

    len = strlen(tbuf);
    tbuf[len] = '\n';

    if(!logger_start)
    {
        printf("%s\n", tbuf);
        return MRT_SUC;
    }

    if(open_log() == MRT_OK)
    {
        if(write(logger.nfd, tbuf, len + 1) < 0)
        {
            send_error("Write log file error, file:%s, %m.", logger.prefix);
        }
    }

    return MRT_SUC;
}

int logger_destroy()
{
    close(logger.nfd);
    s_zero(logger);

    return MRT_OK;
}


void log_backtrace()
{
    void *array[16];
    size_t size;

    size = backtrace(array, 16);

    backtrace_symbols_fd(array, size, logger.nfd);
}



