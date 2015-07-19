#include "global.h"
#include "macro_const.h"
#include "macro_func.h"


int file_size(const char *filename)
{
    struct stat st;

    M_cpsril(filename);

    s_zero(st);

    M_ciril(stat(filename, &st), "stat error, file:%s", filename);

    return (st.st_size);
}


int64_t fd_file_size(int fd)
{
    struct stat st;

    s_zero(st);

    M_ciril(fstat(fd, &st), "fstat error, fd:%d", fd);

    return (int64_t)(st.st_size);
}



FILE *create_FILE(char *fname)
{
    FILE *fp = NULL;
    char *tmp_dir = NULL;
    char *tmp_sep = NULL;

    tmp_dir = fname;

    umask(0);

    while((tmp_sep = strchr(tmp_dir, '/')))
    {
        *tmp_sep = 0;
        if(mkdir(fname, 0777) == MRT_ERR)
        {
            if(errno != EEXIST)
            {
                log_error("%s mkdir error, path:[%s], %m", __func__, fname);
                tmp_sep[0] = '/';
                return NULL;
            }
        }
        tmp_sep[0] = '/';
        tmp_dir = tmp_sep;
        tmp_dir +=1;
    }

    if(!(fp = fopen(fname, "wb")))
    {
        log_error("%s fopen error, file:[%s], error:%m", __func__, fname);
        return NULL;
    }

    return fp;
}


int create_temp_file(char *path, char **nf)
{
    int nfd = -1, i = 0;
    char *fname = NULL;
    struct timeval tv;

    M_cpsril(path);

    fname = M_alloc(strlen(path) + 32);
    if (!fname)
    {
        log_error("malloc fname error:%m");
        return MRT_ERR;
    }

    for(i=0; i<100; i++)
    {
        gettimeofday(&tv, NULL);
        sprintf(fname, "%s/%.4x%.4x.%.2x", path, getpid(), (uint32_t)tv.tv_usec, i);
        if((nfd = create_file(fname)) != -1)
            break;
    }

    if (nfd == MRT_ERR)
    {
        log_error("can't create temp file error:%m");
        M_free(fname);
        return MRT_ERR;
    }

    *nf = fname;

    return nfd;
}

int create_file(char *fname)
{
    int nfd = -1;
    char *tmp_sep = fname;

    umask(022);

    if((nfd = open(fname, O_CREAT|O_TRUNC|O_WRONLY, 0666)) == MRT_ERR)
    {
        if (errno != ENOENT)
        {
            log_error("open %s error:%m", fname);
            return MRT_ERR;
        }
    }
    else
    {
        log_debug("open %s succ", fname);
        return nfd;
    }

    while(tmp_sep && *tmp_sep && (tmp_sep = strchr(tmp_sep, '/')))
    {
        *tmp_sep = 0;
        if(mkdir(fname, 0766) == MRT_ERR)
        {
            if(errno != EEXIST)
            {
                log_error("mkdir error:%m, path:%s", fname);
                *tmp_sep = '/';
                return MRT_ERR;
            }
        }
        *tmp_sep = '/';
        tmp_sep +=1;
    }

    if((nfd = open(fname, O_CREAT|O_TRUNC|O_WRONLY, 0666)) == MRT_ERR)
    {
        log_error("open %s error:%m", fname);
        return MRT_ERR;
    }

    return nfd;
}

int file_move_uniq(char *ofile, char **nfile)
{
    struct stat s;
    struct timeval tv;
    uint64_t k = 0;
    char *nf = NULL, *pend = NULL;

    M_cpsril(ofile);

    if (stat(ofile, &s) == MRT_ERR)
    {
        log_error("stat %s error:%m", ofile);
        return MRT_ERR;
    }

    gettimeofday(&tv, NULL);

    k = ((uint64_t)s.st_ino << 32) + tv.tv_usec;
    nf = M_alloc(strlen(ofile) + 16);
    pend = strrchr(ofile, '/');

    *pend = 0;
    sprintf(nf, "%s/%lx", ofile, k);
    *pend = '/';

    if (link(ofile, nf) == MRT_ERR)
    {
        log_error("link %s to %s error:%m", ofile, nf);
        M_free(nf);
        return MRT_ERR;
    }

    if (unlink(ofile) == MRT_ERR)
    {
        log_error("unlink old file %s error:%m", ofile);
    }
    else
    {
        log_debug("move %s to %s", ofile, nf);
    }

    *nfile = nf;

    return MRT_OK;
}



