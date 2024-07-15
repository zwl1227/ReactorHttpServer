#include "ThreadPool.h"
#include <assert.h>
#include <stdlib.h>
#include "Log.h"

ThreadPool::ThreadPool(EventLoop *mainLoop, int count) : m_mainloop(mainLoop), m_threadNum(count)
{
    m_index = 0;
    m_isStart = false;
    m_workerThreads.clear();
}

ThreadPool::~ThreadPool()
{
}

void ThreadPool::run()
{
    // 主线程启动线程池
    assert(!m_isStart);
    if (m_mainloop->getThreadID() != this_thread::get_id())
    {
        exit(0);
    }

    m_isStart = true;
    // 线程池有子线程
    if (m_threadNum)
    {
        // 启动子线程
        for (int i = 0; i < m_threadNum; i++)
        {
            WorkerThread *subthread = new WorkerThread(i);
            m_workerThreads.push_back(subthread);
            subthread->run();
        }
    }
    // 线程池没有子线程，使用主线程处理。。。
}
// 主线程向子线程的evloop添加任务时，需要先获取子线程的evloop
// 主线程函数
EventLoop *ThreadPool::takeWorkerEventLoop()
{
    assert(m_isStart);
    if (m_mainloop->getThreadID() != this_thread::get_id())
    {
        exit(0);
    }
    EventLoop *evLoop = m_mainloop;
    if (m_threadNum > 0)
    {
        evLoop = m_workerThreads[m_index]->getEventLoop();
        m_index = (m_index + 1) % m_threadNum;
    }
    return evLoop;
}
