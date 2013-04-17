#include "global.h"
#include "inet_event.h"
#include "tdss_config.h"
#include "socket_func.h"
#include "tdss_client.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


tdss_client_t tc;


void server_usage(char *pname)
{
    printf("Usage: %s <-a|-d|-f> file [--help] [--version]\n"
           "\n  -a file\tadd file."
           "\n  -d file\tdel file."
           "\n  -f file\tfind file index & storage addr."

           "\n  --version\tdisplay versioin infomation."
           "\n  --help \tdisplay this message.\n", pname);
    _exit(1);
}




int parse_args(int argc, char* argv[])
{
    const char *options = "a:d:f:";
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
        case 'a':
            tc.opt = OPT_ADD_FILE;
            snprintf(tc.file, sizeof(tc.file) - 1, "%s", optarg);
            break;
        case 'd':
            tc.opt = OPT_DEL_FILE;
            snprintf(tc.file, sizeof(tc.file) - 1, "%s", optarg);
            break;
        case 'f':
            tc.opt = OPT_FIND_FILE;
            snprintf(tc.file, sizeof(tc.file) - 1, "%s", optarg);
            break;

        default:
            server_usage(argv[0]);
            break;
        }
    }

    return MRT_SUC;
}





int get_storage_from_master(char *fname, ip4_addr_t *addr)
{
    int mfd = -1;
    struct stat st;
    M_cpvril(fname);
    M_cpvril(addr);
    char line[MAX_LINE] = {0};

    mfd = socket_connect_wait(tc.master.ip, tc.master.port, tc.timeout);
    if(mfd == -1)
    {
        log_error("connect master:(%s:%d) wait:%d error:%m", tc.master.ip, tc.master.port, tc.timeout);
        return MRT_ERR;
    }

    if(stat(fname, &st) == -1)
    {
        log_error("stat file:%s error:%m", fname, st);
        return MRT_ERR;
    }
    log_debug("file:%s size:%u", fname, (uint32_t)st.st_size);
    tc.size = (uint32_t)st.st_size;

    if(file_md5(fname, tc.md5) == MRT_ERR)
    {
        log_error("file_md5 error, file:%s", fname);
        return MRT_ERR;
    }
    log_debug("file:%s md5:%u", fname, tc.md5);

    snprintf(line, sizeof(line) -1 , "server_find name=%s size=%u\r\n", tc.md5, (uint32_t)st.st_size);

    if(socket_write_wait(mfd, line, strlen(line), tc.timeout) == MRT_ERR)
    {
        log_error("socket_write_wait error:%m");
        return MRT_ERR;
    }

    log_debug("send msg:(%s) to master:(%s:%d)", line, tc.master.ip, tc.master.port);
    s_zero(line);
    if(socket_read_wait(mfd, line, sizeof(line) - 1, tc.timeout) == MRT_ERR)
    {
        log_error("socket_read_wait error:%m");
        return MRT_ERR;
    }

    log_debug("recv msg:(%s) from master:(%s:%d)", line, tc.master.ip, tc.master.port);


    rq_arg_t rq[] = {
        {RQ_TYPE_STR,   "server",   addr->ip,  sizeof(addr->ip)},
        {RQ_TYPE_INT,   "port",     &(addr->port),  0}
    };

    string_t str = {0} ;

    string_copys(&str, line);

    if(request_parse(&str, rq, 2) == -1)
    {
        log_error("request_parse error");
        return -1;
    }


    return MRT_OK;
}


