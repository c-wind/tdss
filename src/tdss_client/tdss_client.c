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
           "\n  -a file\tadd file to storage."
           "\n  -g file\tfile file from storage."
           "\n  -d file\tdel file."
           "\n  -f file\tfind file index & storage addr."

           "\n  --version\tdisplay versioin infomation."
           "\n  --help \tdisplay this message.\n", pname);
    _exit(1);
}




int parse_args(int argc, char* argv[])
{
    const char *options = "a:d:f:g:";
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
        case 'g':
            tc.opt = OPT_GET_FILE;
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


int get_storage_from_index(char *fname, ip4_addr_t *addr, fblock_t *fb)
{
    int mfd = -1, port = 0;
    struct stat st;
    M_cpvril(fname);
    M_cpvril(addr);
    char line[MAX_LINE] = {0}, *ip = NULL;


    ns_get_master_addr(fname, strlen(fname), &ip, &port);

    mfd = socket_connect_wait(ip, port, tc.timeout);
    if(mfd == -1)
    {
        log_error("connect name server:(%s:%d) wait:%d error:%m", ip, port, tc.timeout);
        return MRT_ERR;
    }

    snprintf(line, sizeof(line) -1 , "file_info_get name=%s\r\n", fname);

    if(socket_write_wait(mfd, line, strlen(line), tc.timeout) == MRT_ERR)
    {
        log_error("socket_write_wait error:%m");
        return MRT_ERR;
    }

    log_debug("send msg:(%s) to name server:(%s:%d)", line, ip, port);
    s_zero(line);
    if(socket_read_wait(mfd, line, sizeof(line) - 1, tc.timeout) == MRT_ERR)
    {
        log_error("socket_read_wait error:%m");
        return MRT_ERR;
    }

    log_debug("recv msg:(%s) from name server:(%s:%d)", line, ip, port);

    rq_arg_t rq[] = {
        {RQ_TYPE_STR,   "name",     fb->name,  sizeof(fb->name)},
        {RQ_TYPE_STR,   "file",     fb->file,  sizeof(fb->file)},
        {RQ_TYPE_INT,   "offset",   &fb->offset,    0},
        {RQ_TYPE_INT,   "size",     &fb->size,      0},
        {RQ_TYPE_INT,   "server",   &fb->server,    0}
    };

    string_t str = {0} ;

    string_copys(&str, line);

    if(request_parse(&str, rq, 5) == -1)
    {
        log_error("request_parse error");
        return -1;
    }

    log_info("data_server:%d name:%s file:%s offset:%u size:%u",
             fb->server, fb->name, fb->file, fb->offset, fb->size);
    if(ds_get_addr_by_id(fb->server, &ip, &addr->port) == MRT_ERR)
    {
        log_error("no found server_id:%d", fb->server);
        return MRT_ERR;
    }
    snprintf(addr->ip, sizeof(addr->ip) -1, "%s", ip);

    return 0;
}


int add_file_ref_to_index(char *name)
{
    int mfd = -1, rsize = 0, port = 0;
  //  ip4_addr_t addr = {0};
    char line[BUFSIZ] = {0}, md5[33] = {0}, *ip = NULL;
    FILE *fp = NULL;
    fblock_t fb = {0};

    ns_get_master_addr(name, strlen(name), &ip, &port);

    mfd = socket_connect_wait(ip, port, tc.timeout);
    if(mfd == -1)
    {
        log_error("connect addr:(%s:%d) wait:%d error:%m", ip, port, tc.timeout);
        return MRT_ERR;
    }

    snprintf(line, sizeof(line) -1 , "file_ref_inc name=%s\r\n", name);

    if(socket_write_wait(mfd, line, strlen(line), tc.timeout) == MRT_ERR)
    {
        log_error("socket_write_wait error:%m");
        return MRT_ERR;
    }

    log_debug("send msg:(%s) to index:(%s:%d)", line, ip, port);
    s_zero(line);
    if(socket_read_wait(mfd, line, sizeof(line) - 1, tc.timeout) == MRT_ERR)
    {
        log_error("socket_read_wait error:%m");
        return MRT_ERR;
    }

    log_debug("recv msg:(%s) from index:(%s:%d)", line, ip, port);

    if(strncmp(line, "1000 ", 5))
    {
        log_error("file:%s ref inc error:%s", name, line);
        return MRT_ERR;
    }


    log_info("file:%s ref inc ok, index:(%s:%d)", name, ip, port);

    return MRT_OK;
}


int add_file_to_storage(char *fname)
{
    int mfd = -1, rsize = 0;
    ip4_addr_t addr = {0};
    char line[BUFSIZ] = {0}, md5[33] = {0};
    FILE *fp = NULL;
    fblock_t fb = {0};

    if(file_md5(fname, tc.md5) == MRT_ERR)
    {
        log_error("file_md5 error, file:%s", fname);
        return MRT_ERR;
    }
    log_debug("file:%s md5:%u", fname, tc.md5);

    if(get_storage_from_index(fname, &addr, &fb) == MRT_OK)
    {
        log_error("file exist, server:%d name:%s file:%s offset:%u size:%d",
                  fb.server, fb.name, fb.file, fb.offset, fb.size);
        return MRT_ERR;
    }

    if(get_storage_from_master(fname, &addr) == MRT_ERR)
    {
        log_error("get_storage_from_master error");
        return MRT_ERR;
    }

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

    snprintf(line, sizeof(line) -1 , "file_add name=%s size=%u\r\n", tc.md5, tc.size);

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

    log_info("recv msg:(%s) from master:(%s:%d)", line, addr.ip, addr.port);

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

    log_info("recv msg:(%s) from data_server:(%s:%d)", line, addr.ip, addr.port);

    return MRT_OK;
}


int get_file_from_storage(char *fname)
{
    int mfd = -1, rsize = 0, fsize = 0;
    ip4_addr_t addr = {0};
    char line[BUFSIZ] = {0}, *res = NULL;
    FILE *fp = NULL;
    fblock_t fb = {0};

    if(get_storage_from_index(fname, &addr, &fb) == MRT_ERR)
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

    snprintf(line, sizeof(line) -1 , "file_get name=%s file=%s offset=%u size=%u\r\n", fname, fb.file, fb.offset, fb.size);

    if(socket_write_wait(mfd, line, strlen(line), tc.timeout) == MRT_ERR)
    {
        log_error("socket_write_wait error:%m");
        return MRT_ERR;
    }

    log_debug("send msg:(%s) to storage:(%s:%d)", line, addr.ip, addr.port);
    s_zero(line);
    if((rsize = socket_read_wait(mfd, line, sizeof(line) - 1, tc.timeout)) == MRT_ERR)
    {
        log_error("socket_read_wait error:%m");
        return MRT_ERR;
    }

    res = strstr(line, "\r\n");
    if(!res)
    {
        log_error("storage return error:%s", line);
        return MRT_ERR;
    }
    *res = 0;
    res += 2;

    rsize -= res - line;

    log_debug("recv msg:(%s) from master:(%s:%d)", line, addr.ip, addr.port);

    fp = fopen(fname, "w+");
    if(!fp)
    {
        log_error("fopen file:%s error:%m", fname);
        return MRT_ERR;
    }

    if(fwrite(res, sizeof(char), strlen(res), fp) != strlen(res))
    {
        log_error("fwrite file:%s error:%m", fname);
        fclose(fp);
        return MRT_ERR;
    }
    fsize += rsize;

    while(fsize != fb.size)
    {
        s_zero(line);
        if((rsize = socket_read_wait(mfd, line, sizeof(line) - 1, tc.timeout)) == MRT_ERR)
        {
            log_error("read from storage:(%s:%d) error:%m", addr.ip, addr.port);
            fclose(fp);
            return MRT_ERR;
        }

        if(fwrite(line, sizeof(char), rsize, fp) != rsize)
        {
            log_error("fwrite file:%s error:%m", fname);
            fclose(fp);
            return MRT_ERR;
        }
    }

    log_debug("recv file:(%s) from storage:(%s:%d), fsize:%u fb.size:%u",
              fname, addr.ip, addr.port, fsize, fb.size);

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

    /*
    if(logger_init("log", "tdss_client", tc.log_level) == MRT_ERR)
    {
        log_error("logger_init error");
        return -1;
    }
    */

    switch(tc.opt)
    {
    case OPT_ADD_FILE:
        add_file_to_storage(tc.file);
        break;
    case OPT_GET_FILE:
        get_file_from_storage(tc.file);
        break;
    }

    /*
    iret = get_storage_from_master(tc.file, &addr);

    printf("iret:%d, addr:(%s:%d)\n", iret, addr.ip, addr.port);
    */



    return 0;
}