int mkdir_p(char *file)
{
    char *pstr = NULL, *psep = strchr(file, '/');
    umask(022);
    while(psep && *psep && (pstr = strchr(psep, '/')))
    {
        *pstr = 0;
        if (mkdir(file, 0766) == MRT_ERR)
        {
            if (errno != EEXIST)
            {
                log_error("can't mkdir %s error:%m", file);
                *pstr = '/';
                return MRT_ERR;
            }
        }
        *pstr = '/';
        psep++;
    }

    return MRT_OK;
}



void generate_path(char *buf, size_t size, char *prefix, char *suffix)
{
    struct tm *ntm;
    time_t now;
    int m;
    static uint32_t inc = 0x111111;

    srand(inc++);
    time(&now);
    m = (int) (1.0 * 12 * rand() / (RAND_MAX + 1.0)) + 1;

    ntm = localtime(&now);
    snprintf(buf, size, "%s/%d/%.2d/%.6x%.4x%s",
             prefix, ntm->tm_year + 1900, m, (uint32_t)now, inc, suffix);

}


void generate_filename(char *buf, size_t size)
{
    int num1, num2;

    srand(time(0));

    num1 =  (int) (1.0 * time(0) * rand() / (RAND_MAX + 1.0));
    num2 =  (int) (1.0 * num1 * rand() / (RAND_MAX + 1.0));

    snprintf(buf, size, "%x%x",
             num1, num2);
}




static int __file_lock(file_handle_t *file)
{
    int iret = 0;

    if(file->op_lock)
    {
        switch(file->op_lock)
        {
        case OP_FILE_READ:
            iret = flock(file->fd, LOCK_SH);
            break;
        case OP_FILE_WRITE:
            iret = flock(file->fd, LOCK_EX);
            break;
        }
        file->is_lock = 1;
    }

    return iret;
}

static void __file_unlock(file_handle_t *file)
{
    if(file->is_lock == 1)
    {
        flock(file->fd, LOCK_UN);
    }
}


static int __file_map(file_handle_t *file)
{
    if(file->op_map == 1)
    {
        file->begin = mmap(NULL, file->size, PROT_READ|PROT_WRITE, MAP_SHARED, file->fd, 0);
        if(!file->begin)
            return MRT_ERR;

        file->end = file->begin + file->size;
    }

    return MRT_SUC;
}

static void __file_unmap(file_handle_t *file)
{
    if(file->is_map == 1)
    {
        munmap(file->begin, file->size);
    }
}

static int __file_truncate(file_handle_t *file)
{
    int iret = 0;

    if(file->op_size == 1)
    {
        //新大小是在原来大小基础上加上add_size
        //file->size += file->add_size;
        iret = ftruncate(file->fd, file->size);
        file->op_size = 0;
    }

    return iret;
}

