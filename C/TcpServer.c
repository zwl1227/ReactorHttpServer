#include "TcpServer.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "TcpConnection.h"
TcpServer *tcpServerInit(unsigned short port, int threadNum)
{
    TcpServer *tcpServer = (TcpServer *)malloc(sizeof(TcpServer));
    tcpServer->listener = listenerInit(port);
    tcpServer->mainloop = eventLoopInit();
    tcpServer->threadNum = threadNum;
    tcpServer->threadPool = ThreadPoolInit(tcpServer->mainloop, threadNum);
    return tcpServer;
}

Listener *listenerInit(unsigned short port)
{
    Listener *listener = (Listener *)malloc(sizeof(Listener));
    // 创建监听的fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket");
        return NULL;
    }
    // 设置端口复用
    int opt = 1;
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1)
    {
        perror("setsockopt");
        return NULL;
    }
    // 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("bind");
        return NULL;
    }
    // 监听
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        return NULL;
    }
    // 返回fd
    listener->lfd = lfd;
    listener->port = port;
    return listener;
}
int acceptConnection(void *arg)
{
    Debug("建立新链接");
    TcpServer *server = (TcpServer *)arg;

    // 和客户端建立连接
    int cfd = accept(server->listener->lfd, NULL, NULL);
    Debug("交给子反应堆处理");
    // 从线程池中取出子线程的反应堆模型，处理这个cfd
    EventLoop *evLoop = takeWorkerEventLoop(server->threadPool);

    // 将cfd放入子线程反应堆模型中
    tcpConnectionInit(cfd, evLoop);
    return 0;
}
void tcpServerRun(TcpServer *tcp)
{
    // 启动线程池
    ThreadPoolRun(tcp->threadPool);
    // 添加检测任务
    Channel *channel = channelInit(tcp->listener->lfd, ReadEvent, acceptConnection, NULL, NULL, tcp);
    eventLoopAddTask(tcp->mainloop, channel, ADD);
    eventLoopRun(tcp->mainloop);
}
