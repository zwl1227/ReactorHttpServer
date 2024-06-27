#include "Dispatcher.h"
#include <stdio.h>
#include <sys/poll.h>
#include <stdlib.h>
#define MAX_POLLFD_NUMBER 1024
struct PollData
{
    int maxfd;                            // epoll红黑树标识符
    struct pollfd fds[MAX_POLLFD_NUMBER]; // events数组
};

static void *pollInit();
// 添加
static int pollAdd(Channel *channel, struct EventLoop *evLoop);
// 删除
static int pollRemove(Channel *channel, struct EventLoop *evLoop);
// 修改
static int pollModify(Channel *channel, struct EventLoop *evLoop);
// 事件检测
static int pollDispatch(struct EventLoop *evLoop, int timeout);
// 清除数据（关闭fd或者释放内存)
static int pollClear(struct EventLoop *evLoop);
struct Dispatcher pollDispatcher = {
    pollInit,
    pollAdd,
    pollRemove,
    pollModify,
    pollDispatch,
    pollClear};

static void *pollInit()
{
    struct PollData *data = (struct PollData *)malloc(sizeof(struct PollData));
    data->maxfd = 0;
    for (int i = 0; i < MAX_POLLFD_NUMBER; ++i)
    {
        data->fds[i].fd = -1;
        data->fds[i].events = 0;
        data->fds[i].revents = 0;
    }
    return data;
}

static int pollAdd(Channel *channel, EventLoop *evLoop)
{
    struct PollData *data = (struct PollData *)evLoop->dispatcherData;
    int events = 0;
    if (channel->events & ReadEvent)
        events |= POLLIN;
    if (channel->events & WriteEvent)
        events |= POLLOUT;
    int i = 0;
    for (i; i < MAX_POLLFD_NUMBER; i++)
    {
        if (data->fds[i].fd == -1)
        {
            data->fds[i].fd = channel->fd;
            data->fds[i].events = events;
            data->maxfd = data->maxfd < i ? i : data->maxfd;
            break;
        }
    }
    if (i >= MAX_POLLFD_NUMBER)
    {
        perror("超过最大pollfd数");
        return -1;
    }
    return 0;
}

static int pollRemove(Channel *channel, EventLoop *evLoop)
{
    struct PollData *data = (struct PollData *)evLoop->dispatcherData;
    int events = 0;
    int i = 0;
    for (i; i < MAX_POLLFD_NUMBER; i++)
    {
        if (data->fds[i].fd == channel->fd)
        {
            data->fds[i].fd = -1;
            data->fds[i].events = 0;
            data->fds[i].revents = 0;
            break;
        }
    }
    // 通过channel释放对应的TcpConnection
    channel->destroyCallback(channel->arg);
    if (i >= MAX_POLLFD_NUMBER)
    {
        perror("没有找到需要删除的pollfd");
        return -1;
    }
    return 0;
}

static int pollModify(Channel *channel, EventLoop *evLoop)
{
    struct PollData *data = (struct PollData *)evLoop->dispatcherData;
    int events = 0;
    if (channel->events & ReadEvent)
        events |= POLLIN;
    if (channel->events & WriteEvent)
        events |= POLLOUT;
    int i = 0;
    for (i; i < MAX_POLLFD_NUMBER; i++)
    {
        if (data->fds[i].fd == channel->fd)
        {
            data->fds[i].events = events;
            break;
        }
    }
    if (i >= MAX_POLLFD_NUMBER)
    {
        perror("没有找到需要修改的pollfd");
        return -1;
    }
    return 0;
}

static int pollDispatch(EventLoop *evLoop, int timeout)
{
    struct PollData *data = (struct PollData *)evLoop->dispatcherData;
    int count = poll(data->fds, data->maxfd + 1, timeout * 1000);
    if (count == -1)
    {
        perror("poll");
        return -1;
    }
    for (int i = 0; i <= data->maxfd; i++)
    {

        if (data->fds[i].fd == -1)
            continue;
        if (data->fds[i].revents & POLLIN)
        {
            eventActivate(evLoop, data->fds[i].fd, ReadEvent);
        }
        if (data->fds[i].revents & POLLOUT)
        {
            eventActivate(evLoop, data->fds[i].fd, WriteEvent);
        }
    }

    return 0;
}

static int pollClear(EventLoop *evLoop)
{
    struct PollData *data = (struct PollData *)evLoop->dispatcherData;
    free(data);
    return 0;
}
