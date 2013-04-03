#ifndef __MACRO_CONST_H__
#define __MACRO_CONST_H__


//------------ 返回值 ---------------
#define MRT_OK 0
#define MRT_SUC 0
#define MRT_ERR -1
#define ERR_TIMEOUT -2

#define ERR_FAILED -1
#define ERR_SKIP -2
#define ERR_POSITIVE_PRELIMARY -2
#define ERR_RETRY -3
#define ERR_PERMANENT -4
#define ERR_RECONNECT -8

#define MRT_NULL NULL
// ----------------------------------

// ------------ 内存池类型 ----------
#define MEM_ONCE 1
#define MEM_TRUE 2

#define MAX_ALIGN 32768

// ------------ 日志级别 ------------
#define MRT_DEBUG 1
#define MRT_INFO 2
#define MRT_WARNING 3
#define MRT_ERROR 4
#define MRT_FATAL 5

// ---------- 字节换算 --------------
#define M_1MB   1048576
#define M_16MB  16777216
#define M_64KB  65536
#define M_128KB 131072
#define M_32KB  32768
#define M_16KB  16384
#define M_8KB   8192
#define M_4KB   4096
#define M_1KB   1024

// --------- 日期，时间 ------------
#define MAX_DAY 31
#define MAX_MONTH 12

//文件相关
#define MAX_FILE_NAME 64

#define ERR_FILE_EMPTY -1000

#define OP_FILE_READ    2
#define OP_FILE_WRITE   4

// ----------------------------------

#define CRLF "\r\n"


// ----------- 各种最大长度 --------

#define MAX_MEM 9000

#define LONG_STR 2048
#define MID_STR 1024
#define SHORT_STR 512
#define SMALL_STR 256

#define MAX_URL SMALL_STR
#define MAX_CAPTION 128
#define MAX_TITLE SMALL_STR
#define MAX_DATE 32
#define MAX_TIME 64


#define MAX_HEAD MID_STR
#define MAX_IP 32
#define MAX_PORT 8
#define MAX_ADDR SMALL_STR
#define MAX_PATH MID_STR
#define MIN_PATH SMALL_STR

#define MAX_LIST M_1MB
#define MAX_MD5 32

#define ONE_HOUR 3600

#define MAX_LINE MID_STR
#define MAX_USER 128
#define MAX_ID MAX_USER
#define SMALL_ID 32
#define MAX_PASS 64
#define MAX_CMD MID_STR

#define MEM_ALIGN(size, asize) \
    (((size) + (asize) - 1) & ~((unsigned int) (asize) -1))



#endif
