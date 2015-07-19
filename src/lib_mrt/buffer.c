#include "global.h"
#include <stdarg.h>


int buffer_create(buffer_t **rbuf, int size)
{
    buffer_t *buf = NULL;

    buf = M_alloc(sizeof(*buf) + size);
    if(!buf)
    {
        log_error("create buffer error size:%zd", sizeof(*buf) + size);
        return MRT_ERR;
    }

    buf->type = BUFFER_CREATE;
    buf->data = buf + 1;
    buf->len = 0;
    buf->rpos = buf->data;
    buf->wpos = buf->data;
    buf->str = (char *)buf->data;
    buf->size = size;

    *rbuf = buf;

    return MRT_SUC;
}

int buffer_init(buffer_t *buf, int size)
{
    buf->data = M_alloc(size);
    if(!buf->data)
    {
        log_error("create buffer error size:%d", size);
        return -1;
    }

    buf->type = BUFFER_INIT;
    buf->len = 0;
    buf->rpos = buf->data;
    buf->wpos = buf->data;
    buf->str = (char *)buf->data;
    buf->size = size;

    return MRT_SUC;
}

//如果buffer的剩余空间大于size就直接添加到尾部
//否则把data复制到剩余空间中之后修改size为data中剩余大小
//完成后修改wpos指针,已用缓冲大小
int buffer_push(buffer_t *buf, void *data, int *size)
{
    int len = 0;

    M_cpvril(buf);
    M_cpvril(data);

    if((*size + buf->len) > buf->size)
        len = buf->size - buf->len;
    else
        len = *size;

    memcpy(buf->wpos, data, len);

    buf->wpos += len;
    buf->len += len;
    *size -= len;

    return MRT_SUC;
}


int buffer_clear(buffer_t *buf)
{
    M_cpvril(buf);

    buf->rpos = buf->wpos = buf->data;
    memset(buf->data, 0, buf->size);
    buf->len = 0;

    return MRT_SUC;
}



int buffer_read(int fd, buffer_t *buf)
{
    int rlen = 0;

    M_cpvril(buf);

    while(buf->len < buf->size)
    {

        if((rlen = recv(fd, buf->data + buf->len, buf->size - buf->len, 0)) == MRT_ERR)
        {
            if(errno == EAGAIN)
                break;

            if(errno == EINTR)
                continue;

            log_debug("read info:[%d:%m] recv:%d", errno, buf->len);
            return MRT_ERR;
        }

        if(rlen == 0)
        {
            log_error("fd:%d close.", fd);
            break;
        }
        buf->len += rlen;
    }

    log_debug("read fd:%d, size:%d", fd, buf->len);

    return buf->len;
}


int buffer_write(int fd, buffer_t *buf)
{
    int slen = 0;
    int ssize = 0;

    M_cpvril(buf);

    while(ssize != buf->len)
    {
        if((slen = write(fd, buf->data + ssize, buf->len - ssize)) == MRT_ERR)
        {
            if(errno == EINTR)
                continue;

            if(errno == EAGAIN) //kernel send cache is full
                continue;

            return MRT_ERR;
        }

        ssize += slen;
    }

    log_debug("write fd:%d, size:%d", fd, ssize);

    return MRT_SUC;
}


int buffer_printf(buffer_t *buf, const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vsnprintf (buf->data, buf->size, fmt, ap);
    va_end (ap);
    buf->len = strlen(buf->str);

    return buf->len;
}

int buffer_cats(buffer_t *buf, const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vsnprintf (buf->data, buf->size - buf->len, fmt, ap);
    va_end (ap);
    buf->len = strlen(buf->str);

    return buf->len;
}


int buffer_deinit(buffer_t *buf)
{
    M_cpvril(buf);

    if(buf->type == BUFFER_INIT)
    {
        M_free(buf->data);
        buf->len = 0;
        buf->rpos = NULL;
        buf->wpos = NULL;
        buf->str = NULL;
        buf->size = 0;
    }
    else
    {
        M_free(buf);
    }

    return MRT_SUC;
}

