#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <openssl/conf.h>



//"一主一从"为一组name_server服务
typedef struct
{
    ip4_addr_t          master;
    ip4_addr_t          slave;

}name_server_t;

//没有主从，平行结构, 每个data_server中，每块盘开1个disk_server
typedef struct
{
//    int                 disk_count;
//    ip4_addr_t          *disk;
    ip4_addr_t          server;

}data_server_t;


//由N个server组成, 每个server中有主从两个服务，写操作必须连接主服务器，读操作可连接主或从服务器
typedef struct
{
    int                 timeout;
    int                 daemon;         //运行方式
    ip4_addr_t          local;          //当前服务绑定的IP和端口

    int                 server_id;      //当前服务ID
    int                 server_type;    //当前进程类型是主还是从，根据配置文件中的IP和端口对应出来

    int                 sync_enable;    //是否同步到"从"服务器

    int                 server_count;   //所有name_server服务器总数
    name_server_t       *server;


    char                db_path[128];   //数据保存位置

    inet_event_t        *ie;

}name_server_conf_t;


//ds_info_t存放每个data_server当前状态信息
typedef struct
{
    int                 state;
    ip4_addr_t          addr;

    uint16_t            server_id;

    uint32_t            disk_size;      //单位为MB
    uint32_t            disk_free;      //单位为MB

    uint32_t            inode_size;     //单位为KB
    uint32_t            inode_free;     //单位为KB

}data_server_info_t;

//name_server_info_t存放每个name_server(主服务)当前状态信息
typedef struct
{
    int                 state;
    ip4_addr_t          addr;

    uint16_t            server_id;
    uint32_t            size;           //存储了多少条记录
    uint32_t            used;           //存储了多少条记录

}name_server_info_t;

//master_server_t存放所有data_server和name_server(主)的状态信息
typedef struct
{
    int                     timeout;
    int                     daemon;         //运行方式
    ip4_addr_t              local;          //当前服务绑定的IP和端口

    struct
    {
        int                 id;             //当前检测到的ID
        int                 count;
        data_server_info_t  *info;
        time_t              start;          //当前次检测开始时间
        int                 work_id;        //当前正在工作的ID, 每次有请求过来+1(负载均衡用)
        inet_task_t         task;           //task;

    }data_server;

    struct
    {
        int                 id;             //当前检测到的ID
        int                 count;
        name_server_info_t  *info;
        time_t              start;          //当前次检测开始时间
        inet_task_t         task;           //task;
    }name_server;

    inet_event_t            *ie;


}master_server_t;


//data server由最多255个组构成，每个组由最多255个服务组成（每个服务对应一个硬盘, 并不一定都是一个服务器上的硬盘）
typedef struct
{
    int                     timeout;
    int                     daemon;         //运行方式

    ip4_addr_t              local;          //当前服务绑定的IP和端口

    int                     server_id;      //当前服务的组ID
    int                     disk_id;        //当前服务的服务器ID

    int                     server_count;   //data server
    data_server_t           *server;

    inet_event_t            *ie;

}data_server_conf_t;


#define NSOPT_ADD_FILE  1
#define NSOPT_MOD_FILE  2
#define NSOPT_DEL_FILE  3

//方便操作的文件信息
typedef struct
{
    int             server;     //服务器ID, 最多255台服务器
    char            name[33];   //块文件名由两部分组成一个128位的文件名
    int             size;       //真实文件大小（不是块文件大小）

}fblock_t;


#define OPERATE_SUCCESS 1000

#define ERR_CMD_NOFOUND -1000
#define ERR_CMD_ARG     -1001
#define ERR_OPEN_FILE   -1002
#define ERR_DEL_FILE    -1003
#define ERR_LINK_FILE   -1004

#define ERR_NS_CONN     -1011
#define ERR_NS_WRITE    -1012
#define ERR_NS_READ     -1013
#define ERR_NS_RETURN   -1014
#define ERR_NS_PROC     -1015
//文件的命名不规范，文件名必须是md5之后的结果（16进制的）
#define ERR_NS_FNAME    -1016
//name server连接不上, 或者检测时返回异常
#define ERR_NS_OFFLINE  -1017

//存储服务器已满
#define ERR_DS_FULL     -1201

#define ERR_NOMEM       -1111


int data_server_config_load(char *fname);

int ns_get_master_addr(char *mid, char **ip, int *port);

int ns_get_slave_addr(char *mid, char **ip, int *port);

int ns_get_server_id(char *mid, int *id);

int ns_config_load(char *fname);


int name_server_config_load(char *fname);

#endif

