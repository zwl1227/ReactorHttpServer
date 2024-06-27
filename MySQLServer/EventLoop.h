#pragma once
#include "Dispatcher.h"
#include "ChannelMap.h"
#include <pthread.h>
#include <assert.h>
extern struct Dispatcher epollDispatcher;
extern struct Dispatcher pollDispatcher;
extern struct Dispatcher selectDispatcher;
enum ElemType
{
    ADD,
    DELETE,
    MODIFY
};
// 定义任务节点
typedef struct ChannelElement
{
    int type; // 处理方式
    Channel *channel;
    struct ChannelElement *next;
} ChannelElement;
// 反应堆模型
struct Dispatcher;
typedef struct EventLoop
{
    bool isQuit;
    struct Dispatcher *dispatcher;
    void *dispatcherData;
    // 任务队列
    ChannelElement *head;
    ChannelElement *tail;
    // map fd 和channel映射关系
    ChannelMap *channelMap;
    // 线程id,name,mutex
    pthread_t threadID;
    char threadName[32];
    pthread_mutex_t mutex;
    int socketPair[2]; // 存储本地通信的fd
} EventLoop;

// 初始化
EventLoop *eventLoopInit();
EventLoop *eventLoopInitEx(const char *threadName);
// 启动
int eventLoopRun(EventLoop *evLoop);
// 处理被激活的文件fd
int eventActivate(EventLoop *evLoop, int fd, int event);
// 添加任务到任务队列
int eventLoopAddTask(EventLoop *evLoop, Channel *channel, int type);
// 处理任务队列中的任务
int eventLoopProcessTask(EventLoop *evLoop);
// 处理任务核心函数
int eventLoopAdd(EventLoop *evLoop, Channel *channel);
int eventLoopRemove(EventLoop *evLoop, Channel *channel);
int eventLoopMod(EventLoop *evLoop, Channel *channel);
// 释放channel
int destroyChannel(EventLoop *evLoop, Channel *channel);

int readLocalMessage(void *arg);