#include "global.h"


static int mydb_connect(mydb_t *mm)
{
    int time_out = 5;
    MYSQL *srv = &(mm->srv);

    if(mm->conn)
    {
        if(!mysql_ping(srv))
            return MRT_SUC;
        set_error("mysql ping error:%s, will reconnect", mysql_error(&mm->srv));
        mysql_close(srv);
        mm->conn = 0;
    }

    if(!mysql_init(&mm->srv))
    {
        set_error("mysql_init error:%s.", mysql_error(&mm->srv));
        return MRT_ERR;
    }

    if(mysql_options(&mm->srv, MYSQL_OPT_CONNECT_TIMEOUT, (char *)&time_out))
    {
        set_error("mysql_options error:%s.", mysql_error(&mm->srv));
        return MRT_ERR;
    }

    if(mysql_options(&mm->srv, MYSQL_SET_CHARSET_NAME, "utf8"))
    {
        set_error("mysql_options error:%s.", mysql_error(&mm->srv));
        return MRT_ERR;
    }

    if(!mysql_real_connect(&mm->srv, mm->ip,
                           mm->user, mm->pass,
                           mm->name, mm->port, NULL, CLIENT_MULTI_STATEMENTS))
    {
        set_error("mysql connect error:%s", mysql_error(&mm->srv));
        return MRT_ERR;
    }

    mysql_query(&mm->srv, "SET NAMES utf8");

    mm->conn = 1;

    return MRT_OK;
}


int mydb_exec(mydb_t *mm, string_t *cmd)
{
    int num;
    MYSQL_RES* mysql_res;
    M_cpvril(mm);

    MYSQL *srv = &(mm->srv);


    M_ciril(mydb_connect(mm), "connect to mysql error.");

    if(mysql_query(srv, cmd->str) != MRT_OK)
    {
        set_error("mysql error:%s", mysql_error(srv));
        return MRT_ERR;
    }

    switch(cmd->str[0])
    {
    case 'i':
    case 'u':
    case 'd':
        return mysql_affected_rows(srv);
    default:
        if((mysql_res = mysql_store_result(srv)))
        {
            num = mysql_num_rows(mysql_res);
            mysql_free_result(mysql_res);
            return num;
        }
        return 0;
    }
}


int mydb_query_uint32(mydb_t *mm, string_t *cmd, uint32_t *num)
{
    M_cpvril(mm);
    M_cpvril(cmd);
    M_cpvril(num);

    MYSQL *srv = &(mm->srv);
    MYSQL_ROW row;
    MYSQL_RES *mysql_res = NULL;


    M_ciril(mydb_connect(mm), "connect to mysql error.");

    if(mysql_query(srv, cmd->str) != MRT_OK)
    {
        set_error("mysql error:%s", mysql_error(srv));
        return MRT_ERR;
    }


    if(!(mysql_res = mysql_store_result(srv)))
    {
        set_error("mysql error:%s.", mysql_error(srv));
        return MRT_ERR;
    }

    if(!(row = mysql_fetch_row(mysql_res)))
    {
        set_error("mysql no found record.");
        mysql_free_result(mysql_res);
        return MRT_ERR;
    }

    *num = (uint32_t)atoi(row[0]);
    mysql_free_result(mysql_res);

    if(cmd->str[0] == 'c')
    {
        if(mysql_next_result(srv) == MRT_ERR)
        {
            set_error("mysql_next_result error:%s.", mysql_error(srv));
            return MRT_SUC;
        }
    }

    return MRT_SUC;
}

int mydb_query_exist(mydb_t *mm, string_t *cmd)
{
    M_cpvril(mm);
    M_cpvril(cmd);

    MYSQL *srv = &(mm->srv);
    MYSQL_ROW row;
    MYSQL_RES *mysql_res = NULL;


    M_ciril(mydb_connect(mm), "connect to mysql error.");

    if(mysql_query(srv, cmd->str) != MRT_OK)
    {
        set_error("mysql error:%s", mysql_error(srv));
        return MRT_ERR;
    }


    if(!(mysql_res = mysql_store_result(srv)))
    {
        set_error("mysql error:%s.", mysql_error(srv));
        return MRT_ERR;
    }

    if(!(row = mysql_fetch_row(mysql_res)))
    {
        set_error("mysql no found record.");
        mysql_free_result(mysql_res);
        return 0;
    }

    mysql_free_result(mysql_res);

    if(cmd->str[0] == 'c')
    {
        if(mysql_next_result(srv) == MRT_ERR)
        {
            set_error("mysql_next_result error:%s.", mysql_error(srv));
            return 1;
        }
    }

    return 1;
}



int mydb_query_int(mydb_t *mm, string_t *cmd, int *num)
{
    M_cpvril(mm);
    M_cpvril(cmd);
    M_cpvril(num);

    MYSQL *srv = &(mm->srv);
    MYSQL_ROW row;
    MYSQL_RES *mysql_res = NULL;


    M_ciril(mydb_connect(mm), "connect to mysql error.");

    if(mysql_query(srv, cmd->str) != MRT_OK)
    {
        set_error("mysql error:%s", mysql_error(srv));
        return MRT_ERR;
    }


    if(!(mysql_res = mysql_store_result(srv)))
    {
        set_error("mysql error:%s.", mysql_error(srv));
        return MRT_ERR;
    }

    if(!(row = mysql_fetch_row(mysql_res)))
    {
        set_error("mysql no found record.");
        mysql_free_result(mysql_res);
        return MRT_ERR;
    }

    *num = atoi(row[0]);
    mysql_free_result(mysql_res);

    if(cmd->str[0] == 'c')
    {
        if(mysql_next_result(srv) == MRT_ERR)
        {
            set_error("mysql_next_result error:%s.", mysql_error(srv));
            return MRT_SUC;
        }
    }

    return MRT_SUC;
}



int mydb_query_long(mydb_t *mm, string_t *cmd, long *num)
{
    M_cpvril(mm);
    M_cpvril(cmd);
    M_cpvril(num);

    MYSQL *srv = &(mm->srv);
    MYSQL_ROW row;
    MYSQL_RES *mysql_res = NULL;


    M_ciril(mydb_connect(mm), "connect to mysql error.");

    if(mysql_query(srv, cmd->str) != MRT_OK)
    {
        set_error("mysql error:%s", mysql_error(srv));
        return MRT_ERR;
    }

    if(!(mysql_res = mysql_store_result(srv)))
    {
        set_error("mysql error:%s", mysql_error(srv));
        return MRT_ERR;
    }

    if(!(row = mysql_fetch_row(mysql_res)))
    {
        set_error("mysql no found record");
        mysql_free_result(mysql_res);
        return MRT_ERR;
    }

    *num = atoll(row[0]);

    mysql_free_result(mysql_res);

    return MRT_SUC;
}



int mydb_query(mydb_t *mm, string_t *cmd)
{
    M_cpvril(mm);

    MYSQL *srv = &(mm->srv);

    M_ciril(mydb_connect(mm), "connect to mysql error.");

    if(mysql_query(srv, cmd->str) != MRT_OK)
    {
        set_error("mysql error:%s, errno:%d", mysql_error(srv), mysql_errno(srv));
        return MRT_ERR;
    }

    //log_info("mysql query:%s -----------", cmd->str);
    return MRT_SUC;
}



char *format_mysql_string(char *src)
{
    char *ps = src;
    while(*ps)
    {
        *ps = *ps==39 ? 34 : *ps;
        ps++;
    }
    return src;
}

