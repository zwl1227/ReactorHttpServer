#pragma once
#include <pthread.h>
#include "EventLoop.h"
// 定义子线程对应的结构体
// 子线程用来执行eventloop代码
typedef struct WorkerThread
{
    pthread_t threadID;
    char name[24];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    EventLoop *evLoop;
} WorkerThread;
// 初始化
int workerThreadInit(WorkerThread *thread, int index);
// 启动线程
void workerThreadRun(WorkerThread *thread);
