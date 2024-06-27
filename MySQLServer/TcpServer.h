#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"
typedef struct Listener
{
    int lfd;
    unsigned short port;
} Listener;

typedef struct TcpServer
{
    int threadNum;
    EventLoop *mainloop;
    ThreadPool *threadPool;
    Listener *listener;
} TcpServer;

// 初始化
TcpServer *tcpServerInit(unsigned short port, int threadNum);
// 初始化监听
Listener *listenerInit(unsigned short port);
// 启动Tcp服务器
void tcpServerRun(TcpServer *tcp);