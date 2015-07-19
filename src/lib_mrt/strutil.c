#include "global.h"
#include <openssl/md5.h>
#include <stdarg.h>

inline int str_format(char *str);



//在str中从右到左查找tag, 将tag之后的字符串复制出来返回
char *str_rcpy(char *src, char tag)
{
    char *nstr = NULL;

    M_cpsrvl(src);

    if(!(nstr = M_alloc(strlen(strrchr(src, tag)))))
    {
        worker_set_error("M_alloc error.");
        return NULL;
    }

    strncpy(nstr, strrchr(src, tag) + 1, strlen(strrchr(src, tag)) - 1);

    return nstr;
}


//在str中从左到右查找tag, 将tag之前的字符串复制出来返回
char *str_lcpy(char *src, char tag)
{
    char *nstr = NULL;

    M_cpsrvl(src);

    if(!(nstr = M_alloc(strrchr(src, tag) - src + 1)))
    {
        worker_set_error("M_alloc error.");
        return NULL;
    }

    strncpy(nstr, src, strrchr(src, tag) - src);

    return nstr;
}

//从左到右在src中查找tag，如果找到将tag之后的部分复制到dest中，最长为dlen
int str_lcut(char *src, char *dest, size_t dlen, char *tag)
{
    char *nstr = NULL;

    M_cpsril(src);
    M_cpsril(dest);

    if((nstr = strstr(src, tag)))
    {
        snprintf(dest, dlen, "%s", nstr+strlen(tag));
        return MRT_SUC;
    }

    return MRT_ERR;
}


//从右到左在src中查找tag，如果找到将tag之后的部分复制到dest中，最长为dlen
int str_rcut(char *src, char *dest, size_t dlen, char *tag)
{
    char *nstr = NULL;
    int tlen = strlen(tag);

    M_cpsril(src);
    M_cpsril(dest);
    if((nstr = strrchr(src, tag[tlen - 1])))
    {
        if(((nstr - src) > tlen) && (!strncmp(nstr - tlen, tag, tlen)))
        {
            snprintf(dest, dlen, "%s", nstr + 1);
            return MRT_SUC;
        }
    }

    return MRT_ERR;
}


int make_md5(unsigned char *src, uint16_t len, char *dest)
{
    int i =0;
    unsigned char buf[MAX_MD5] = {0};

    MD5(src, len, buf);

    for(; i< 16; i++)
        sprintf(dest + 2 * i, "%2.2x", buf[i]);

    return MRT_OK;
}





int str_join(char **dest, char *src)
{
    char *nbuf = NULL;

    M_cpsril(*dest);
    M_cpsril(src);

    if(!(nbuf = M_alloc(strlen(*dest) + strlen(src) +1)))
    {
        worker_set_error("M_alloc error.");
        return MRT_ERR;
    }

    sprintf(nbuf, "%s%s", *dest, src);

    M_free(*dest);

    *dest = nbuf;

    return strlen(*dest);
}




int str_ltrim(char *src)
{
    int lp = 0, mlen = 0;

    M_cpsrinl(src);

    while(isspace(src[lp]))
        lp++;

    if(lp)
    {
        mlen = strlen(src) - lp;
        memmove(src, src + lp , mlen);
        src[mlen] = 0;
    }

    return MRT_SUC;
}

int str_rtrim(char *src)
{
    M_cpsrinl(src);

    while(isspace(src[strlen(src) -1]))
    {
        src[strlen(src) -1] = 0;
    }

    return MRT_SUC;
}


int note_filter(char *src)
{
    char *ret = NULL;

    M_cpsril(src);

    if((ret = strstr(src, "//")))
        *ret = 0;

    if((ret = strchr(src, '#')))
        *ret = 0;

    return str_format(src);
}

time_t str_to_time(char *src)
{
    struct tm tm;

    memset(&tm, 0, sizeof(struct tm));

    sscanf(src, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);

    tm.tm_year -= 1900;
    tm.tm_mon -= 1;

    return mktime(&tm);
}


int str_format(char *str)
{
    M_cpsrinl(str);

    M_cirinl(str_ltrim(str));

    M_cirinl(str_rtrim(str));

    return MRT_OK;
}









int str_tolower(char *src)
{
    char *pstr = src;

    M_cpsril(src);

    while(*pstr)
    {
        if(isupper(*pstr))
        {
            *pstr = tolower(*pstr);
        }
        pstr++;
    }

   return MRT_SUC;
}



