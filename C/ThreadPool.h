#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
#include <stdbool.h>
typedef struct ThreadPool
{
    EventLoop *mainloop;
    bool isStart;
    int threadNum;
    WorkerThread *workerThreads;
    int index;
} ThreadPool;
// 初始化线程池
ThreadPool *ThreadPoolInit(EventLoop *mainLoop, int count);
// 启动线程池
void ThreadPoolRun(ThreadPool *pool);
// 取出线程池中某个子线程的反应堆实例
EventLoop *takeWorkerEventLoop(ThreadPool *pool);