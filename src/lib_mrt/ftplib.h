#ifndef __FTPLIB_H__
#define __FTPLIB_H__

#include "socket_active.h"

#define SAVE_STRCMP(a,b) ((!a && !b) || (a && b && !strcmp(a,b)))

typedef struct S_ftp_reply
{
    uint16_t        code;
    char            *reply;
    char            *message;

}T_ftp_reply;

typedef struct S_host_type
{
    unsigned int    ip;
    char            *hostname;
    unsigned short  port;
} T_host_type;

enum stype
{
    ST_UNIX,
    ST_VMS,
    ST_WINNT,
    ST_MACOS,
    ST_OS400,
    ST_OTHER,
    ST_UNDEFINED
};

enum ftype
{
    FT_PLAINFILE,
    FT_DIRECTORY,
    FT_SYMLINK,
    FT_UNKNOWN
};

typedef struct S_file_info T_file_info;
struct S_file_info
{
    enum ftype          type;	/* file type */
    char                *name;	/* file name */
    off_t               size;	/* file size */
    time_t              tstamp;	/* time-stamp */
    int                 perms;
    char                *linkto;
    T_file_info         *prev;	/* ...and next structure. */
    T_file_info         *next;	/* ...and next structure. */
};

typedef struct S_folder_list T_folder_list;
struct S_folder_list
{
    char                *name;
    T_file_info         *list;
    T_folder_list       *next;
};

typedef struct S_ftp_conn
{
    T_host_type          *host;
    char            *user;
    char            *pass;
    int             sock;
    int             data_sock;
    int             serv_sock;

    T_ftp_reply     reply;

    char            *sbuf;
    int             sbuflen;
    T_proxy  *ps;

    T_folder_list   *folder_list;
    char            *current_folder;

    unsigned int    local_ip;
    unsigned int    bindaddr;

    unsigned char   needcwd     :1;
    unsigned char   loggedin    :1;
    unsigned char   portmode    :1;
    char            current_type:2; /* -1 (undefined), 0 (ascii), 1 binary */
    int             secure      :2; /* 1:tls required, 2:tls disabled */
#ifdef HAVE_SSL
    unsigned char   datatls     :1;
#endif

    enum stype      OS;
} T_ftp_conn;

/* konstruktor */
T_host_type  * ftp_new_host(unsigned ip, char * hostname, unsigned short port);
T_ftp_conn * ftp_new(T_host_type * host, int secure, char *user, char *pass);

/* dekonstruktor */
void ftp_free_host(T_host_type * host);
void ftp_quit(T_ftp_conn * self);

/* basic send/recv-api */
int  ftp_get_msg(T_ftp_conn * self);
void ftp_issue_cmd(T_ftp_conn * self, char * cmd, char * value);

/* ftp-functions */
int  ftp_connect(T_ftp_conn * self, T_proxy * ps);
int  ftp_login(T_ftp_conn * self);
#ifdef HAVE_SSL
int  ftp_auth_tls(T_ftp_conn * self);
int  ftp_set_protection_level(T_ftp_conn * self);
#endif
int  ftp_do_syst(T_ftp_conn * self);
int  ftp_do_abor(T_ftp_conn * self);
void ftp_do_quit(T_ftp_conn * self);
int  ftp_do_cwd(T_ftp_conn * self, char * directory);
int  ftp_do_mkd(T_ftp_conn * self, char * directory);
int  ftp_do_chmod(T_ftp_conn * self, char * file, char *chmod);

int  ftp_get_modification_time(T_ftp_conn * self, char * filename, time_t * timestamp);
int  ftp_get_filesize(T_ftp_conn * self, char * filename, off_t * filesize);
int  ftp_get_fileinfo(T_ftp_conn * self, char * filename, T_file_info ** info);
int  ftp_set_type(T_ftp_conn * self, int type);

int  ftp_do_list(T_ftp_conn * self);
int  ftp_get_list(T_ftp_conn * self);
int  ftp_do_rest(T_ftp_conn * self, off_t filesize);
int  ftp_do_stor(T_ftp_conn * self, char * filename/*, off_t filesize*/);
int  ftp_do_dele(T_ftp_conn * self, char * filename);
int  ftp_do_rmd(T_ftp_conn * self, char * dirname);

int  ftp_establish_data_connection(T_ftp_conn * self);
int  ftp_complete_data_connection(T_ftp_conn * self);

int  ftp_do_passive(T_ftp_conn * self);
int  ftp_do_port(T_ftp_conn * self);

T_folder_list   *directory_add_dir(char * current_directory, T_folder_list * A, T_file_info * K);

T_file_info     *fileinfo_find_file(T_file_info * F, char * name);
T_file_info     *ftp_find_directory(T_ftp_conn * self);
void            ftp_fileinfo_free(T_ftp_conn * self);
T_file_info     *ftp_get_current_T_folder_list(T_ftp_conn * self);

void parse_passive_string(char * msg, unsigned int * ip, unsigned short int * port);

T_file_info * ftp_parse_ls (const char *file, const enum stype system_type);

char *read_whole_line(FILE *fp);


#endif
