#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "global.h"


#define int64toa(num, buf, base) sprintf(buf, "%d", num)

#define ipaddr h_addr_list[0]

int get_ip_addr(char* hostname, unsigned int * ip);

char * printip(unsigned char * ip);
char * base64(char * p, size_t len);

/* =================================== *
 * ====== main socket functions ====== *
 * =================================== */

int socket_connect_wait(char *addr, unsigned short port, int timeout)
{
    struct sockaddr_in iaddr;
    int sock = -1;
    struct timeval tm = {timeout,0};

    iaddr.sin_family      = AF_INET;
    iaddr.sin_addr.s_addr = inet_addr(addr);
    iaddr.sin_port        = htons((unsigned short) port);

    if((sock = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        log_error("socket error:%m");
        return MRT_ERR;
    }

    setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(char *)&tm, sizeof(struct timeval));
    //setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));

    if(connect(sock, (const struct sockaddr *)&iaddr, sizeof(iaddr)) == MRT_ERR)
    {
        log_error("socket connect error:%m");
        close(sock);
        return MRT_ERR;
    }

    return sock;
}

int socket_connect_nonblock(char *addr, unsigned short port)
{
    struct sockaddr_in iaddr;
    int sock = -1;

    iaddr.sin_family      = AF_INET;
    iaddr.sin_addr.s_addr = inet_addr(addr);
    iaddr.sin_port        = htons((unsigned short) port);

    if((sock = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        log_error("socket error:%m");
        return MRT_ERR;
    }

    if(SOCKET_NONBLOCK(sock) == MRT_ERR)
    {
        log_error("SOCKET_NONBLOCK error:%m");
        close(sock);
        return MRT_ERR;
    }

    if(connect(sock, (const struct sockaddr *)&iaddr, sizeof(iaddr)) == MRT_ERR)
    {
        if(errno == EINPROGRESS)
            return sock;

        log_error("socket connect:(%s:%d) error:%m", addr, port);
        close(sock);
        return MRT_ERR;
    }

    return sock;
}


int socket_listen(unsigned bindaddr, unsigned short *s_port)
{
    struct sockaddr_in serv_addr;
    int  sock = -1;

    /*
     * Open a TCP socket(an Internet STREAM socket)
     */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0))<0)
        log_error("server: can't open new socket");
    /*
     * Bind out local address so that the client can send to us
     */
    memset((void *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family            = AF_INET;
    serv_addr.sin_port              = htons(*s_port);
    serv_addr.sin_addr.s_addr       = htonl(bindaddr);

    if(bind(sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
    {
        log_error("server: can't bind local address.");
        exit(0);
    }

    if(!*s_port)
    {
        /* #### addrlen should be a 32-bit type, which int is not
           guaranteed to be.  Oh, and don't try to make it a size_t,
           because that can be 64-bit.  */
        socklen_t addrlen = sizeof (struct sockaddr_in);
        if (getsockname (sock, (struct sockaddr *)&serv_addr, &addrlen) < 0)
        {
            if(sock != -1)
            {
                close (sock);
                sock = -1;
            }
            log_error("Failed to open server socket.");
            return -1;
        }
        *s_port = ntohs (serv_addr.sin_port);
    }

    /* TODO USS install a signal handler to clean up if user interrupt the server */
    /* TODO USS sighandler(*s_sock); */
    listen(sock, 1);
    log_debug("Server socket ready to accept client connection.");

    return sock;
}

/* s_sock is the listening server socket and we accept one incoming
 * connection */
int socket_accept(int sock)
{
    socklen_t clilen;
    struct sockaddr_in client_addr;
    int  nfd = -1;

    clilen = sizeof( client_addr);
    nfd = accept(sock, (struct sockaddr *)&client_addr, &clilen);
    if(nfd == MRT_ERR)
    {
        log_error("accepting the incoming connection");
        exit(4);
    }

    log_debug("Server socket accepted new connection fd:%d.", nfd);

    return nfd;
}


/* recv, but take care of read-timeouts and read-interuptions */
/* error-levels: ERR_TIMEOUT, MRT_ERR */
int socket_read_wait(int  sock, void *buf, size_t len, int timeout)
{
    int res;
    struct timeval tm = {timeout,0};

    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tm, sizeof(struct timeval)) == MRT_ERR)
        return MRT_ERR;
    do
    {
        res = recv(sock, buf, len, 0);

    } while( (res == MRT_ERR && errno == EINTR) );

    return res;
}

/* recv, but take care of read-timeouts and read-interuptions */
/* error-levels: ERR_TIMEOUT, MRT_ERR */
int socket_read(int  sock, void *buf, size_t len)
{
    int res;
    /* TODO NRV looks like a possible bug to me. when there is no data pending,
     * TODO NRV but already data received (but not the complete ssl-block),
     * TODO NRV the next receive might fail, but we are in blocking read and
     * TODO NRV won't detect the connection-break-down. */


    do
    {
        res = recv(sock, buf, len, 0);
    } while( (res == MRT_ERR && errno == EINTR) );

    if(res == 0)
        return MRT_ERR;

    return res;
}

/* simple function to send through the socket
 * if ssl is available send through the ssl-module */
int socket_write_wait(int sock, void *buf, size_t len, int timeout)
{
    int res;
    struct timeval tm = {timeout,0};

    if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tm, sizeof(struct timeval)) == MRT_ERR)
        return MRT_ERR;
    do
    {
        res = send(sock, buf, len, 0);

    } while( (res == MRT_ERR && errno == EINTR) );

    return (res == MRT_ERR) ? MRT_ERR : MRT_SUC;
}
/* simple function to send through the socket
 * if ssl is available send through the ssl-module */
