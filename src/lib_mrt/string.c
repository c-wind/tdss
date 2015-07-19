#include "global.h"
#include <openssl/md5.h>
#include <openssl/aes.h>
#include <stdarg.h>

inline int str_format(char *str);


string_t *string_create(int32_t size)
{
    string_t *sret = NULL;

    M_cvrvl((sret = M_alloc(sizeof(string_t))), "alloc string error.");
    M_cvrvl((sret->idx = sret->str = M_alloc(size)), "alloc string buffer error.");
    sret->size = size;

    return sret;
}

int string_realloc(string_t *src, int32_t size)
{
    if(!(src->str = M_realloc(src->str, size)))
        return MRT_ERR;

    src->idx = src->str;
//    printf("string_realloc from size:%u to size:%u\n", src->size, size);
    src->size = size;

    return MRT_SUC;
}

string_t *string_new(int32_t size, const char *fmt, ...)
{
    string_t *sret = NULL;
    va_list ap;

    M_cvrvl((sret = string_create(size)), "create string error.");

    va_start (ap, fmt);
    (void) vsnprintf (sret->str, sret->size, fmt, ap);
    va_end (ap);

    sret->len = strlen(sret->str);

    return sret;
}

int string_copyb(string_t *dat, char *src, int slen)
{
    M_cpvril(dat);
    M_cpsril(src);

    if((dat->size - 1) < slen)
        M_ciril(string_realloc(dat, slen + 1), "string realloc error.");

    memcpy(dat->str, src, slen);
    dat->len = slen;
    dat->str[dat->len] = 0;

    return MRT_SUC;
}

int string_copys(string_t *dat, char *src)
{
    M_cpvril(dat);
    M_cpsril(src);

    int slen = strlen(src);

    if((dat->size - 1) < slen)
        M_ciril(string_realloc(dat, slen + 1), "string realloc error.");

    memcpy(dat->str, src, slen);
    dat->len = slen;
    dat->str[dat->len] = 0;

    return MRT_SUC;
}

int string_copy(string_t *dat, string_t *src)
{
    M_cpvril(dat);
    M_cpvril(src);
    return string_copyb(dat, src->str, src->len);
}


int string_catb(string_t *dat, char *src, int slen)
{
    M_cpvril(dat);
    M_cpvril(src);

    if(slen > (dat->size - dat->len - 1))
    {
        M_ciril(string_realloc(dat, dat->size + slen + 1), "string realloc error.");
    }

    memcpy(dat->str + dat->len, src, slen);
    dat->len += slen;
    dat->str[dat->len] = 0;

    return MRT_SUC;
}


int string_cat_int(string_t *dat, int num)
{
    M_cpvril(dat);

    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "%d", num);
    int slen = strlen(buf);

    if((dat->size - dat->len - 1) < slen)
        M_ciril(string_realloc(dat, dat->size + slen + 1), "string realloc error.");

    memcpy(dat->str + dat->len, buf, slen);
    dat->len += slen;
    dat->str[dat->len] = 0;

    return MRT_SUC;
}


int string_cats(string_t *dat, char *src)
{
    M_cpvril(dat);
    M_cpsril(src);
    int slen = strlen(src);

    if((dat->size - dat->len - 1) < slen)
    {
        M_cvril((dat->str = M_realloc(dat->str, dat->size + slen + 1)), "realloc string buffer error.");
        dat->size += slen + 1;
    }

    memcpy(dat->str + dat->len, src, slen);
    dat->len += slen;
    dat->str[dat->len] = 0;

    return MRT_SUC;
}

int string_cat(string_t *dat, string_t *src)
{
    M_cpvril(dat);
    M_cpvril(src);

    return (string_catb(dat, src->str, src->len));
}


void string_free(string_t *src)
{
    if(!src)
        return;

    if(src->str && src->size)
    {
        M_free(src->str);
        src->size = 0;
        src->str = 0;
        src->len = 0;
    }
}

void string_zero(string_t *src)
{
    memset(src->str, 0, src->size);
    src->len = 0;
}



int string_ltrim(string_t *src)
{
    uint16_t lp = 0;

    M_cpvril(src);

    while(src->str[lp] == 9||
          src->str[lp] == 10||
          src->str[lp] == 13||
          src->str[lp] == 32)
        lp++;

    if(lp > 0)
    {
        src->len -= lp;
        memmove(src->str, src->str + lp , src->len);
        src->str[src->len - 1] = 0;
    }

    return MRT_OK;
}


