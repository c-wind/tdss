#ifndef __MACRO_FUNC_H__
#define __MACRO_FUNC_H__

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define SOCK_ERROR(x) (x == ERR_RECONNECT || x == ERR_TIMEOUT)

#define FTP_ERROR(x) (x == ERR_RETRY || x == ERR_PERMANENT)

//数组清空用p_zero,结构体清空用s_zero
#define p_zero(obj) memset(obj, 0, sizeof(*obj))
#define s_zero(obj) memset(&obj, 0, sizeof(obj))


#define MEM_ALIGN(size, asize) \
    (((size) + (asize) - 1) & ~((unsigned int) (asize) -1))

//检测int返回string不记录日志
#define M_cirvnl(val) \
    if((val) == -1) \
return NULL;

//检测int返回int不记录日志
#define M_cirinl(val) \
    if((val) == -1) \
return -1;

#define M_csrinl(val) \
    if(!(val)||!*(val)) \
return -1;

#define M_csrvnl(val) \
    if(!(val)||!*(val)) \
return NULL;

//检测int返回int记录日志
#define M_ciril(val, msg, ...) \
    if((val) == -1) \
{ \
    log_error("%s "msg" %m.", __func__, ##__VA_ARGS__); \
    return -1; \
}

//检测int返回记录日志
#define M_cirvl(val, msg, ...) \
    if((val) == -1) \
{ \
    log_error("%s "msg, __func__, ##__VA_ARGS__); \
    return NULL; \
}

//检测int返回记录日志
#define M_cirvs(val, msg, ...) \
    if((val) == -1) \
{ \
    worker_set_error("%s "msg, __func__, ##__VA_ARGS__); \
    return NULL; \
}


//检测string返回int记录日志
#define M_csril(val, msg,  ...) \
    if(!(val)||!*(val)) \
{ \
    log_error("%s "msg" %m.", __func__, ##__VA_ARGS__); \
    return -1; \
}

//检测string返回string记录日志
#define M_csrvl(val, msg, ...) \
    if(!(val)||!*(val)) \
{ \
    log_error("%s "msg" %m.", __func__, ##__VA_ARGS__); \
    return NULL; \
}

//检测string为空则continue不记录日志
#define M_cscnl(val) \
    if(!(val) || !*(val)) \
continue;

//检测string为空则continue记录日志
#define M_cscl(val, msg, ...) \
    if(!(val) || !*(val)) \
{ \
    log_info("%s "msg, __func__, ##__VA_ARGS__); \
    continue; \
}


//检测int为-1则continue不记录日志
#define M_cicnl(val) \
    if((val) == -1) \
continue;

//检测int为-1则continue记录日志
#define M_cicl(val, msg, ...) \
    if((val) == -1) \
{ \
    log_info("%s "msg, __func__, ##__VA_ARGS__); \
    continue; \
}



//检测void *返回int记录日志
#define M_cvril(val, msg, ...) \
    if(!(val)) \
{ \
    log_error("%s "msg, __func__, ##__VA_ARGS__); \
    return -1; \
}

//检测void *返回int, 设置错误信息
#define M_cvris(val, msg, ...) \
    if(!(val)) \
{ \
    worker_set_error("%s "msg, __func__, ##__VA_ARGS__); \
    return -1; \
}

//检测void *返回int不记录日志
#define M_cvrinl(val) \
    if(!(val)) \
{ \
    return -1; \
}


//检测void *返回void *记录日志
#define M_cvrvl(val, msg, ...) \
    if(!(val)) \
{ \
    log_error("%s "msg" %m", __func__, ##__VA_ARGS__); \
    return NULL; \
}

//检测void *返回void *设置错误信息
#define M_cvrvs(val, msg, ...) \
    if(!(val)) \
{ \
    worker_set_error("%s "msg, __func__, ##__VA_ARGS__); \
    return NULL; \
}

//检测void *返回void *不记录日志
#define M_cvrvnl(val) \
    if(!(val)) \
{ \
    return NULL; \
}

//检测int返回int设置错误信息
#define M_ciris(val, msg, ...) \
    if((val) == MRT_ERR) \
{ \
    worker_set_error("%s "msg, __func__, ##__VA_ARGS__); \
    return MRT_ERR; \
}


// ---------------------------------------------




//------------ 检测传入参数用 -----------------

//检测传入参数字符串是否为空，记录日志，返回类型为int
#define M_cpsril(val) \
    if(!(val)||!*(val)) \
{ \
    log_error("parameter "#val" is null."); \
    return -1; \
}
//参数字符串是否为空，不记录日志，返回类型为int
#define M_cpsrinl(val) \
    if(!(val)||!*(val)) \
    return -1;



//检测传入参数是否为-1
#define M_cpiril(val) \
    if((val) == -1) \
{ \
    log_error("parameter "#val" is -1."); \
    return -1; \
}


//检测传入参数字符串是否为空，记录日志，返回类型为void *
#define M_cpsrvl(val) \
    if(!(val)||!*(val)) \
{ \
    log_error("parameter "#val" is null."); \
    return NULL; \
}

//检测传入参数字符串是否为空，并设置错误信息，不记录日志，返回类型为void *
#define M_cpsrvs(val) \
    if(!(val)||!*(val)) \
{ \
    set_error("parameter "#val" is null."); \
    return NULL; \
}


//检测传入参数是否为空，记录日志，返回类型为void *
#define M_cpvrvl(val) \
    if(!(val)) \
{ \
    log_error("%s parameter is null.", __func__); \
    return NULL; \
}

//检测传入参数是否为空，并设置错误信息，不记录日志，返回类型为int
#define M_cpvris(val) \
    if(!(val)) \
{ \
    set_error("parameter "#val" is null."); \
    return -1; \
}


//检测传入参数是否为空，记录日志，返回类型为int
#define M_cpvril(val) \
    if(!(val)) \
{ \
    log_error("parameter "#val" is null."); \
    return -1; \
}


//检测文件存在
#define M_file_exist(file) access(file, F_OK)

//检测文件读权限
#define M_file_read(file) access(file, R_OK)

//检测文件写权限
#define M_file_write(file) access(file, W_OK)

//检测文件执行权限
#define M_file_exec(file) access(file, X_OK)




#ifdef USE_MEM_POOL
#define M_alloc(val) memory_alloc(val, __LINE__, (char *)__func__)
#define M_realloc(val, size) memory_realloc(val, size, __LINE__, (char *)__func__)
#define M_free(val) memory_free(val,  __LINE__, (char *)__func__)
#else
#define M_alloc(val) calloc(1, val)
#define M_realloc(val, size) realloc(val, size)
#define M_free(val) free(val)
#endif

#ifndef __DEBUG__
#define log_fatal(fmt, ...) logger_write(MRT_FATAL, "FATAL", "%s "fmt, __func__, ##__VA_ARGS__)
#define log_error(fmt, ...) logger_write(MRT_ERROR, "ERROR", "%s "fmt, __func__, ##__VA_ARGS__)
#define log_warning(fmt, ...) logger_write(MRT_WARNING, "WARNING", "%s "fmt, __func__, ##__VA_ARGS__)
#define log_info(fmt, ...) logger_write(MRT_INFO, "INFO", "%s "fmt, __func__, ##__VA_ARGS__)
#define log_debug(fmt, ...) logger_write(MRT_DEBUG, "DEBUG", "%s "fmt, __func__, ##__VA_ARGS__)

#else
#define log_fatal(fmt, ...) printf(fmt"\n", __VA_ARGS__)
#define log_error(fmt, ...) printf(fmt"\n", __VA_ARGS__)
#define log_warning(fmt, ...) printf(fmt"\n", __VA_ARGS__)
#define log_info(fmt, ...) printf(fmt"\n", __VA_ARGS__)
#define log_debug(fmt, ...) printf(fmt"\n", __VA_ARGS__)
#endif



#define M_strdup(src, len) str_newcpy(src, len)

#endif

