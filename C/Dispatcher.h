#pragma once
#include "Channel.h"
#include "EventLoop.h"
struct EventLoop;
struct Dispatcher
{
    // init--初始化epoll poll select需要的数据块
    void *(*init)();
    // 添加
    int (*add)(Channel *channel, struct EventLoop *evLoop);
    // 删除
    int (*remove)(Channel *channel, struct EventLoop *evLoop);
    // 修改
    int (*modify)(Channel *channel, struct EventLoop *evLoop);
    // 事件检测
    int (*dispatch)(struct EventLoop *evLoop, int timeout);
    // 清除数据（关闭fd或者释放内存)
    int (*clear)(struct EventLoop *evLoop);
};
