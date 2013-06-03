#ifndef __MASTER_H__
#define __MASTER_H__

#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <openssl/conf.h>
#include "string_func.h"
#include "tdss_config.h"


#define MY_VERSION "0.0.1"

#define SERVER_OFFLINE      1
#define SERVER_ONLINE       2

#define SERVER_TYPE_DATA    0
#define SERVER_TYPE_NAME    1

//SESSION_WAIT:正在等子任务返回
//SESSION_PROC:子任务正在处理返回结果
#define SESSION_BEGIN       0
#define SESSION_READ        1
#define SESSION_WRITE       2
#define SESSION_REPLY       3
#define SESSION_LOOP        4
#define SESSION_PROC        5
#define SESSION_WAIT        6
#define SESSION_END         7



typedef struct session_s session_t;
struct session_s
{
    int             state;          //当前会话的状态， SESSION_BEGIN到SESSION_END
    int             next;           //当reply完之后要执行的动作，监听读还是写, 还是断开
    uint32_t        id;             //一个新命令到来时重新生成id，以记录当前会话

    int             server_type;    //0:检测name_server 1:检测data_server

    int             (*proc)(inet_task_t *);

    inet_task_t     *parent;        //父任务;

    struct
    {
        int         state;          //调用node_server服务执行状态
        int         type;           //类型NSOPT_ADD_FILE：保存文件信息, NSOPT_MOD_FILE:修改，NSOPT_DEL_FILE删除
        string_t    cmd;
        int         (*finish)(inet_task_t *);
    }node_server;

    string_t        input;          //客户端发送过来的命令
    string_t        output;         //要给客户端返回的结果
};

typedef struct
{
    char            *name;
    int             (*func)(inet_task_t *);
}command_t;


int conf_load(char *fname);

int command_process();

void flush();

void reply(char *s);

//功能：
//      获取本机eth1网卡IP
int get_eth1_ip(char *addr, int size);



//调用mail_save时，先创建临时文件，保存成功后link到mid文件上
//参数:
//      c:返回码
#define session_return(fmt, ...) \
    do { \
        ss->state = SESSION_REPLY; \
        string_printf(&ss->output, fmt"\r\n", ##__VA_ARGS__); \
        return SESSION_REPLY; \
    }while(0)

#define NEXT_READ   0
#define NEXT_WRITE  1
#define NEXT_END    -1

#define OPERATE_SUCCESS 1000

#define ERR_CMD_NOFOUND -1000
#define ERR_CMD_ARG     -1001
#define ERR_OPEN_FILE   -1002
#define ERR_DEL_FILE    -1003
#define ERR_LINK_FILE   -1004

#define ERR_FB_ADD      -1100
#define ERR_FB_MOD      -1101
#define ERR_FB_DEL      -1102
#define ERR_FB_REF_INC  -1103
#define ERR_FB_REF_DEC  -1104
#define ERR_FB_STATUS   -1105

#define ERR_NS_CONN     -1011
#define ERR_NS_WRITE    -1012
#define ERR_NS_READ     -1013
#define ERR_NS_RETURN   -1014
#define ERR_NS_PROC     -1015

#define ERR_NOMEM       -1111


int request_init(inet_task_t *it);

int request_process(inet_task_t *it);

int request_deinit(inet_task_t *it);

int data_server_config_load(char *fname);

int ns_save_mid(inet_task_t *it);

int name_server_sync(inet_task_t *it);

int server_check_init();

#endif