char *str_newcpy(char *src, size_t len)
{
    char *ret = NULL;

    M_cpsrvl(src);

    if(!(ret = M_alloc(len + 1)))
    {
        worker_set_error("M_alloc error.");
        return NULL;
    }

    strncpy(ret, src, len);

    return ret;
}


//在字符串src中查找separator出现的次数
int str_part_num(char *src, char *separator)
{
    int num = 0;
    char *ps = src;

    while((ps = strstr(ps, separator)))
    {
       ps += strlen(separator);
       num++;
    }

    return num;
}


int str_separate(char **src, char *dest, char *sep)
{
    char *ps = NULL;

    M_cpsril(*src);

    if(!(ps = strstr(*src, sep)))
    {
        strcpy(dest, *src);
        *src = NULL;
        return MRT_SUC;
    }

    *ps = 0;
    strcpy(dest, *src);
    *ps = *sep;

    *src = ps + strlen(sep);

    return MRT_SUC;
}



int str_find(char *src, char *key, char *val, char *sep1, char *sep2)
{
    char *ps = NULL;

    char pk[SMALL_STR] = {0};

    M_cpsril(src);
    M_cpsril(key);

    ps = strstr(src, key);
    while(ps)
    {
        M_cirinl(str_separate(&ps, pk, sep1))

            if(strcmp(pk, key) == 0)
            {
                return str_separate(&ps, val, sep2);
            }

        ps = strstr(ps, key);
    }


    return MRT_ERR;
}



int last_gets(char *src, char *start, char *end, char **dest)
{
    char *psb = NULL;
    char *pse = NULL;

    M_cpsril(src);
    M_cpsril(start);
    M_cpsril(end);

    if((psb = strstr(src, start)) == NULL)
    {
        worker_set_error("No found begin tags:[%s], src:[%s].", start, src);
        return MRT_ERR;
    }

    if((pse = strstr(psb + strlen(start), end)) == NULL)
    {
        worker_set_error("No found end tags:[%s], src:[%s].", end, psb);
        return MRT_ERR;
    }
    *pse = 0;
    *dest = psb;

    return MRT_SUC;
}




int comm_gets(char *src, char *sb, char *se, char *dest, int dlen)
{
    char *psb = NULL;
    char *pse = NULL;
    int len = 0;
    int blen = 0;

    M_cpsril(src);
    M_cpsril(sb);
    M_cpsril(se);

    blen = strlen(sb);


    if(!(psb = strstr(src, sb)))
    {
        worker_set_error("No found begin tags:[%s], src:[%s].", sb, src);
        return MRT_ERR;
    }

    if(!(pse = strstr(psb + blen, se)))
    {
        worker_set_error("No found end tags:[%s], src:[%s].",se, psb);
        return MRT_ERR;
    }

    len = pse - psb - blen > 0 ? pse - psb - blen: 0;

    if(len > 0)
    {
        if(len > dlen)
        {
            worker_set_error("Dest buffer is smaller than new string, dest len:%d, new:%d.",
                      dlen, len);
            return MRT_ERR;
        }
        strncpy(dest, psb + blen, len);
        return MRT_SUC;
    }


    return MRT_ERR;
}


int move_cut_gets(char **src, char *start, char *end, char **dest)
{
    char *psb = NULL;
    char *pse = NULL;
    int blen = 0;
    int elen = 0;

    M_cpsril(src);
    M_cpsril(start);
    M_cpsril(end);


    blen = strlen(start);
    elen = strlen(end);

    if((psb = strstr(*src, start)) == NULL)
    {
        worker_set_error("No found begin tags:[%s], src:[%s].", start, *src);
        return MRT_ERR;
    }

    if((pse = strstr(psb + blen, end)) == NULL)
    {
        worker_set_error("No found end tags:[%s], src:[%s].", end, psb);
        return MRT_ERR;
    }

    if((pse - psb - blen) < 1)
        return MRT_ERR;

    *pse = 0;
    *dest = psb + blen;
    *src = pse + elen;

    return MRT_OK;
}




