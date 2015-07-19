#ifndef __FILE_FUNC_H__
#define __FILE_FUNC_H__


#define FILE_HANDLE_INIT 0
#define FILE_HANDLE_OPEN 1
#define FILE_HANDLE_CLOSE 2

#define FD_TYPE_SOCKET  2
#define FD_TYPE_FILE    4
#define FD_TYPE_ACTIVE  8
#define FD_TYPE_PASSIVE 16


typedef struct
{
    int         stat;                   //FILE_HANDLE_INIT:未使用，FILE_HANDLE_OPEN:已打开文件, FILE_HANDLE_CLOSE:已关闭
    int         type;                   //FD_TYPE_SOCKET或者FD_TYPE_FILE, 标识当前handle是文件还是网络
    int         events;                 //下一步操作类型，read,write
    int         num;                    //对此fd操作的次数, 强制关闭到达最大操作次数的fd

    int         fd;
    char        from[MAX_PATH];         //文件全路径或对方服务器地址

    //以下只在fd为文件操作时有效
    int64_t     size;                   //文件打开时的大小
    int64_t     add_size;               //文件附加大小

    int         op_append;              //如果要追加内容，置为1
    int         op_map;                 //如果需要挂载到内存，置为1
    int         op_lock;                //不需要加锁，置为0，其它OP_FILE_READ,OP_FILE_WRITE
    int         op_size;                //如果需要修改大小，置为1, 新大小为new_size中指定的

    int         is_map;                 //是否挂到内存中，1为挂，0为只打开没挂载
    int         is_lock;                //是否加锁，1为加锁，0为不加锁

    void        *begin;
    void        *end;

    buffer_t    *buffer;                //文件缓冲区

}file_handle_t;

int file_open(file_handle_t *file);

int file_open_read(file_handle_t *file);

//功能：
//      在指定目录path中，创建一个临时文件，并打开后返回
int file_open_temp(char *path, file_handle_t *file);

//功能:
//      初始化一个file
//参数：
//      stat:状态（FILE_HANDLE_INIT:未使用，FILE_HANDLE_OPEN:已打开文件, FILE_HANDLE_CLOSE:已关闭）
//      type:文件类型(FD_TYPE_SOCKET或者FD_TYPE_FILE) 或 (FD_TYPE_ACTIVE 或者 FD_TYPE_PASSIVE)
//      from:当为文件时，这里配置文件名，当为网络连接时这里是远程IP及端口
//      buffer_size:如果这个值大于0，就初始化这个文件的buffer
//返回：
//      0:成功
//      -1:出错
int file_handle_init(file_handle_t *file, int fd, int stat, int type, char *from, int buffer_size);

void file_close(file_handle_t *file);

int file_size(const char *filename);

int64_t fd_file_size(int fd);

int file_write_loop(int fd, void *vptr, size_t n);

int file_buffer_init(file_handle_t *file, int size);

//将buffer中数据写入文件中
int file_buffer_flush(file_handle_t *file);

//向buffer中写入数据
int file_buffer_write(file_handle_t *file, void *data, int size);

void file_buffer_deinit(file_handle_t *file);

inline int file_delete(char *fname);

char *file_to_string(char *file);

//设置文件打开后并挂载到内存中
#define FMOD_MAP(file) (file)->op_map = 1

//设置文件打开后要追加内容，文件指针会移动到最后
#define FMOD_APPEND(file) (file)->op_append = 1

//设置文件打开后并加
#define FMOD_READ(file) (file)->op_lock = OP_FILE_READ

#define FMOD_WRITE(file) (file)->op_lock = OP_FILE_WRITE

#define FMOD_SIZE(file, size) \
    (file)->add_size = size; \
    (file)->op_size = 1

int file_move_uniq(char *ofile, char **nfile);

int create_temp_file(char *path, char **nf);

#endif
