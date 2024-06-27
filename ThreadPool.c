#include "ThreadPool.h"
#include <assert.h>
#include <stdlib.h>
#include "Log.h"
ThreadPool *ThreadPoolInit(EventLoop *mainLoop, int count)
{
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    pool->index = 0;
    pool->isStart = false;
    pool->mainloop = mainLoop;
    pool->workerThreads = (WorkerThread *)malloc(sizeof(WorkerThread) * count);
    return pool;
}

void ThreadPoolRun(ThreadPool *pool)
{

    // 主线程启动线程池

    assert(pool && !pool->isStart);
    if (pool->mainloop->threadID != pthread_self())
    {
        exit(0);
    }

    pool->isStart = true;
    // 线程池有子线程
    if (pool->threadNum)
    {
        // 启动子线程

        for (int i = 0; i < pool->threadNum; i++)
        {
            workerThreadInit(&pool->workerThreads[i], i);
            workerThreadRun(&pool->workerThreads[i]);
        }
    }
    // 线程池没有子线程，使用主线程处理
}
// 主线程向子线程的evloop添加任务时，需要先获取子线程的evloop
// 主线程函数
EventLoop *takeWorkerEventLoop(ThreadPool *pool)
{
    assert(pool->isStart);
    if (pool->mainloop->threadID != pthread_self())
    {
        exit(0);
    }
    EventLoop *evLoop = pool->mainloop;
    if (pool->threadNum > 0)
    {
        evLoop = pool->workerThreads[pool->index].evLoop;
        pool->index = (pool->index + 1) % pool->threadNum;
    }
    return evLoop;
}
