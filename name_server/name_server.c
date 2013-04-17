#include "global.h"
#include "inet_event.h"
#include "socket_func.h"
#include "name_server.h"
#include "tdss_config.h"
#include "fblock_mem.h"


extern data_server_conf_t ds_conf;
extern name_server_conf_t ns_conf;


//static time_t session_begin;


#define ERROR_DEFINE(ename, eno, emsg) int errnum_##ename = eno; char *errmsg_##ename = emsg

ERROR_DEFINE(read,          1001,   "read error");
ERROR_DEFINE(write,         1002,   "write error");
ERROR_DEFINE(timeout,       1003,   "timeout");
ERROR_DEFINE(max_error,     1004,   "too many error");
ERROR_DEFINE(connectdb,     1005,   "connect db error");


#define session_exit(ename) \
    do { \
        log_error("session exit, errno:%d error:%s", errnum_##ename, errmsg_##ename); \
        _exit(0); \
    } while(0)


void server_usage(char *pname)
{
    printf("Usage: %s -h host -p port [-d] [--help] [--version]\n"
           "\n  -h host\tbind ip."
           "\n  -p port\tbind port."
           "\n  -d     \trun as daemon.\n"
           "\n  --version\tdisplay versioin infomation."
           "\n  --help \tdisplay this message.\n", pname);
    _exit(1);
}


int parse_args(int argc, char* argv[])
{
    const char *options = "h:p:d";
    int opt;

    if(argc == 2)
    {
        if(!strncasecmp(argv[1], "--help", 6))
        {
            server_usage(argv[0]);
        }

        if(!strncasecmp(argv[1], "--version", 9))
        {
            printf("%s version %s, build time %s %s\n", argv[0], VERSION, __DATE__, __TIME__);
            _exit(0);
        }
    }

    if(argc < 3)
    {
        server_usage(argv[0]);
    }

    while ((opt = getopt(argc, argv, options)) != EOF)
    {
        switch(opt)
        {
        case 'h':
            snprintf(ns_conf.local.ip, sizeof(ns_conf.local.ip) - 1, "%s", optarg);
            break;

        case 'p':
            ns_conf.local.port = atoi(optarg);
            break;

        case 'd':
            ns_conf.daemon = 1;
            break;

        default:
            server_usage(argv[0]);
            break;
        }

    }

    return MRT_SUC;
}






int main(int argc, char *argv[])
{
    int fd = -1;

    parse_args(argc, argv);

    if((ns_conf.daemon == 1) && (daemon_init(".") == MRT_ERR))
    {
        log_error("daemon_init error");
        return -1;
    }

    if(logger_init("log", "name_server", MRT_DEBUG) == MRT_ERR)
    {
        log_error("logger_init error");
        return -1;
    }


    if(data_server_config_load("etc/data_server.ini") == MRT_ERR)
    {
        log_error("ds_config_load error");
        return -1;
    }

    if(name_server_config_load("etc/name_server.ini") == MRT_ERR)
    {
        log_error("name_server_config_init error");
        return -1;
    }


    if(fblock_mem_init(ns_conf.db_path) == MRT_ERR)
    {
        log_error("fblock_mem_init error");
        return -1;
    }

    fd = socket_bind_nonblock(ns_conf.local.ip, ns_conf.local.port);
    if(fd == -1)
    {
        log_error("socket_bind_nonblock host:(%s:%d) error:%m",
                  ns_conf.local.ip, ns_conf.local.port);
        return -1;
    }

    ns_conf.ie = inet_event_init(10240, fd);
    if(!ns_conf.ie)
    {
        log_error("inet_event_init error.");
        return -1;
    }


    ns_conf.ie->task_init = request_init;
    ns_conf.ie->task_deinit = request_deinit;
    ns_conf.ie->timeout = 30;

    inet_event_loop(ns_conf.ie);

    return 0;
}