int move_gets(char **src, char *sb, char *se, char *dest, int dest_size)
{
    char *psb = NULL;
    char *pse = NULL;
    int len = 0;
    int blen = 0;
    int elen = 0;

    M_cpsril(src);
    M_cpsril(sb);
    M_cpsril(se);


    blen = strlen(sb);
    elen = strlen(se);

    if(!(psb = strstr(*src, sb)))
    {
        worker_set_error("No found begin tags:[%s], src:[%s].", sb, *src);
        return MRT_ERR;
    }

    if(!(pse = strstr(psb + blen, se)))
    {
        worker_set_error("No found end tags:[%s], src:[%s].", se, psb);
        return MRT_ERR;
    }

    len = pse - psb - blen;

    if(len < 1 || len > dest_size)
    {
        worker_set_error("new string length error, len:%d, dest size:%d",
                  len, dest_size);
        return MRT_ERR;
    }

    strncpy(dest, psb + blen, len);

    *src = pse + elen;

    return MRT_OK;
}





//在src中查找sb，如果找到再sb之后查找se，如果找到就返回指向se之后的指针
char *str_jump_part(char *src, char *sb, char *se)
{
    char *psb = NULL;
    char *pse = NULL;

    M_cpsrvl(src);
    M_cpsrvl(sb);
    M_cpsrvl(se);


    M_csrvnl((psb = strstr(src, sb)));
    M_csrvnl((pse = strstr(psb + strlen(sb), se)));

    return (pse + strlen(se));
}



//在src中查找tag，如果找到就返回指向tag之后的指针
char *str_jump_tag(char *src, char *tag)
{
    char *psb = NULL;

    M_cpsrvl(src);
    M_cpsrvl(tag);


    if((psb = strstr(src, tag)) == NULL)
    {
        worker_set_error("No found tags:[%s].", tag);
        return NULL;
    }

    return (psb + strlen(tag));
}





//从以\n分隔的行中查找=，左边的为key右边为val，如果key与tag相同就把val赋值给dest
int get_field(char *src, char *tag, char *dest, size_t dlen)
{
    char *fp = src, *fn = NULL;
    char key[SHORT_STR];
    char val[SHORT_STR];

    M_cpsril(src);
    M_cpsril(tag);

    while((fn = strchr(fp, '\n')))
    {
        memset(key, 0, sizeof(key));
        memset(val, 0, sizeof(val));

        if(sscanf(fp, "%[^=]=%[^\n]", key, val) < 2)
        {
            fp = strstr(fp, "\n") + 1;
            continue;
        }

        if(str_format(key)||str_format(val))
            continue;

        if(!strcasecmp(tag, key))
        {
            snprintf(dest, dlen, "%s", val);
            return MRT_OK;
        }

        fp = fn + 1;
    }

    return MRT_ERR;
}




int buffer_loop_read_line(char **src, char *rbuf)
{
    char *pn = NULL;

    M_cpsril(*src);

    if((pn = strchr(*src, '\r')))
    {
        *pn = 0;
        strcpy(rbuf, *src);
        if(rbuf[strlen(rbuf) - 1] == '\r')
            rbuf[strlen(rbuf) - 1] = 0;
        *pn = '\r';
        *src = pn + 1;
    }
    else
    {
        strcpy(rbuf, *src);
        if(rbuf[strlen(rbuf) - 1] == '\r')
            rbuf[strlen(rbuf) - 1] = 0;
        *src = NULL;
    }

    return 0;
}


int urlencode(char *src, int slen, char *dest, int dlen)
{
    char *from, *to;
    char hexchars[] = "0123456789abcdef";
    char c;

    if(dlen <= slen)
        return -1;

    from = src;
    to = dest;

    while ((from - src < slen) && (to - dest < dlen))
    {
        c = *from++;

        if (c == ' ')
        {
            *to++ = '+';
            continue;
        }

        if(!isalnum(c))
        {
            if((to - dest + 4) > dlen)
                return -1;

            to[0] = '%';
            to[1] = hexchars[(c & 0xf0) >> 4];
            to[2] = hexchars[(c & 0x0f)];
            to += 3;
            continue;
        }

        *to++ = c;
    }

    *to = 0;

    return 0;
}

int urldecode(char *str, int len)
{
    char *dest = str;
    char *data = str;
    int value = 0;
    int c;

    while (len--)
    {
        if (*data == '+')
        {
            *dest = ' ';
        }
        else if (*data == '%' && len >= 2 && isxdigit(data[1]) && isxdigit(data[2]))
        {
            c = data[1];
            value = ((c > 'Z') ? c - 'a' + 10 : (c > '9') ? c - 'A' + 10 : c - '0') * 16;
            c = data[2];
            value += (c > 'Z') ? c - 'a' + 10 : (c > '9') ? c - 'A' + 10 : c - '0';
            *dest = (char) value;
            data += 2;
            len -= 2;
        }
        else
        {
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = 0;
    return dest - str;
}



