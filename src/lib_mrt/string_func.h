#ifndef __STRING_FUNC_H__
#define __STRING_FUNC_H__


typedef struct string_t
{
    //stat标志当前字符串是否可用
    uint8_t         stat:1;
    //buf为有效字符串
    char            *str;
    //idx指向字符串中任意位置的指针
    char            *idx;
    //len是当前所用的长度
    int32_t         len;
    //size为分配的内存大小
    int32_t         size;

}string_t;


// ----------- 函数声明 ----------------

//生成string_t对象，字符个数为len
string_t *string_create(int32_t len);

//生成string_t对象，字符个数为len, 并将src复制到字符串头部
string_t *string_new(int32_t len, const char *fmt, ...);

void string_free(string_t *src);

int str_separate(char **src, char *dest, char *sep);

char *str_newcpy(char *src, size_t len);

int str_tolower(char *src);

int str_part_num(char *src, char *separator);

int str_format(char *str);

char *str_jump_tag(char *src, char *tag);

int file_size(const char *filename);

int comm_gets(char *src, char *sb, char *se, char *dest, int dlen);

int make_md5(unsigned char *src, uint16_t len, char *dest);

int move_gets(char **src, char *sb, char *se, char *dest, int dest_size);

int note_filter(char *src);

int conf_filter(char *fbuf, int32_t len);

int last_gets(char *src, char *start, char *end, char **dest);

int move_cut_gets(char **src, char *start, char *end, char **dest);

int charset_convert(char *f_set, char *t_set, char *f_str, size_t f_len, char *t_str, size_t t_len);

time_t str_to_time(char *src);

FILE *create_FILE(char *fname);

int create_file(char *fname);

void gen_filename(char *buf);

//生成随机月份的文件名
void gen_old_path(char *buf, char *prefix, char *suffix);

//在src中查找begin与end之间的内容，如果成功将结果放到dest中
int string_fetch(string_t *src, char *begin, char *end, string_t *dest);

//在src的idx指向的内容中，查找begin与end之间的内容，
//如果成功将结果放到dest中,之后移动src中的idx指向end后
//int string_move_fetch(string_t *src, string_t *begin, string_t *end, string_t *dest);
int string_move_fetch(string_t *src, char *begin, char *end, string_t *dest);

int string_rtrim(string_t *src);

int string_ltrim(string_t *src);

int string_printf(string_t *str, const char *fmt, ...);

void string_add(string_t *dat, const char *fmt, ...);

void string_zero(string_t *src);

int string_cat(string_t *dat, string_t *src);

int string_catb(string_t *dat, char *src, int len);

int string_cats(string_t *dat, char *src);

int string_copy(string_t *dat, string_t *src);

int string_copys(string_t *dat, char *src);

int string_copyb(string_t *dat, char *src, int len);

//功能：
//      将字符串中的from替换为to        替换所有匹配的from
int string_replace(string_t *src, char *from, char *to);


//功能：
//      将字符串中from_begin开始到from_end结束部分替换为to      替换所有匹配的from
int string_replace_part(string_t *src, char *from_begin, char *from_end, char *to);

//功能：
//      将src中字符串进行urlencode编码后存入dest中
//参数：
//      src:原字符串
//      slen:原字符串长度
//      dest:目标字符串
//      dlen:目标字符串长度
//返回：
//      0:成功
//      -1:目标字符串长度不够
int urlencode(char *src, int slen, char *dest, int dlen);

//功能：
//      将str中字符串进行urldecode，结果存回str中
int urldecode(char *str, int len);


int charset_convert(char *f_set, char *t_set, char *f_str, size_t f_len, char *t_str, size_t t_len);

//int charset_convert_string(char *from_set, char *to_set, string_t *from_str, string_t *to_str);
int charset_convert_string(char *from_set, char *to_set, string_t *from_str);

int string_realloc(string_t *src, int32_t size);

int string_base64_encode(string_t *src, string_t *dest);

int string_base64_decode(string_t *src, string_t *dest);

int aes_decode(string_t *input, char *src_key, string_t *output);

int aes_encode(string_t *input, char *src_key, string_t *output);

#endif
