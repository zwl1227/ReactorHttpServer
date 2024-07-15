#pragma once
#include "Dispatcher.h"
#include <sys/select.h>
using namespace std;
class SelectDispatcher : public Dispatcher
{
public:
    SelectDispatcher(EventLoop *evLoop);
    ~SelectDispatcher();
    // 添加
    virtual int add() override;
    // 删除
    virtual int remove() override;
    // 修改
    virtual int modify() override;
    // 事件检测
    virtual int dispatch(int timeout = 2) override;

private:
    void setFdSet();
    void clearFdSet();

private:
    fd_set m_readfds;  // 读集合
    fd_set m_writefds; // 写集合
    const int m_maxSize = 1024;
};