int socket_write(int sock, void *buf, size_t len)
{
    return send(sock, buf, len, 0);
}



int socket_accept_block(int lsfd, int block)
{
    struct sockaddr_in sdr;
    struct linger ling = {1, 0};
    socklen_t slen = sizeof(struct sockaddr_in);
    int nfd;

    if((nfd = accept(lsfd, (struct sockaddr*)&sdr, &slen)) == MRT_ERR)
    {
        log_debug("accept error:%m.");
        return MRT_ERR;
    }

    if(SOCKET_NONBLOCK(nfd) == MRT_ERR)
    {
        log_error("socket set blocking error, error:%m");
        close(nfd);
        return MRT_ERR;
    }

    if(setsockopt(nfd, SOL_SOCKET, SO_LINGER, (const char *)&ling, sizeof(ling)) == MRT_ERR)
    {
        log_error("setsockopt error:%m");
        return MRT_ERR;
    }

    return nfd;
}



int socket_bind(char *host, int port)
{
    struct sockaddr_in serveraddr;
    int nfd = 0;
    int nOptVal = 1;
    struct linger optval1 = {1, 0};

    if((nfd = socket(AF_INET, SOCK_STREAM, 0)) == MRT_ERR)
    {
        log_error("%s socket error.%m", __func__);
        return MRT_ERR;
    }

    memset(&serveraddr, 0, sizeof(struct sockaddr));

    serveraddr.sin_family = AF_INET;
    inet_aton(host, &(serveraddr.sin_addr));
    serveraddr.sin_port=htons(port);

    if(setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, &nOptVal, sizeof(int)) == MRT_ERR)
    {
        log_error("%s setsockopt error.%m", __func__);
        return MRT_ERR;
    }

    if(setsockopt(nfd, SOL_SOCKET, SO_LINGER, &optval1, sizeof(struct linger)) == MRT_ERR)
    {
        log_error("%s setsockopt error.%m", __func__);
        return MRT_ERR;
    }

    if (fcntl(nfd, F_SETFL, O_NONBLOCK) == MRT_ERR)
    {
        log_error("%s set nonblock error.", __func__);
        return MRT_ERR;
    }

    if(bind(nfd,(struct sockaddr *)&serveraddr, sizeof(serveraddr)) == MRT_ERR)
    {
        log_error("%s bind error.%m", __func__);
        return MRT_ERR;
    }

    if(listen(nfd, 128) == MRT_ERR)
    {
        log_error("%s listen error.%m", __func__);
        close(nfd);
        return MRT_ERR;
    }

    log_info("bind success fd:%d, ip:%s, port:%d", nfd, host, port);

    return nfd;
}


