#include "Dispatcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "Log.h"
#define MAX_EVENTS_NUMBER 520
typedef struct EpollData
{
    int epfd;                   // epoll红黑树标识符
    struct epoll_event *events; // events数组
} EpollData;

static void *epollInit();
// 添加
static int epollAdd(Channel *channel, struct EventLoop *evLoop);
// 删除
static int epollRemove(Channel *channel, struct EventLoop *evLoop);
// 修改
static int epollModify(Channel *channel, struct EventLoop *evLoop);
// 事件检测
static int epollDispatch(struct EventLoop *evLoop, int timeout);
// 清除数据（关闭fd或者释放内存)
static int epollClear(struct EventLoop *evLoop);
static int epollCtl(Channel *channel, struct EventLoop *evLoop, int op);
struct Dispatcher epollDispatcher = {
    epollInit,
    epollAdd,
    epollRemove,
    epollModify,
    epollDispatch,
    epollClear};

static void *epollInit()
{
    struct EpollData *data = (struct EpollData *)malloc(sizeof(struct EpollData));
    data->epfd = epoll_create(10);
    if (data->epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    // calloc 初始化地址并且赋初值0
    data->events = (struct epoll_event *)calloc(MAX_EVENTS_NUMBER, sizeof(struct epoll_event));
    return data;
}

static int epollAdd(Channel *channel, EventLoop *evLoop)
{
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("epoll_ctl_add");
        exit(0);
    }
    return ret;
}

static int epollRemove(Channel *channel, EventLoop *evLoop)
{
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("epoll_ctl_del");
        exit(0);
    }
    channel->destroyCallback(channel->arg);
    return ret;
}

static int epollModify(Channel *channel, EventLoop *evLoop)
{
    int ret = epollCtl(channel, evLoop, EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("epoll_ctl_mod");
        exit(0);
    }
    return ret;
}

static int epollDispatch(EventLoop *evLoop, int timeout)
{
    struct EpollData *data = (EpollData *)evLoop->dispatcherData;
    int count = epoll_wait(data->epfd, data->events, MAX_EVENTS_NUMBER, timeout * 1000);
    for (int i = 0; i < count; i++)
    {

        int events = data->events[i].events;
        int fd = data->events[i].data.fd;
        // EPOLLERR对端断开连接，EPOLLHUP对端断开连接仍然通信
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            // 断开连接
            // epollRemove(channel, evLoop);
            continue;
        }
        if (events & EPOLLIN)
        {

            eventActivate(evLoop, fd, ReadEvent);
        }
        if (events & EPOLLOUT)
        {
            eventActivate(evLoop, fd, WriteEvent);
        }
    }

    return 0;
}

static int epollClear(EventLoop *evLoop)
{
    EpollData *data = (EpollData *)evLoop->dispatcherData;
    free(data->events);
    close(data->epfd);
    free(data);
    return 0;
}

static int epollCtl(Channel *channel, EventLoop *evLoop, int op)
{
    EpollData *data = (EpollData *)evLoop->dispatcherData;
    struct epoll_event ev;
    ev.data.fd = channel->fd;
    int events = 0;
    if (channel->events & ReadEvent)
        events |= EPOLLIN;
    if (channel->events & WriteEvent)
        events |= EPOLLOUT;
    ev.events = events;
    int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
    return ret;
}
