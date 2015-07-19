#include "global.h"
#include "tdss_config.h"
#include "socket_func.h"
#include "master.h"


extern data_server_conf_t ds_conf;
extern name_server_conf_t ns_conf;
extern master_server_conf_t ms_conf;





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
            snprintf(ms_conf.local.ip, sizeof(ms_conf.local.ip) - 1, "%s", optarg);
            break;

        case 'p':
            ms_conf.local.port = atoi(optarg);
            break;

        case 'd':
            ms_conf.daemon = 1;
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

    if((ms_conf.daemon == 1) && (daemon_init(".") == MRT_ERR))
    {
        log_error("daemon_init error");
        return -1;
    }

    if(master_server_config_load("etc/master_server.ini") == MRT_ERR)
    {
        log_error("master_server_config_load error");
        return -1;
    }

    if(logger_init("log", "master", ms_conf.log_level) == MRT_ERR)
    {
        log_error("logger_init error");
        return -1;
    }

    if(data_server_config_load("etc/data_server.ini") == MRT_ERR)
    {
        log_error("data_server_config_load error");
        return -1;
    }

    if(name_server_config_load("etc/name_server.ini") == MRT_ERR)
    {
        log_error("name_server_config_load error");
        return -1;
    }

    callback_t cb_accept, cb_request, cb_response, cb_close;

    callback_set(cb_accept, on_accept);
    callback_set(cb_request, on_request);
    callback_set(cb_response, on_response);
    callback_set(cb_close, on_close);




    if(event_center_init(MAX_CONN, ms_conf.timeout,  ms_conf.local.ip, ms_conf.local.port,
                          callback_t on_accept,     //在接收完连接时调用
                      callback_t on_request,    //在接收到用户数据之后调用
                      callback_t on_response,   //在向用户发送完数据之后调用，不管当前发送缓冲区有没有需要发送的数据，都会调用
                      callback_t on_close      //在关闭连接之前调用
                     )

    ms_conf.ie->task_init = request_init;
    ms_conf.ie->task_deinit = request_deinit;
    ms_conf.ie->timeout = ms_conf.timeout;

    if(server_check_init() == MRT_ERR)
    {
        log_error("server_check_init error");
        return -1;
    }


    inet_event_loop(ms_conf.ie);

    return 0;
}

