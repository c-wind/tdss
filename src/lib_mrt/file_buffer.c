#include "global.h"

int file_buffer_init(file_handle_t *file, int size)
{
    file->buffer = M_alloc(sizeof(buffer_t));
    if(!file->buffer)
    {
        log_error("file buffer create error, size:%d", size);
        return MRT_ERR;
    }

    return buffer_init(file->buffer, size);
}

void file_buffer_deinit(file_handle_t *file)
{
    if(file)
        if(file->buffer)
        {
            file_buffer_flush(file);
            buffer_deinit(file->buffer);
            //M_free(file->buffer);
            file->buffer = NULL;
        }
}

int file_buffer_flush(file_handle_t *file)
{
    if(file_write_loop(file->fd, file->buffer->data, file->buffer->len))
    {
        log_error("write file error:%m");
        return MRT_ERR;
    }

//    log_debug("write file:%s size:%u", file->from, file->buffer->len);

    buffer_clear(file->buffer);

    return MRT_SUC;
}



int file_buffer_write(file_handle_t *file, void *data, int size)
{
    int dsize = size;

    if(buffer_push(file->buffer, data, &dsize) == MRT_ERR)
    {
        log_error("buffer append error.");
        return MRT_ERR;
    }

    //如果size不为0，说明buffer已满, 需要将buffer写到磁盘上
    while(dsize > 0)
    {
        if(file_buffer_flush(file) == MRT_ERR)
        {
            log_error("file:(%s) flush error.", file->from);
            return MRT_ERR;
        }

        if(buffer_push(file->buffer, (data + (size - dsize)), &dsize) == MRT_ERR)
        {
            log_error("buffer append error.");
            return MRT_ERR;
        }
    }

 //   log_debug("file:(%s) write buffer success, data size:%d", file->from, size);

    return MRT_SUC;
}



