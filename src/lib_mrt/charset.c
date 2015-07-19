#include "global.h"
#include <iconv.h>

extern char *error_msg;

int charset_convert(char *f_set, char *t_set, char *f_str, size_t f_len, char *t_str, size_t t_len)
{
    iconv_t ct;

    if((ct = iconv_open(t_set,f_set)) == (iconv_t)-1)
    {
        log_error("iconv_open error, from:%s, to:%s, error:%m.", f_set, t_set);
        return MRT_ERR;
    }

    if(iconv(ct, &f_str, &f_len, &t_str,&t_len) == MRT_ERR)
    {
        log_error("iconv error, from:%s, to:%s, error:%m.", f_set, t_set);
        return MRT_ERR;
    }

    iconv_close(ct);

    return MRT_OK;
}

static void charset_convert_end();
static char last_charset_from[64] = {0};
static char last_charset_to[64] = {0};
iconv_t ct;

static int charset_convert_begin(char *from_set, char *to_set)
{
    //如果这次要转换的编码与上次转换的一样直接返回
    if(!strcasecmp(from_set, last_charset_from) && !strcasecmp(to_set, last_charset_to))
        return MRT_SUC;

    //不是第一次转换要先关闭
    if(*last_charset_from && *last_charset_to)
        charset_convert_end();

    //打开
    if((ct = iconv_open(to_set, from_set)) == (iconv_t)-1)
    {
        set_error("iconv_open error, from:%s, to:%s, error:%m.", from_set, to_set);
        return MRT_ERR;
    }
    else
    {
        snprintf(last_charset_from, sizeof(last_charset_from) - 1, "%s", from_set);
        snprintf(last_charset_to, sizeof(last_charset_to) - 1, "%s", to_set);
    }

    return MRT_SUC;
}

static void charset_convert_end()
{
    iconv_close(ct);
}



int charset_convert_string(char *from_set, char *to_set, string_t *from_str)
{
    size_t fsize, tsize;
    char *to_str = NULL, *pt, *pf;

    //if(charset_convert_begin(from_set, to_set) == MRT_ERR)
    if(charset_convert_begin(from_set, "UTF-8//IGNORE") == MRT_ERR)
        return MRT_ERR;

    fsize = from_str->len;
    tsize = from_str->size << 1;

    if(!(to_str = M_alloc(tsize)))
        return MRT_ERR;

    pt = to_str;
    pf = from_str->str;

    //log_debug("from:%s, to:%s, fsize:%ld tsize:%ld.", from_set, to_set, fsize, tsize);

    if(iconv(ct, &pf, &fsize, &pt, &tsize) == MRT_ERR)
    {
        set_error("iconv error, from:%s, to:%s, error:%m, fsize:%lu tsize:%lu.", from_set, to_set, fsize, tsize);
        M_free(to_str);
        return MRT_ERR;
    }

    if(string_copys(from_str, to_str) == MRT_ERR)
    {
        log_error("string_copy error:%s", get_error());
        M_free(to_str);
        return MRT_ERR;
    }
    M_free(to_str);

    return MRT_OK;
}
