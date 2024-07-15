#pragma once
#include "Dispatcher.h"
#include <sys/epoll.h>
using namespace std;
class EpollDispatcher : public Dispatcher
{
public:
    EpollDispatcher(EventLoop *evLoop);
    ~EpollDispatcher();
    // 添加
    virtual int add() override;
    // 删除
    virtual int remove() override;
    // 修改
    virtual int modify() override;
    // 事件检测
    virtual int dispatch(int timeout = 2) override;

private:
    int epollCtl(int op);

private:
    int m_epfd;                   // epoll红黑树标识符
    struct epoll_event *m_events; // 就绪的events数组
    const int m_maxNode = 520;
};