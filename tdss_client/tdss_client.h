#ifndef __TDSS_CLIENT_H__
#define __TDSS_CLIENT_H__

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

#define OPT_ADD_FILE    1
#define OPT_DEL_FILE    2
#define OPT_FIND_FILE   3

typedef struct
{
    int         opt;
    char        file[MAX_LINE];

    int         log_level;
    int         timeout;
    ip4_addr_t  master;


    char        md5[MAX_ID];
    uint32_t    size;

}tdss_client_t;

#endif

