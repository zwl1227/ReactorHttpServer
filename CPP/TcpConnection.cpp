#include "TcpConnection.h"
#include <unistd.h>
#include "Log.h"

TcpConnection::TcpConnection(int fd, EventLoop *evloop) : m_evLoop(evloop)
{
    m_readBuffer = new Buffer(10240);
    m_writeBuffer = new Buffer(10240);
    // http
    m_request = new HttpRequest();
    m_response = new HttpResponse();
    m_name = "Connection-" + to_string(fd);
    m_channel = new Channel(fd, FDEvent::ReadEvent, processRead, processWrite, tcpConnectionDestroy, this);
    evloop->addTask(m_channel, ElemType::ADD);
    // Debug("和客户端建立连接，threadName:%s,threadID:%ld,connName:%s", evloop->getThreadName().data(), evloop->getThreadID(), m_name);
}

TcpConnection::~TcpConnection()
{
    if (m_readBuffer && m_readBuffer->readableSize() == 0 &&
        m_writeBuffer && m_writeBuffer->readableSize() == 0)
    {
        delete m_readBuffer;
        delete m_writeBuffer;
        delete m_request;
        delete m_response;
        m_evLoop->freeChannel(m_channel);
    }
    Debug("链接断开，释放资源，connName:%s", m_name.data());
}

int TcpConnection::tcpConnectionDestroy(void *arg)
{
    TcpConnection *conn = static_cast<TcpConnection *>(arg);
    if (conn != nullptr)
    {
        delete conn;
    }
    return 0;
}

int TcpConnection::processRead(void *arg)
{
    TcpConnection *conn = static_cast<TcpConnection *>(arg);
    // 接受数据,存入readbuffer
    int socket = conn->m_channel->getSocket();
    int count = conn->m_readBuffer->socketRead(socket);
    Debug("接收的HTTP请求数据:%s", conn->m_readBuffer->data() + conn->m_readBuffer->getReadPos());
    if (count > 0)
    {
        // 接收到http请求，解析请求
#ifdef MSG_SEND_AUTO
        // 解析完成后要发送writebuffer，因此需要检测写事件
        conn->m_channel->writeEventEnable(true);
        conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
#endif
        // 1.从readbuffer中解析请求，组织请求:请求行、请求头、空行、请求数据存入request，
        // 2.并根据reqeust，组织响应数据：状态行、响应头、空行、响应数据写入writebuffer
        bool flag = conn->m_request->parseHttpRequest(conn->m_readBuffer, conn->m_response, conn->m_writeBuffer, socket);
        if (!flag)
        {
            // 解析失败
            string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
            conn->m_writeBuffer->appendString(errMsg);
        }
    }
    else
    {
#ifdef MSG_SEND_AUTO
        // 断开连接
        Debug("客户端断开连接");
        conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
    }
#ifndef MSG_SEND_AUTO
    // 断开连接
    Debug("客户端断开连接");
    conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
    return 0;
}

int TcpConnection::processWrite(void *arg)
{
    TcpConnection *conn = static_cast<TcpConnection *>(arg);
    int socket = conn->m_channel->getSocket();
    int count = conn->m_writeBuffer->sendData(socket);
    if (count > 0)
    {
        // 判断数据是否完全发送出去
        if (conn->m_writeBuffer->readableSize() == 0)
        {

            // 不再检测写事件--修改channel中保存的写事件
            conn->m_channel->writeEventEnable(false);
            // 修改dispatcher中检测的集合--添加任务节点
            conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
            // 和客户端断开连接 删除任务节点
            conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
        }
    }
    return 0;
}
