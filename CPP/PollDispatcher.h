#pragma once
#include "Dispatcher.h"
#include <sys/poll.h>
using namespace std;
class PollDispatcher : public Dispatcher
{
public:
    PollDispatcher(EventLoop *evLoop);
    ~PollDispatcher();
    // 添加
    virtual int add() override;
    // 删除
    virtual int remove() override;
    // 修改
    virtual int modify() override;
    // 事件检测
    virtual int dispatch(int timeout = 2) override;

private:
    int m_maxfd;
    struct pollfd *m_fds;
    const int m_maxNode = 1024;
};