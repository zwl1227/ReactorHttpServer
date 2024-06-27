#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Log.h"

typedef struct TcpConnection
{
    EventLoop *evLoop;
    Channel *channel;
    Buffer *writeBuffer;
    Buffer *readBuffer;
    char name[32];
    HttpRequest *request;
    HttpResponse *response;
} TcpConnection;

// 初始化
TcpConnection *tcpConnectionInit(int fd, EventLoop *evloop);
int tcpConnectionDestroy(void *arg);