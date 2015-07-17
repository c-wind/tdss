#ifndef __STRING_FUNC_H__
#define __STRING_FUNC_H__


typedef struct S_charset
{
    char            from[MAX_ID];
    char            to[MAX_ID];
    iconv_t         ct;

}T_charset;


typedef struct T_array
{
    //stat标志当前数组是否可用
    uint8_t         stat:1;
    //data为真实的数组
    void            *data;
    //idx指向数组中任意位置的指针
    void            *idx;
    //len是当前所用的长度
    uint16_t        len;
    //size为分配的内存大小
    uint16_t        size;

}T_array;


// ----------- 函数声明 ----------------

string_t *string_new(char *src, uint32_t len);

int str_separate(char **src, char *dest, char *sep);

char *str_newcpy(char *src);

int str_tolower(char *src);

int str_part_num(char *src, char *separator);

int str_format(char *str);

int jump_part(char **src, char *sb, char *se);

int jump_to(char **src, char *to);

int jump_over(char **src, char *to);

int file_size(const char *filename);

int comm_gets(char *src, char *sb, char *se, char *dest, int dlen);

int make_md5(unsigned char *src, uint16_t len, char *dest);

int move_gets(char **src, char *sb, char *se, char *dest);

int note_filter(char *src);

int conf_filter(char *fbuf, uint32_t len);

int last_gets(char *src, char *start, char *end, char **dest);

int move_cut_gets(char **src, char *start, char *end, char **dest);

int charset_convert(char *f_set, char *t_set, char *f_str, size_t f_len, char *t_str, size_t t_len);

int charset_convert_string(char *from_set, char *to_set, string_t *from_str, string_t *to_str);

time_t str_to_time(char *src);

FILE *create_FILE(char *fname);

int create_file(char *fname);

void gen_filename(char *buf);

//生成随机月份的文件名
void gen_old_path(char *buf, char *prefix, char *suffix);

#endif
