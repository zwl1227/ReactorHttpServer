#include "Dispatcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#define MAX_FDSET_NUMBER 1024
struct SelectData
{

    fd_set readfds;  // 读集合
    fd_set writefds; // 写集合
};

static void *selectInit();
// 添加
static int selectAdd(Channel *channel, struct EventLoop *evLoop);
// 删除
static int selectRemove(Channel *channel, struct EventLoop *evLoop);
// 修改
static int selectModify(Channel *channel, struct EventLoop *evLoop);
// 事件检测
static int selectDispatch(struct EventLoop *evLoop, int timeout);
// 清除数据（关闭fd或者释放内存)
static int selectClear(struct EventLoop *evLoop);
static void setFdSet(Channel *channel, struct SelectData *data);
static void clearFdSet(Channel *channel, struct SelectData *data);
struct Dispatcher selectDispatcher = {
    selectInit,
    selectAdd,
    selectRemove,
    selectModify,
    selectDispatch,
    selectClear};

static void *selectInit()
{
    struct SelectData *data = (struct SelectData *)malloc(sizeof(struct SelectData));
    FD_ZERO(&data->readfds);
    FD_ZERO(&data->writefds);
    return data;
}

static int selectAdd(Channel *channel, EventLoop *evLoop)
{
    struct SelectData *data = (struct SelectData *)evLoop->dispatcherData;
    if (channel->fd >= MAX_FDSET_NUMBER)
    {
        return -1;
    }
    setFdSet(channel, data);
    return 0;
}

static int selectRemove(Channel *channel, EventLoop *evLoop)
{
    struct SelectData *data = (struct SelectData *)evLoop->dispatcherData;
    clearFdSet(channel, data);
    channel->destroyCallback(channel->arg);
    return 0;
}

static int selectModify(Channel *channel, EventLoop *evLoop)
{
    struct SelectData *data = (struct SelectData *)evLoop->dispatcherData;
    clearFdSet(channel, data);
    setFdSet(channel, data);
    return 0;
}

static int selectDispatch(EventLoop *evLoop, int timeout)
{
    struct SelectData *data = (struct SelectData *)evLoop->dispatcherData;
    struct timeval val;
    val.tv_sec = timeout;
    val.tv_usec = 0;
    fd_set rdtmp = data->readfds;
    fd_set wrtmp = data->writefds;
    int count = select(MAX_FDSET_NUMBER, &rdtmp, &wrtmp, NULL, &val);
    if (count == -1)
    {
        perror("select");
        return -1;
    }
    for (int i = 0; i < MAX_FDSET_NUMBER; i++)
    {

        if (FD_ISSET(i, &rdtmp))
        {
            eventActivate(evLoop, i, ReadEvent);
        }
        if (FD_ISSET(i, &wrtmp))
        {
            eventActivate(evLoop, i, WriteEvent);
        }
    }
    return 0;
}

static int selectClear(EventLoop *evLoop)
{
    struct SelectData *data = (struct SelectData *)evLoop->dispatcherData;
    free(data);
    return 0;
}
static void setFdSet(Channel *channel, struct SelectData *data)
{
    if (channel->events & ReadEvent)
        FD_SET(channel->fd, &data->readfds);
    if (channel->events & WriteEvent)
        FD_SET(channel->fd, &data->writefds);
}
static void clearFdSet(Channel *channel, struct SelectData *data)
{
    if (channel->events & ReadEvent)
        FD_CLR(channel->fd, &data->readfds);
    if (channel->events & WriteEvent)
        FD_CLR(channel->fd, &data->writefds);
}