int file_open_create(file_handle_t *file)
{
    int nfd = -1;
    int flag  = O_RDWR|O_EXCL|O_CREAT;

    M_cpvril(file);

    umask(022);

    if((nfd = open(file->from, flag, 0666)) == MRT_ERR)
    {
        log_error("open file:[%s] error:%m", file->from);
        return MRT_ERR;
    }

    file->fd = nfd;

    if(file->op_lock && __file_lock(file))
    {
        log_fatal("lock file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    if(file->op_map && __file_map(file))
    {
        log_fatal("map file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    //打开成功，置为1
    file->stat = 1;

    return MRT_SUC;
}

int file_open_append(file_handle_t *file)
{
    int nfd = -1;
    int flag  = O_RDWR||O_APPEND;

    M_cpvril(file);

    umask(022);

    if((nfd = open(file->from, flag, 0666)) == MRT_ERR)
    {
        log_error("open file:[%s] error:%m", file->from);
        return MRT_ERR;
    }

    file->fd = nfd;
    file->op_append = 1;

    if(file->op_lock && __file_lock(file))
    {
        log_error("lock file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    //锁定后要获取文件大小，有可能其它有锁操作将其修改
    file->size = fd_file_size(nfd);
    if(file->size == MRT_ERR)
    {
        log_error("fd_file_size error:%m, fd:%d", nfd);
        close(nfd);
        return MRT_ERR;
    }


    if(__file_truncate(file))
    {
        log_fatal("truncate file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    if(file->op_map && __file_map(file))
    {
        log_fatal("map file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    //打开成功，置为1
    file->stat = 1;

    return MRT_SUC;
}

int file_open_read(file_handle_t *file)
{
    int nfd = -1;
    int flag  = O_RDONLY;

    M_cpvril(file);

    if((nfd = open(file->from, flag, 0666)) == MRT_ERR)
    {
        log_error("open file:[%s] error:%m", file->from);
        return MRT_ERR;
    }

    file->fd = nfd;

    if(file->op_lock && __file_lock(file))
    {
        log_error("lock file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    //锁定后要获取文件大小，有可能其它有锁操作将其修改
    file->size = fd_file_size(nfd);
    if(file->size == MRT_ERR)
    {
        log_error("fd_file_size error:%m, fd:%d", nfd);
        close(nfd);
        return MRT_ERR;
    }


    if(__file_truncate(file))
    {
        log_fatal("truncate file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    if(file->op_map && __file_map(file))
    {
        log_fatal("map file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    //打开成功，置为1
    file->stat = 1;

    return MRT_SUC;
}

int file_open(file_handle_t *file)
{
    int nfd = -1;
    int flag  = O_RDWR|O_CREAT;

    M_cpvril(file);

    umask(022);

    if(file->op_append == 1)
    {
        flag |= O_APPEND;
    }

    if((nfd = open(file->from, flag, 0666)) == MRT_ERR)
    {
        log_error("open file:[%s] error:%m", file->from);
        return MRT_ERR;
    }

    file->fd = nfd;

    if(file->op_lock && __file_lock(file))
    {
        log_error("lock file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    //锁定后要获取文件大小，有可能其它有锁操作将其修改
    file->size = fd_file_size(nfd);
    if(file->size == MRT_ERR)
    {
        log_error("fd_file_size error:%m, fd:%d", nfd);
        close(nfd);
        return MRT_ERR;
    }


    if(__file_truncate(file))
    {
        log_fatal("truncate file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    if(file->op_map && __file_map(file))
    {
        log_fatal("map file:(%s) error:%m.", file->from);
        close(nfd);
        return MRT_ERR;
    }

    //打开成功，置为1
    file->stat = 1;

    return MRT_SUC;
}


int file_open_temp(char *path, file_handle_t *file)
{
    struct timeval tv;
    int i;

    M_cpvril(file);

    for(i=0; i<100; i++)
    {
        gettimeofday(&tv, NULL);
        snprintf(file->from, sizeof(file->from), "%s/%.4x.%.4x.%.2x",
                 path, getpid(), (uint32_t)tv.tv_usec, i);

        if(file_open_create(file) == MRT_SUC)
            break;
    }

    if(i == 100)
    {
        log_error("can't create tmp file, error:%m");
        return MRT_ERR;
    }

    if(file_buffer_init(file, M_8KB) == MRT_ERR)
    {
        log_error("file buffer init error, size:%d", M_8KB);
        file_close(file);
        return MRT_ERR;
    }

    log_debug("open temp file:%s success", file->from);

    return MRT_SUC;
}


int file_handle_init(file_handle_t *file, int fd, int stat, int type, char *from, int buffer_size)
{
    M_cpvril(file);
    M_cpvril(from);

    if(buffer_size > 0 && !file->buffer)
    {
        if(file_buffer_init(file, buffer_size) == MRT_ERR)
        {
            log_error("file buffer init error, size:%d", buffer_size);
            return MRT_ERR;
        }
    }

    file->fd = fd;
    file->type = type;
    file->stat = stat;

    if(from && *from)
        snprintf(file->from, sizeof(file->from), "%s", from);
    else
        *file->from = 0;

    return MRT_SUC;
}

file_handle_t *file_handle_create(int fd, int stat, int type, char *from, int buffer_size)
{
    file_handle_t *file = NULL;

    M_cpvrvl(from);

    if(!(file = M_alloc(sizeof(*file))))
    {
        log_error("alloc file error:%m, size:%zu", sizeof(*file));
        return NULL;
    }

    if(buffer_size > 0 && !file->buffer)
    {
        if(file_buffer_init(file, buffer_size) == MRT_ERR)
        {
            log_error("file buffer init error, size:%d", buffer_size);
            return NULL;
        }
    }

    file->fd = fd;
    file->type = type;
    file->stat = stat;
    snprintf(file->from, sizeof(file->from), "%s", from);

    return file;
}


void file_close(file_handle_t *file)
{
    if(file)
    {
        __file_unmap(file);
        __file_truncate(file);
        __file_unlock(file);
        file_buffer_deinit(file);

        if(file->stat == FILE_HANDLE_OPEN)
            close(file->fd);


        file->fd = 0;
        file->stat = FILE_HANDLE_CLOSE;
        log_debug("close file:(%s) success.", file->from);
    }
}


int file_write_loop(int fd, void *vptr, size_t n)
{
    size_t nleft = 0;
    ssize_t nwritten = 0;
    const char *ptr;

    ptr = (char *) vptr;
    nleft = n;

    while( nleft > 0 )
    {
        if((nwritten = write(fd, ptr, nleft) ) <= 0 )
        {
            if( errno == EINTR )
            {
                nwritten = 0;
            }
            else
            {
                return MRT_ERR;
            }
        }
        nleft = nleft - nwritten;
        ptr = ptr + nwritten;
    }

    return MRT_SUC;
}

char *file_to_string(char *file)
{
    int nfd, size;
    char *buf = NULL;

    size = file_size(file);
    if(size == MRT_ERR)
    {
        return NULL;
    }

    if((nfd = open(file, O_RDONLY, 0666)) == MRT_ERR)
    {
        log_error("open %s error:%m", file);
        return NULL;
    }

    buf = M_alloc(size + 1);
    if(!buf)
    {
        close(nfd);
        log_error("malloc size:%d error:%m", size);
        return NULL;
    }

    size = read(nfd, buf, size);
    if(size == MRT_ERR)
    {
        log_error("read %s error:%m", file);
        close(nfd);
        M_free(buf);
        return NULL;
    }

    close(nfd);

    buf[size] = 0;

    return buf;
}



int file_delete(char *fname)
{
    if(unlink(fname) == MRT_ERR)
    {
        log_error("delete file:%s error:%m", fname);
        return MRT_ERR;
    }

    log_info("delete file:%s success.", fname);
    return MRT_SUC;
}






#ifdef FILE_FUNC_TEST
int main(int argc, char *argv[])
{
    int nfd = -1;
    char file[100] = {0};

    int i=0;

    for(; i< 10000; i++)
    {
        gen_old_path(file, "test", ".txt");

        if((nfd = create_file(file)) == MRT_ERR)
        {
            printf("error:%s\n", get_error());
            return -1;
        }

        write(nfd, file, strlen(file));
        close(nfd);
    }

    return 0;
}

#endif