int string_rtrim(string_t *src)
{
    M_cpvril(src);

    while(src->len > 1)
    {
        switch(src->str[src->len - 1])
        {
        case 9:
        case 10:
        case 13:
        case 32:
            src->str[src->len - 1] = 0;
            src->len--;
            break;
        default:
            return MRT_OK;
            break;
        }
    }

    return MRT_ERR;
}

int string_replace(string_t *src, char *from, char *to)
{
    char *pstr, *pb = src->str;
    string_t dest;

    s_zero(dest);

    while((pstr = strstr(pb, from)))
    {
        if(string_copyb(&dest, pb, pstr - pb) == MRT_ERR)
        {
            log_error("string_copyb pb error:%s", get_error());
            string_free(&dest);
            return MRT_ERR;
        }

        if(string_cats(&dest, to) == MRT_ERR)
        {
            log_error("string_cats to error:%s", get_error());
            string_free(&dest);
            return MRT_ERR;
        }

        if(string_cats(&dest, pstr + strlen(from)) == MRT_ERR)
        {
            log_error("string_cats pstr error:%s", get_error());
            string_free(&dest);
            return MRT_ERR;
        }

        pb =  dest.str;
    }

    if(dest.len)
    {
        if(string_copy(src, &dest) == MRT_ERR)
        {
            log_error("string_copyb dest error:%s", get_error());
            string_free(&dest);
            return MRT_ERR;
        }
    }
    string_free(&dest);

    return MRT_SUC;
}

int string_replace_part(string_t *src, char *from_begin, char *from_end, char *to)
{
    char *pstr, *pb = src->str, *pe = NULL;
    string_t dest;

    s_zero(dest);

    while((pstr = strstr(pb, from_begin)))
    {
        if(!(pe = strstr(pstr+strlen(from_begin), from_end)))
            break;

        if(string_copyb(&dest, pb, pstr - pb) == MRT_ERR)
        {
            log_error("string_copyb pb error:%s", get_error());
            return MRT_ERR;
        }

        if(string_cats(&dest, to) == MRT_ERR)
        {
            log_error("string_cats to error:%s", get_error());
            return MRT_ERR;
        }

        if(string_cats(&dest, pe + strlen(from_end)) == MRT_ERR)
        {
            log_error("string_cats pstr error:%s", get_error());
            return MRT_ERR;
        }

        pb =  dest.str;
    }

    if(dest.len)
    {
        if(string_copy(src, &dest) == MRT_ERR)
        {
            log_error("string_copyb dest error:%s", get_error());
            return MRT_ERR;
        }
    }

    return MRT_SUC;
}


int string_printf(string_t *str, const char *fmt, ...)
{
    va_list ap;
    va_list ap2;
    int32_t len;

    va_start (ap, fmt);
    va_copy(ap2, ap);
    len = vsnprintf(str->str, str->size, fmt, ap);
    va_end (ap);

    if(len >= str->size)
    {
        M_cvril((str->str = M_realloc(str->str, len + 1)), "realloc string buffer error.");
        str->size = len + 1;
        len = vsnprintf (str->str, str->size, fmt, ap2);
    }

    va_end (ap2);
    str->len = len;
    str->idx = str->str;

    return MRT_SUC;
}





int string_fetch(string_t *src, char *begin, char *end, string_t *dest)
{
    char *psb = NULL;
    char *pse = NULL;
    int  blen = 0;

    M_cpvril(src);
    M_cpsril(begin);
    M_cpsril(end);

    blen  = strlen(begin);

    if(!(psb = strstr(src->str, begin)))
    {
        set_error("No found begin tags:[%s].", begin);
        return MRT_ERR;
    }

    if(!(pse = strstr(psb + blen, end)))
    {
        set_error("No found end tags:[%s].", end);
        return MRT_ERR;
    }

    *pse = 0;
    if(string_printf(dest, "%s", psb+blen) == MRT_ERR)
    {
        *pse = end[0];
        set_error("string_printf error.");
        return MRT_ERR;
    }
    *pse = end[0];

    return MRT_SUC;
}




