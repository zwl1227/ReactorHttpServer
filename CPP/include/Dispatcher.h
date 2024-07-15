#pragma once
#include "Channel.h"
#include <string>
#include "EventLoop.h"
using namespace std;
class EventLoop;
class Dispatcher
{
public:
    Dispatcher(EventLoop *evLoop);
    virtual ~Dispatcher();
    // 添加
    virtual int add();
    // 删除
    virtual int remove();
    // 修改
    virtual int modify();
    // 事件检测
    virtual int dispatch(int timeout = 2);
    inline void setChannel(Channel *channel)
    {
        m_channel = channel;
    }

protected:
    EventLoop *m_evLoop;
    string m_name = string();
    Channel *m_channel;
};