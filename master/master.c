#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "socket_func.h"
#include "master.h"


extern data_server_conf_t ds_conf;
extern name_server_conf_t ns_conf;

master_server_t ms;




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
            printf("%s version %s, build time %s %s\n", argv[0], MY_VERSION, __DATE__, __TIME__);
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
            snprintf(ms.local.ip, sizeof(ms.local.ip) - 1, "%s", optarg);
            break;

        case 'p':
            ms.local.port = atoi(optarg);
            break;

        case 'd':
            ms.daemon = 1;
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

    if((ms.daemon == 1) && (daemon_init(".") == MRT_ERR))
    {
        log_error("daemon_init error");
        return -1;
    }

    if(logger_init("log", "master", MRT_INFO) == MRT_ERR)
    {
        log_error("logger_init error");
        return -1;
    }

    if(data_server_config_load("etc/data_server.ini") == MRT_ERR)
    {
        log_error("msig_load error");
        return -1;
    }

    if(name_server_config_load("etc/name_server.ini") == MRT_ERR)
    {
        log_error("name_server_config_init error");
        return -1;
    }


    fd = socket_bind_nonblock(ms.local.ip, ms.local.port);
    if(fd == -1)
    {
        log_error("socket_bind_nonblock host:(%s:%d) error:%m",
                  ms.local.ip, ms.local.port);
        return -1;
    }

    ms.ie = inet_event_init(10240, fd);
    if(!ms.ie)
    {
        log_error("inet_event_init error.");
        return -1;
    }

    ms.ie->task_init = request_init;
    ms.ie->task_deinit = request_deinit;
    ms.ie->timeout = 30;

    if(server_check_init() == MRT_ERR)
    {
        log_error("server_check_init error");
        return -1;
    }


    inet_event_loop(ms.ie);

    return 0;
}

