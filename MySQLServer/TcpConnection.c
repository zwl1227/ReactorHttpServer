#include "TcpConnection.h"

int processRead(void *arg)
{
    TcpConnection *conn = (TcpConnection *)arg;
    // 接受数据,存入readbuffer
    int count = bufferSocketRead(conn->readBuffer, conn->channel->fd);
    Debug("接收的HTTP请求数据:%s", conn->readBuffer->data + conn->readBuffer->readpos);
    if (count > 0)
    {
        // 接收到http请求，解析请求
        int sock = conn->channel->fd;
#ifdef MSG_SEND_AUTO
        // 解析完成后要发送writebuffer，因此需要检测写事件
        writeEventEnable(conn->channel, true);
        eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
#endif
        // 1.从readbuffer中解析请求，组织请求:请求行、请求头、空行、请求数据存入request，
        // 2.并根据reqeust，组织响应数据：状态行、响应头、空行、响应数据写入writebuffer
        bool flag = parseHttpRequest(conn->request, conn->readBuffer, conn->response, conn->writeBuffer, sock);
        if (!flag)
        {
            // 解析失败
            char *errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
            bufferAppendString(conn->writeBuffer, errMsg);
        }
    }
    else
    {
#ifdef MSG_SEND_AUTO
        // 断开连接
        Debug("客户端断开连接");
        eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
    }
#ifndef MSG_SEND_AUTO
    // 断开连接
    eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
}
int processWrite(void *arg)
{

    TcpConnection *conn = (TcpConnection *)arg;
    int count = bufferSendData(conn->writeBuffer, conn->channel->fd);
    if (count > 0)
    {
        // 判断数据是否完全发送出去
        if (bufferReadableSize(conn->writeBuffer) == 0)
        {

            // 不再检测写事件--修改channel中保存的写事件
            writeEventEnable(conn->channel, false);
            // 修改dispatcher中检测的集合--添加任务节点
            eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
            // 和客户端断开连接 删除任务节点
            eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
        }
    }
}
TcpConnection *tcpConnectionInit(int fd, EventLoop *evloop)
{
    TcpConnection *conn = (TcpConnection *)malloc(sizeof(TcpConnection));
    conn->evLoop = evloop;
    conn->readBuffer = bufferInit(10240);
    conn->writeBuffer = bufferInit(10240);
    // http
    conn->request = httpRequestInit();
    conn->response = httpResponseInit();
    sprintf(conn->name, "Connection-%d", fd);
    conn->channel = channelInit(fd, ReadEvent, processRead, processWrite, tcpConnectionDestroy, conn);
    eventLoopAddTask(evloop, conn->channel, ADD);
    Debug("和客户端建立连接，threadName:%s,threadID:%ld,connName:%s", evloop->threadName, evloop->threadID, conn->name);
    return conn;
}

int tcpConnectionDestroy(void *arg)
{
    TcpConnection *conn = (TcpConnection *)arg;
    if (conn != NULL)
    {
        if (conn->readBuffer && bufferReadableSize(conn->readBuffer) == 0 && conn->writeBuffer && bufferReadableSize(conn->writeBuffer) == 0)
        {
            destroyChannel(conn->evLoop, conn->channel);
            bufferDestroy(conn->readBuffer);
            bufferDestroy(conn->writeBuffer);
            httpRequestDestroy(conn->request);
            httpResponseDestroy(conn->response);
            free(conn);
        }
    }
    Debug("链接断开，释放资源，connName:%s", conn->name);
    return 0;
}
