#include "WorkerThread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
int workerThreadInit(WorkerThread *thread, int index)
{
    thread->evLoop = NULL;
    thread->threadID = 0;
    sprintf(thread->name, "SubThread-%d", index);
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
    return 0;
}

// 子线程的回调函数
void *subThreadRunning(void *arg)
{
    WorkerThread *thread = (WorkerThread *)arg;
    pthread_mutex_lock(&thread->mutex);
    thread->evLoop = eventLoopInitEx(thread->name);
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);
    eventLoopRun(thread->evLoop);
    return NULL;
}
// 主线程调用
void workerThreadRun(WorkerThread *thread)
{
    // 创建子线程
    pthread_create(&thread->threadID, NULL, subThreadRunning, thread);
    // 阻塞主线程,因为主线程负责向子线程中的evloop中添加任务，若子线程的evloop没有创建完成则阻塞主线程，等待子线程创建evloop
    pthread_mutex_lock(&thread->mutex);
    while (thread->evLoop == NULL)
    {
        pthread_cond_wait(&thread->cond, &thread->mutex);
    }
    pthread_mutex_unlock(&thread->mutex);
}
