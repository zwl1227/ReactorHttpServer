#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Log.h"
// 通过Tcpconnection将fd放入evloop的监听事件中
// 主线程通过Tcpconnetion为子线程分配任务，唤醒子线程开始工作
class TcpConnection
{
public:
    TcpConnection(int fd, EventLoop *evloop);
    ~TcpConnection();
    inline string getName()
    {
        return m_name;
    }

public:
    static int tcpConnectionDestroy(void *arg);
    static int processRead(void *arg);
    static int processWrite(void *arg);

private:
    EventLoop *m_evLoop;
    Channel *m_channel;
    Buffer *m_writeBuffer;
    Buffer *m_readBuffer;
    string m_name;
    HttpRequest *m_request;
    HttpResponse *m_response;
};
