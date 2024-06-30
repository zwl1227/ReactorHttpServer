#pragma once
#include "Dispatcher.h"
#include <thread>
#include <mutex>
#include <queue>
#include <map>
class Dispatcher;
enum class ElemType : char
{
    ADD,
    DELETE,
    MODIFY
};
// 定义任务节点
typedef struct ChannelElement
{
    ElemType type; // 处理方式
    Channel *channel;
} ChannelElement;
// 反应堆模型

class EventLoop
{
public:
    EventLoop();
    EventLoop(const string threadName);
    ~EventLoop();
    // 启动
    int run();
    // 处理被激活的文件fd
    int eventActive(int fd, int event);
    // 添加任务到任务队列
    int addTask(Channel *channel, ElemType type);
    // 处理任务队列中的任务
    int processTask();

    // 从检测集合中删除，并释放channel
    int freeChannel(Channel *channel);
    // 主线程在添加任务给子线程时，若子线程当前I/O被阻塞且没有任何事件注册将会一直注册
    // 通过此函数，主线程向子线程的socketpait[0]发送数据，则sockerpair[1]的fd将会就绪读事件，
    // 这样就打破阻塞，使得子线程能处理主线程交付的任务
    static int readLocalMessage(void *arg);

    inline thread::id getThreadID()
    {
        return m_threadID;
    }
    inline string getThreadName()
    {
        return m_threadName;
    }

private:
    void taskWakeup();
    int readMessage();
    // 处理任务核心函数
    int add(Channel *channel);
    int remove(Channel *channel);
    int mod(Channel *channel);

private:
    bool m_isQuit;
    Dispatcher *m_dispatcher; // epoll、poll、select dispatcher
    void *m_dispatcherData;
    queue<ChannelElement *> m_taskQ; // 任务队列
    // map fd 和channel映射关系
    map<int, Channel *> m_channelMap;
    // 线程id,name,mutex
    thread::id m_threadID;
    string m_threadName;
    mutex m_mutex;
    int m_socketPair[2]; // 存储本地通信的fd
};
