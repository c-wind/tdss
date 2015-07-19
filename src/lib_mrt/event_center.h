#ifndef __EVENT_CENTER_H__
#define __EVENT_CENTER_H__

//event type -------------------------
#include "factory.h"

#define EVENT_NONE 0
#define EVENT_CONN 0
#define EVENT_RECV  1
#define EVENT_SEND  2
#define EVENT_PROC  4
#define EVENT_ERROR 8
#define EVENT_TIMEOUT 16
#define EVENT_OVER 32

#define CONN_WAIT_SEND 1
#define CONN_WAIT_RECV 2

#define EVENT_EINTR -4

//不管函数执行成功失败都会清空ec
/*
                      callback_t on_accept,     //在接收完连接时调用
                      callback_t on_request,    //在接收到用户数据之后调用
                      callback_t on_response,   //在向用户发送完数据之后调用，不管当前发送缓冲区有没有需要发送的数据，都会调用
                      callback_t on_close,      //在关闭连接之前调用
                      */
int event_center_init(int max_conn, int timeout, char *host, int port,
                      callback_t on_accept,
                      callback_t on_request,
                      callback_t on_response,
                      callback_t on_close
                      );

int event_loop();

void worker_return(task_t *tsk);


#endif