int string_move_fetch(string_t *src, char *begin, char *end, string_t *dest)
{
    char *psb = NULL;
    char *pse = NULL;
    int len = 0, blen = 0;

    M_cpvril(src);
    M_cpsril(begin);
    M_cpsril(end);

    blen = strlen(begin);

    if(!(psb = strstr(src->idx, begin)))
    {
        set_error("No found begin tags:[%s], src:[%s].", begin, src->idx);
        return MRT_ERR;
    }

    if(!(pse = strstr(psb + blen, end)))
    {
        set_error("No found end tags:[%s], src:[%s].", end, psb);
        return MRT_ERR;
    }

    len = pse - psb - blen;

    if(string_copyb(dest, psb + blen, len) == MRT_ERR)
    {
        set_error("string_copyb error, len:%d", len);
        return MRT_ERR;
    }

    src->idx = pse + strlen(end);

    return MRT_OK;
}


int aes_decode(string_t *input, char *src_key, string_t *output)
{
    unsigned char key[AES_BLOCK_SIZE + 1] = {0};
    unsigned char iv[AES_BLOCK_SIZE + 1] = {0};
    int nlen = 0, klen = strlen(src_key);
    AES_KEY aes;

    M_cpsril(src_key);

    memset(key, '*', AES_BLOCK_SIZE);
    memcpy(key, src_key, AES_BLOCK_SIZE > klen ? klen : sizeof(key));
    log_debug("key:%s", key);

    if (AES_set_decrypt_key(key, 128, &aes) < 0)
    {
        log_debug("AES_set_encrypt_key error.");
        return MRT_ERR;
    }

    string_t dstr = {0};

    //解码base64
    M_ciril(string_base64_decode(input, &dstr), "base64 decode error");

    if((dstr.len + 1) % AES_BLOCK_SIZE == 0)
        nlen = dstr.len + 1;
    else
        nlen = ((dstr.len + 1) /  AES_BLOCK_SIZE + 1 ) * AES_BLOCK_SIZE;

    if(nlen > output->size)
    {
        if(string_realloc(output, nlen))
        {
            set_error("string_realloc error");
            string_free(&dstr);
            return MRT_ERR;
        }
    }


    AES_cbc_encrypt((unsigned char *)dstr.str, (unsigned char*)output->str, dstr.len, &aes, iv, AES_DECRYPT);

    string_free(&dstr);

    return MRT_OK;
}





int aes_encode(string_t *input, char *src_key, string_t *output)
{
    unsigned char key[AES_BLOCK_SIZE + 1] = {0};
    unsigned char iv[AES_BLOCK_SIZE + 1] = {0};
    int nlen = 0, klen = strlen(src_key);
    AES_KEY aes;

    M_cpsril(src_key);

    memset(key, '*', AES_BLOCK_SIZE);
    memcpy(key, src_key, AES_BLOCK_SIZE > klen ? klen : sizeof(key));
    log_debug("key:%s", key);

    if (AES_set_decrypt_key(key, 128, &aes) < 0)
    {
        log_debug("AES_set_encrypt_key error.");
        return MRT_ERR;
    }

    if((input->len + 1) % AES_BLOCK_SIZE == 0)
    {
        nlen = input->len + 1;
    }
    else
    {
        nlen = ((input->len + 1) /  AES_BLOCK_SIZE + 1 ) * AES_BLOCK_SIZE;
    }

    if (AES_set_encrypt_key(key, 128, &aes) < 0)
    {
        log_debug("AES_set_encrypt_key error.");
        return -1;
    }

    if((input->len + 1) % AES_BLOCK_SIZE == 0)
        nlen = input->len + 1;
    else
        nlen = ((input->len + 1) /  AES_BLOCK_SIZE + 1 ) * AES_BLOCK_SIZE;

    string_t *dstr = string_create(nlen);
    if(!dstr)
    {
        return MRT_ERR;
    }

    AES_cbc_encrypt((unsigned char *)input->str, (unsigned char*)dstr->str, nlen, &aes, iv, AES_ENCRYPT);
    dstr->len = nlen;

    if(string_base64_encode(dstr, output) == MRT_ERR)
    {
        set_error("base64 encode error");
        string_free(dstr);
        return MRT_ERR;
    }

    string_free(dstr);
    M_free(dstr);

    return MRT_OK;
}

#if 0
int main(int argc, char *argv[])
{
    char str[100] = {'*'};
    int i=0;
    string_t *src = string_new(1024, "tian1234%%#");
    string_t dest = {0};
    string_t dest2 = {0};

//    int aes_encode(string_t *input, char *src_key, string_t *output)
   i =  aes_encode(src, "123456", &dest);

    printf("str:%s\n", dest.str);

    i =  aes_decode(&dest, "123456", &dest2);
    printf("i:%d, dest:%s\n", i, dest2.str);




        return 0;
}
#endif

