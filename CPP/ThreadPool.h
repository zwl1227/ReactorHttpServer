#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
#include <vector>
class ThreadPool
{
public:
    // 主线程反应堆、子线程个数
    ThreadPool(EventLoop *mainLoop, int count);
    ~ThreadPool();
    void run();
    EventLoop *takeWorkerEventLoop();

private:
    EventLoop *m_mainloop;                  // 主线程反应堆
    bool m_isStart;                         // 线程池启动状态
    int m_threadNum;                        // 子线程个数
    vector<WorkerThread *> m_workerThreads; // 子线程
    int m_index;                            // 预分配任务子线程id
};
