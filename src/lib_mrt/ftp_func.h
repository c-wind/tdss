#ifndef __FTP_H__
#define __FTP_H__

#define SAVE_STRCMP(a,b) ((!a && !b) || (a && b && !strcmp(a,b)))

typedef struct S_ftp_reply
{
    uint16_t        code;
    char            *reply;
    char            *message;

}ftp_reply_t;

typedef struct S_host_type
{
    unsigned int    ip;
    char            *hostname;
    unsigned short  port;
} host_type_t;

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

typedef struct S_file_info file_info_t;
struct S_file_info
{
    enum ftype          type;	/* file type */
    char                *name;	/* file name */
    off_t               size;	/* file size */
    time_t              tstamp;	/* time-stamp */
    int                 perms;
    char                *linkto;
    file_info_t         *prev;	/* ...and next structure. */
    file_info_t         *next;	/* ...and next structure. */
};

typedef struct S_folder_list T_folder_list;
struct S_folder_list
{
    char                *name;
    file_info_t         *list;
    T_folder_list       *next;
};

typedef struct S_ftp_conn
{
    host_type_t          *host;
    char            *user;
    char            *pass;
    int             sock;
    int             data_sock;
    int             serv_sock;

    ftp_reply_t     reply;

    char            *sbuf;
    int             sbuflen;
    proxy_t  *ps;

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
} ftp_conn_t;

typedef struct S_ftp_session ftp_session_t;
struct S_ftp_session
{
    ftp_conn_t       *ftp;
    proxy_t         *proxy;

    int             portmode;
    int             bindaddr;
    int             chmod;

    char            *root;
    char            *local_file;
    char            *target_folder;
    char            *target_file;

    off_t           local_fsize;
    off_t           target_fsize;

    time_t          local_ftime;
    time_t          target_ftime;

    short int       retry;
    int             retry_interval;

    uint8_t         done:1;
    char            binary:2;

    file_info_t     *directory;
    ftp_session_t   *next;

};



/* konstruktor */
host_type_t  * ftp_new_host(unsigned ip, char * hostname, unsigned short port);
ftp_conn_t * ftp_new(host_type_t * host, int secure, char *user, char *pass);

/* dekonstruktor */
void ftp_free_host(host_type_t * host);
void ftp_quit(ftp_conn_t * self);

/* basic send/recv-api */
int  ftp_get_msg(ftp_conn_t * self);
void ftp_issue_cmd(ftp_conn_t * self, char * cmd, char * value);

/* ftp-functions */
int  ftp_connect(ftp_conn_t * self, proxy_t * ps);
int  ftp_login(ftp_conn_t * self);
#ifdef HAVE_SSL
int  ftp_auth_tls(ftp_conn_t * self);
int  ftp_set_protection_level(ftp_conn_t * self);
#endif
int  ftp_do_syst(ftp_conn_t * self);
int  ftp_do_abor(ftp_conn_t * self);
void ftp_do_quit(ftp_conn_t * self);
int  ftp_do_cwd(ftp_conn_t * self, char * directory);
int  ftp_do_mkd(ftp_conn_t * self, char * directory);
int  ftp_do_chmod(ftp_conn_t * self, char * file, char *chmod);

int  ftp_get_modification_time(ftp_conn_t * self, char * filename, time_t * timestamp);
int  ftp_get_filesize(ftp_conn_t * self, char * filename, off_t * filesize);
int  ftp_get_fileinfo(ftp_conn_t * self, char * filename, file_info_t ** info);
int  ftp_set_type(ftp_conn_t * self, int type);

int  ftp_do_list(ftp_conn_t * self);
int  ftp_get_list(ftp_conn_t * self);
int  ftp_do_rest(ftp_conn_t * self, off_t filesize);
int  ftp_do_stor(ftp_conn_t * self, char * filename/*, off_t filesize*/);
int  ftp_do_dele(ftp_conn_t * self, char * filename);
int  ftp_do_rmd(ftp_conn_t * self, char * dirname);

int  ftp_establish_data_connection(ftp_conn_t * self);
int  ftp_complete_data_connection(ftp_conn_t * self);

int  ftp_do_passive(ftp_conn_t * self);
int  ftp_do_port(ftp_conn_t * self);

T_folder_list   *directory_add_dir(char * current_directory, T_folder_list * A, file_info_t * K);

file_info_t     *fileinfo_find_file(file_info_t * F, char * name);
file_info_t     *ftp_find_directory(ftp_conn_t * self);
void            ftp_fileinfo_free(ftp_conn_t * self);

void parse_passive_string(char * msg, unsigned int * ip, unsigned short int * port);

file_info_t * ftp_parse_ls (const char *file, const enum stype system_type);

char *read_whole_line(FILE *fp);




int do_cwd(ftp_session_t * fsession, char * targetdir);
int long_do_cwd(ftp_session_t * fsession);
int try_do_cwd(ftp_conn_t * ftp, char * path, int mkd);

int do_send(ftp_session_t * fsession);

int fsession_process_file(ftp_session_t * fsession);

/* for ftp-ls.c */

void retry_wait(ftp_session_t *fsession);

int parse_url(ftp_session_t * fsession, char *url);

struct wput_timer *wtimer_alloc();

void wtimer_reset (struct wput_timer *wt);

void bar_create(ftp_session_t * fsession);

double wtimer_elapsed (struct wput_timer *wt);

char * calculate_transfer_rate(double time_diff, off_t tbytes, unsigned char sp);

file_info_t * ftp_get_current_folder_list(ftp_conn_t * self);

void bar_update(ftp_session_t * fsession, off_t transfered, int transfered_last, struct wput_timer * last);


int ftp_process_file(ftp_session_t * fs, int opt_delete);

int ftp_session_init(ftp_session_t *fs, char *addr, int port, char *user, char *pass);

int ftp_session_destroy(ftp_session_t *fs);









#endif