int socket_bind_nonblock(char *host, int port)
{
    int nfd = socket_bind(host, port);

    if(nfd == MRT_ERR)
    {
        log_error("bind:(%s:%d) error:%m", host, port);
        return MRT_ERR;
    }

    if(SOCKET_NONBLOCK(nfd) == MRT_ERR)
    {
        log_error("SOCKET_NONBLOCK:(%s:%d) error:%m", host, port);
        close(nfd);
        return MRT_ERR;
    }

    return nfd;
}



ssize_t socket_write_loop( int fd, const void *vptr, size_t n )
{
    size_t nleft = 0;
    ssize_t nwritten = 0;
    const char *ptr;

    ptr = (char *) vptr;
    nleft = n;

    while( nleft > 0 )
    {
        if( (nwritten = write(fd, ptr, nleft) ) <= 0 )
        {
            if( errno == EINTR )
            {
                nwritten = 0;
            }
            else
            {
                return -5;
            }
        }
        nleft = nleft - nwritten;
        ptr = ptr + nwritten;
    }
    return(n);
}



int socket_request(int fd, int timeout, string_t *str)
{
    char rbuf[MAX_LINE] = {0};

    if(str->len > 0)
    {
        if(socket_write_wait(fd, str->str, str->size, 30) != str->len)
        {
            log_error("send msg error, msg:%s, fd:%d.", str->str, fd);
            return MRT_ERR;
        }
    }

    if(socket_read_wait(fd, rbuf, MAX_LINE, timeout) < 0)
    {
        log_error("read msg error, fd:%d.", fd);
        return MRT_ERR;
    }

    if(strncmp(rbuf, "+OK", 3) != 0)
    {
        log_error("recv error msg:%s, fd:%d.", rbuf, fd);
        return MRT_ERR;
    }

    return MRT_OK;
}


/* =================================== *
 * ============= utils =============== *
 * =================================== */

int get_ip_addr(char* hostname, unsigned int * ip)
{
    struct hostent *ht;
    ht = gethostbyname(hostname);

    if(ht != 0x0)
    {
        log_debug("IP of `%s` is `%s`", hostname, ((unsigned char *) ht->h_addr_list[0]));
        memcpy(ip, ht->h_addr_list[0], 4);
        return MRT_SUC;
    }

    return MRT_ERR;
}


int socket_ntoa(struct sockaddr_in addr, char *abuf, int asize)
{
    return snprintf(abuf, asize, "%d.%d.%d.%d:%d",
                    addr.sin_addr.s_addr        & 0xFF,
                    (addr.sin_addr.s_addr >> 8) & 0xFF,
                    (addr.sin_addr.s_addr >> 16)& 0xFF,
                    (addr.sin_addr.s_addr >> 24)& 0xFF,
                    ntohs(addr.sin_port));
}

int get_local_ip(int sockfd, char * local_ip)
{
    struct sockaddr_in mysrv;
    struct sockaddr *myaddr;
    socklen_t addrlen = sizeof (mysrv);

    myaddr = (struct sockaddr *) (&mysrv);

    if (getsockname (sockfd, myaddr, &addrlen) < 0)
        return MRT_ERR;

    memcpy (local_ip, &mysrv.sin_addr, 4);

    return MRT_SUC;
}



