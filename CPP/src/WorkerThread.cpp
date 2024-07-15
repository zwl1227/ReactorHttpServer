#include "WorkerThread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

WorkerThread::WorkerThread(int index)
{
    m_evLoop = nullptr;
    m_thread = nullptr;
    m_threadID = thread::id();
    m_name = "SubThread-" + to_string(index);
}

WorkerThread::~WorkerThread()
{
    if (m_thread != nullptr)
        delete m_thread;
}
// 主线程调用函数
void WorkerThread::run()
{
    // 主线程创建子线程
    m_thread = new thread(&WorkerThread::running, this);
    m_threadID = m_thread->get_id();
    // 阻塞主线程,因为主线程负责向子线程中的evloop中添加任务
    // 若子线程的evloop没有创建完成则阻塞主线程，等待子线程创建evloop
    unique_lock<mutex> locker(m_mutex);
    while (m_evLoop == nullptr)
    {
        m_cond.wait(locker);
    }
}

void WorkerThread::running()
{
    // 使用环境变量唤醒主线程
    m_mutex.lock();
    m_evLoop = new EventLoop(m_name);
    m_mutex.unlock();
    m_cond.notify_one();
    m_evLoop->run();
}
