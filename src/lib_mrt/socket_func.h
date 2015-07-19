#ifndef __SOCKET_FUNC_H__
#define __SOCKET_FUNC_H__

#include <sys/types.h>
#include <sys/socket.h>

#define CLOSE(x) while((close(x) == -1) && (errno == EINTR))
#define SOCKET_BLOCK(s) fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0)& ~O_NONBLOCK)
#define SOCKET_NONBLOCK(s) fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0)|O_NONBLOCK)

typedef struct
{
    unsigned int    ip;
    unsigned short  port;
    char            *user;
    char            *pass;
    unsigned int    bind:1;
    unsigned int    type:2;

}proxy_t;

typedef struct
{
    char        ip[33];
    int         port;
}ip4_addr_t;

int socket_connect_wait(char *addr, unsigned short port, int timeout);

int socket_listen(unsigned bindaddr, unsigned short * s_port);

int socket_accept(int sock);

void socket_close(int  sock);

int socket_read_wait(int  sock, void *buf, size_t len, int timeout);

int socket_read(int  sock, void *buf, size_t len);

int socket_write_wait(int sock, void *buf, size_t len, int timeout);

int socket_write(int sock, void *buf, size_t len);

int get_ip_addr(char* hostname, unsigned int * ip);

int get_local_ip(int sockfd, char * local_ip);

int socket_wait_write(int s, int timeout);

int socket_wait_read(int s, int timeout);

int socket_timeout_connect(int sock, struct sockaddr *remote_addr, size_t size, int timeout);

int  proxy_init(proxy_t *ps);

int  proxy_listen(proxy_t *ps, unsigned int * ip, unsigned short * port);

int  proxy_accept(int  server);

int  proxy_connect(proxy_t *ps, unsigned int ip, unsigned short port, const char *hostname);

int socket_accept_block(int lsfd, int blck);

int socket_bind(char *host, int port);

int socket_read_string(int fd, string_t *str);

int socket_write_string(int fd, string_t *str);

ssize_t socket_write_loop( int fd, const void *vptr, size_t n );

int socket_request(int fd, int timeout, string_t *str);

int socket_ntoa(struct sockaddr_in addr, char *abuf, int asize);

//非阻塞的连接
int socket_connect_nonblock(char *addr, unsigned short port);

//非阻塞的绑定, 如果绑定成功返回socket的fd，出错返回-1
int socket_bind_nonblock(char *host, int port);


#endif