int get_storage_from_index(char *fname, ip4_addr_t *addr)
{
    int mfd = -1;
    struct stat st;
    M_cpvril(fname);
    M_cpvril(addr);
    char line[MAX_LINE] = {0}, md5[33] = {0};

    mfd = socket_connect_wait(tc.master.ip, tc.master.port, tc.timeout);
    if(mfd == -1)
    {
        log_error("connect master:(%s:%d) wait:%d error:%m", tc.master.ip, tc.master.port, tc.timeout);
        return MRT_ERR;
    }

    if(stat(fname, &st) == -1)
    {
        log_error("stat file:%s error:%m", fname, st);
        return MRT_ERR;
    }

    log_debug("file:%s size:%u", fname, (uint32_t)st.st_size);
    if(file_md5(fname, md5) == MRT_ERR)
    {
        log_error("file_md5 error, file:%s", fname);
        return MRT_ERR;
    }
    log_debug("file:%s md5:%u", fname, md5);

    snprintf(line, sizeof(line) -1 , "server_find name=%s size=%u\r\n", md5, (uint32_t)st.st_size);

    if(socket_write_wait(mfd, line, strlen(line), tc.timeout) == MRT_ERR)
    {
        log_error("socket_write_wait error:%m");
        return MRT_ERR;
    }

    log_debug("send msg:(%s) to master:(%s:%d)", line, tc.master.ip, tc.master.port);
    s_zero(line);
    if(socket_read_wait(mfd, line, sizeof(line) - 1, tc.timeout) == MRT_ERR)
    {
        log_error("socket_read_wait error:%m");
        return MRT_ERR;
    }

    log_debug("recv msg:(%s) from master:(%s:%d)", line, tc.master.ip, tc.master.port);


    rq_arg_t rq[] = {
        {RQ_TYPE_STR,   "server",   addr->ip,  sizeof(addr->ip)},
        {RQ_TYPE_INT,   "port",     &(addr->port),  0}
    };

    string_t str = {0} ;

    string_copys(&str, line);

    if(request_parse(&str, rq, 2) == -1)
    {
        log_error("request_parse error");
        return -1;
    }


    return 0;
}


int add_file_to_storage(char *fname)
{
    int mfd = -1, rsize = 0;
    ip4_addr_t addr = {0};
    char line[BUFSIZ] = {0}, md5[33] = {0};
    FILE *fp = NULL;

    if(get_storage_from_master(fname, &addr) == MRT_ERR)
    {
        log_error("get_storage_from_master error");
        return MRT_ERR;
    }

    mfd = socket_connect_wait(addr.ip, addr.port, tc.timeout);
    if(mfd == -1)
    {
        log_error("connect addr:(%s:%d) wait:%d error:%m", addr.ip, addr.port, tc.timeout);
        return MRT_ERR;
    }

    snprintf(line, sizeof(line) -1 , "file_save name=%s size=%u\r\n", tc.md5, tc.size);

    if(socket_write_wait(mfd, line, strlen(line), tc.timeout) == MRT_ERR)
    {
        log_error("socket_write_wait error:%m");
        return MRT_ERR;
    }

    log_debug("send msg:(%s) to master:(%s:%d)", line, addr.ip, addr.port);
    s_zero(line);
    if(socket_read_wait(mfd, line, sizeof(line) - 1, tc.timeout) == MRT_ERR)
    {
        log_error("socket_read_wait error:%m");
        return MRT_ERR;
    }

    log_debug("recv msg:(%s) from master:(%s:%d)", line, addr.ip, addr.port);

    s_zero(line);
    fp = fopen(fname, "r");
    if(!fp)
    {
        log_error("fopen file:%s error:%m", fname);
        return MRT_ERR;
    }

    while((rsize = fread(line, sizeof(char), sizeof(line), fp)))
    {
        if(socket_write_wait(mfd, line, rsize, tc.timeout) == MRT_ERR)
        {
            log_error("socket_write_wait file error:%m");
            fclose(fp);
            return MRT_ERR;
        }
    }

    s_zero(line);
    if(socket_read_wait(mfd, line, sizeof(line) - 1, tc.timeout) == MRT_ERR)
    {
        log_error("socket_read_wait error:%m");
        return MRT_ERR;
    }

    log_debug("recv msg:(%s) from master:(%s:%d)", line, addr.ip, addr.port);

    return MRT_OK;
}





int main(int argc, char *argv[])
{
    int iret = -1;
    ip4_addr_t addr = {0};

    s_zero(tc);

    parse_args(argc, argv);

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


    if(tdss_client_config_load("etc/client.ini") == MRT_ERR)
    {
        log_error("tdss_client_config_load error");
        return -1;
    }

    if(logger_init("log", "tdss_client", tc.log_level) == MRT_ERR)
    {
        log_error("logger_init error");
        return -1;
    }


    add_file_to_storage(tc.file);
    /*
    iret = get_storage_from_master(tc.file, &addr);

    printf("iret:%d, addr:(%s:%d)\n", iret, addr.ip, addr.port);
    */



    return 0;
}

