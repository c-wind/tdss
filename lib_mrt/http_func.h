#ifndef __HTTP_FUNC_H__
#define __HTTP_FUNC_H__

typedef struct
{
    string_t            data;
    char                *pbuf;
    char                *pbody;
    int                 blen;
    int                 bsize;
    char                url[MAX_URL];
    char                charset[MAX_URL];
    char                base[MAX_URL];
    char                *start_url;

}mrt_page_t;

typedef struct
{
    string_t            data;

    string_t            charset;
    string_t            base;

//    char                head_description[MAX_URL];         //当前页head中的description
 //   char                head_keywords[MAX_URL];         //当前页head中的description
//    char                *pbuf;
//    char                *pbody;
//    int                 blen;
//    int                 bsize;
    char                url[MAX_URL];
  //  char                charset[MAX_URL];
//    char                base[MAX_URL];
    char                *start_url;

}html_page_t;

typedef struct mrt_mark_s mrt_mark_t;
struct mrt_mark_s
{
    //begin在过滤删除的时候是开始标签，在替换的时候是要被替换的字段
    char                begin[MAX_CAPTION];
    //end在过滤删除的时候是结束标签，在替换的时候是代替上面degin的字段
    char                end[MAX_CAPTION];

    //在替换时，如果新的内容长度大于旧的，这个值就是新的字段的长度
    uint16_t            new_len;

    M_list_entry        (mrt_mark_t)
};

typedef struct
{
    int                 size;

    M_list_head         (mrt_mark_t);

}mrt_mark_list_t;

typedef struct
{
    mrt_mark_list_t     remove_list;
    mrt_mark_list_t     replace_list;

}mrt_filter_t;

void http_free_page(html_page_t *page);

int http_check_prefix(char *src);


int html_fetch_img_ex(char *src, char **dest);

int html_fetch_img(char *src, char *url, int url_size);

int html_fetch_href_ex(char *src, char **url, char **caption);

//int html_fetch_href(char *src, char *url, int url_size, char *caption, int caption_size);
int html_fetch_href(string_t *src, string_t *url, string_t *caption);

int html_part_fetch_href(string_t *src, char *part_start, char *part_end, string_t *url,  string_t *caption);
//int html_part_fetch_href(char *src, char *part_start, char *part_end, char *url, int url_size, char *caption, int caption_size);

int html_move_fetch_href(string_t *src, string_t *url, string_t *caption);

int fix_charset_newcpy(char *src_charset, char **content);


int html_mark_filter(mrt_filter_t *filter, string_t *src);

int html_format(mrt_page_t *page, char *data);

//int fix_caption(mrt_page_t *page, char *caption);

int html_fix_url_ex(mrt_page_t *page, char **url);

int html_fix_url(html_page_t *page, string_t *url);

int html_fetch_base(string_t *src, html_page_t *page);

int html_fetch_charset(string_t *src, html_page_t *page);

int html_head_parse(html_page_t *page);


int html_fix_charset(html_page_t *page, char *charset);

//int html_fix_caption(mrt_page_t *page, char *caption);

int html_fix_caption(html_page_t *page, string_t *caption);

int http_url_parse(char *src, char *host, char *page, char *port);

int http_check_prefix(char *src);

int http_head_find(char *buf, char *key, char *val, uint32_t);

//int http_recv_page(char *url, mrt_page_t **rpage, int parse);

int http_recv_page(char *url, html_page_t *page, int parse);


//接收远程文件，并保存到file中, referer是文件所在页的url
int http_recv_file(char *url, char *referer, string_t *new_path);

int http_get_parameter(char *src, char *key, char *val);

#endif
