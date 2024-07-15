#include "TcpServer.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "TcpConnection.h"
TcpServer::TcpServer(unsigned short port, int threadNum)
{
    m_port = port;
    setListen();
    m_mainloop = new EventLoop();
    m_threadNum = threadNum;
    m_threadPool = new ThreadPool(m_mainloop, threadNum);
}

TcpServer::~TcpServer()
{
}

void TcpServer::setListen()
{
    // 创建监听的fd
    m_lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_lfd == -1)
    {
        perror("socket");
        return;
    }
    // 设置端口复用
    int opt = 1;
    int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1)
    {
        perror("setsockopt");
        return;
    }
    // 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(m_lfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("bind");
        return;
    }
    // 监听
    ret = listen(m_lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        return;
    }
}

void TcpServer::run()
{
    // 启动线程池
    m_threadPool->run();
    // 添加检测任务
    Debug("listen fd:%d", m_lfd);
    Channel *channel = new Channel(m_lfd, FDEvent::ReadEvent, acceptConnection, NULL, NULL, this);
    m_mainloop->addTask(channel, ElemType::ADD);
    m_mainloop->run();
}

int TcpServer::acceptConnection(void *arg)
{
    Debug("建立新链接");
    TcpServer *server = (TcpServer *)arg;
    // 和客户端建立连接
    int cfd = accept(server->m_lfd, NULL, NULL);
    Debug("交给子反应堆处理");
    // 从线程池中取出子线程的反应堆模型，处理这个cfd
    EventLoop *evLoop = server->m_threadPool->takeWorkerEventLoop();
    // 将cfd放入子线程反应堆模型中
    TcpConnection *tcpConn = new TcpConnection(cfd, evLoop);
    return 0;
}
