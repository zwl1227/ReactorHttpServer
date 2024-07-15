#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include "EventLoop.h"
// 定义子线程对应的结构体
// 子线程用来执行eventloop代码
class WorkerThread
{
public:
    WorkerThread(int index);
    ~WorkerThread();
    void run();

    // 向evloop中添加任务时调用
    inline EventLoop *getEventLoop()
    {
        return m_evLoop;
    }

private:
    void running();

private:
    thread *m_thread;
    thread::id m_threadID;
    string m_name;
    mutex m_mutex;
    condition_variable m_cond;
    EventLoop *m_evLoop;
};
