#ifndef __MYSQL_FUNC_H__
#define __MYSQL_FUNC_H__

#include <mysql/mysql.h>

typedef struct
{
    MYSQL               srv;
    char                ip[MAX_ADDR];
    int                 port;
    char                name[MAX_ID];
    char                user[MAX_USER];
    char                pass[MAX_PASS];

    pthread_mutex_t     mtx;
    int                 conn;

}mydb_t;

int mydb_exec(mydb_t *, string_t *);

int mydb_query(mydb_t *, string_t *);

int mydb_query_uint32(mydb_t *mm, string_t *cmd, uint32_t *num);

int mydb_query_int(mydb_t *mm, string_t *cmd, int *num);

int mydb_query_long(mydb_t *mm, string_t *cmd, long *num);

char *format_mysql_string(char *src);

//有记录返回1，无返回0， 出错返回MRT_ERR
int mydb_query_exist(mydb_t *mm, string_t *cmd);


#endif
