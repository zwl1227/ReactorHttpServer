#include "EventLoop.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <error.h>
#include "Log.h"
EventLoop *eventLoopInit()
{
    return eventLoopInitEx(NULL);
}

EventLoop *eventLoopInitEx(const char *threadName)
{
    EventLoop *evLoop = (EventLoop *)malloc(sizeof(EventLoop));
    evLoop->isQuit = false;
    evLoop->threadID = pthread_self();
    pthread_mutex_init(&evLoop->mutex, NULL);
    strcpy(evLoop->threadName, threadName == NULL ? "MainThread" : threadName);
    evLoop->dispatcher = &epollDispatcher;
    evLoop->dispatcherData = evLoop->dispatcher->init();
    // 链表
    evLoop->head = evLoop->tail = NULL;
    // map
    evLoop->channelMap = channelMapInit(128);
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }
    // 指定规则 evLoop->socketPair[0]发，evLoop->socketPair[1]接受数据
    Channel *channel = channelInit(evLoop->socketPair[1], ReadEvent, readLocalMessage, NULL, NULL, evLoop);
    // channel 添加任务队列里
    eventLoopAddTask(evLoop, channel, ADD);
    return evLoop;
}

int eventLoopRun(EventLoop *evLoop)
{
    assert(evLoop != NULL);
    // 取出事件分发和检测模型
    struct Dispatcher *dispatcher = evLoop->dispatcher;
    if (evLoop->threadID != pthread_self())
    {
        return -1;
    }
    while (!evLoop->isQuit)
    {
        // epoll,evloop中的dispatcher使用evLoop中的dispatchdata来进行I/O复用，并使用evloop中的channel执行对应回调函数
        dispatcher->dispatch(evLoop, 2);
        eventLoopProcessTask(evLoop);
    }
    return 0;
}

int eventActivate(EventLoop *evLoop, int fd, int event)
{
    if (fd < 0 || evLoop == NULL)
        return -1;
    struct Channel *channel = evLoop->channelMap->list[fd];
    assert(channel->fd == fd);
    if (event & ReadEvent && channel->readCallback)
    {
        Debug("处理就绪读事件");
        channel->readCallback(channel->arg);
    }
    if (event & WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(channel->arg);
    }
    return 0;
}
// 写数据
void taskWakeup(EventLoop *evLoop)
{
    const char *msg = "write";
    write(evLoop->socketPair[0], msg, sizeof(msg));
}
// 读数据
int readLocalMessage(void *arg)
{
    struct EventLoop *evLoop = (struct EventLoop *)arg;
    char buf[256];
    read(evLoop->socketPair[1], buf, sizeof(buf));
}
int eventLoopAddTask(EventLoop *evLoop, Channel *channel, int type)
{
    // 加锁
    pthread_mutex_lock(&evLoop->mutex);
    // 创建新节点
    ChannelElement *node = (ChannelElement *)malloc(sizeof(ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    // 添加新节点
    if (evLoop->head == NULL)
    {
        evLoop->head = evLoop->tail = node;
    }
    else
    {
        evLoop->tail->next = node;
        evLoop->tail = node;
    }
    pthread_mutex_unlock(&evLoop->mutex);
    // 处理节点
    // 对链表的添加，可能是当前线程也可能是其他线程（主线程）
    // 若是修改，则是当前线程发起，当前线程处理
    // 若是添加，则是由主线程发起的，子线程处理
    // 不能让主线程处理任务队列，需要由当前子线程去处理
    if (evLoop->threadID == pthread_self())
    {
        // 子线程--处理当前线程
        eventLoopProcessTask(evLoop);
    }
    else
    {
        // 主线程--告诉子线程处理任务队列中的任务，让子线程解除阻塞
        taskWakeup(evLoop);
    }
    return 0;
}

int eventLoopProcessTask(EventLoop *evLoop)
{
    pthread_mutex_lock(&evLoop->mutex);
    ChannelElement *head = evLoop->head;
    while (head != NULL)
    {
        Channel *channel = head->channel;
        if (head->type == ADD)
        {
            eventLoopAdd(evLoop, channel);
        }
        else if (head->type == DELETE)
        {
            eventLoopRemove(evLoop, channel);
        }
        else if (head->type == MODIFY)
        {
            eventLoopMod(evLoop, channel);
        }
        ChannelElement *tmp = head;
        head = head->next;
        free(tmp);
    }
    evLoop->head = evLoop->tail = NULL;
    pthread_mutex_unlock(&evLoop->mutex);
    return 0;
}

int eventLoopAdd(EventLoop *evLoop, Channel *channel)
{
    Debug("添加任务处理");
    // 1.把channel的fd和channel以键值对形式存储在channelmap中
    // 2.把channel对应的fd和event上epoll树
    int fd = channel->fd;
    struct ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        // 没有空间,需要扩容
        if (!makeMapRoom(channelMap, fd, sizeof(Channel *)))
        {
            return -1;
        }
    }
    // 查询fd对应的channel位置，并存储
    if (channelMap->list[fd] == NULL)
    {
        channelMap->list[fd] = channel;
        evLoop->dispatcher->add(channel, evLoop);
    }
    return 0;
}

int eventLoopRemove(EventLoop *evLoop, Channel *channel)
{

    // 把channel对应的fd和event从epoll树上移除
    int fd = channel->fd;
    struct ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        // channel不在检测集合中
        return -1;
    }
    int ret = evLoop->dispatcher->remove(channel, evLoop);
    return ret;
}

int eventLoopMod(EventLoop *evLoop, Channel *channel)
{

    // 把channel对应的event更改设置到epoll树上
    int fd = channel->fd;
    struct ChannelMap *channelMap = evLoop->channelMap;
    if (fd >= channelMap->size || channelMap->list[fd] == NULL)
    {

        return -1;
    }

    // channelMap->list[fd] = channel;
    evLoop->dispatcher->modify(channel, evLoop);

    return 0;
}

int destroyChannel(EventLoop *evLoop, Channel *channel)
{
    // 删除channel和fd的队友关系
    evLoop->channelMap->list[channel->fd] = NULL;
    // 关闭fd
    close(channel->fd);
    // 释放channel地址
    free(channel);
    return 0;
}
