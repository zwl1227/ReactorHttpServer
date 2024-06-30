#include "EventLoop.h"
#include "EpollDispatcher.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <error.h>
#include <assert.h>
#include <unistd.h>
#include "Log.h"

EventLoop::EventLoop() : EventLoop(string())
{
}

EventLoop::EventLoop(const string threadName)
{

    m_isQuit = false;
    m_threadID = this_thread::get_id();
    m_threadName = threadName == string() ? "MainThread" : threadName;
    m_dispatcher = new EpollDispatcher(this);
    // map
    m_channelMap.clear();
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }
#if 0
    // 指定规则 evLoop->socketPair[0]发，evLoop->socketPair[1]接受数据
    Channel *channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, readLocalMessage, NULL, NULL, this);
#else
    // 绑定 bind
    auto obj = bind(&EventLoop::readMessage, this);
    Channel *channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, obj, NULL, NULL, this);
#endif

    // channel 添加任务队列里
    addTask(channel, ElemType::ADD);
}

EventLoop::~EventLoop()
{
}

int EventLoop::run()
{
    // 取出事件分发和检测模型
    Dispatcher *dispatcher = m_dispatcher;
    if (m_threadID != this_thread::get_id())
    {
        return -1;
    }
    while (!m_isQuit)
    {
        // Debug("listen accept");
        // epoll,evloop中的dispatcher使用evLoop中的dispatchdata来进行I/O复用，并使用evloop中的channel执行对应回调函数
        dispatcher->dispatch(2);
        processTask();
    }
    return 0;
}

int EventLoop::eventActive(int fd, int event)
{
    if (fd < 0)
        return -1;
    Channel *channel = m_channelMap[fd];
    assert(channel->getSocket() == fd);
    if (event & static_cast<int>(FDEvent::ReadEvent) && channel->m_readCallback)
    {
        channel->m_readCallback(const_cast<void *>(channel->getArg()));
    }
    if (event & static_cast<int>(FDEvent::WriteEvent) && channel->m_writeCallback)
    {
        channel->m_writeCallback(const_cast<void *>(channel->getArg()));
    }
    return 0;
}

int EventLoop::addTask(Channel *channel, ElemType type)
{
    // 加锁
    m_mutex.lock();
    // 创建新节点
    ChannelElement *node = new ChannelElement;
    node->channel = channel;
    node->type = type;
    // 添加新节点
    m_taskQ.push(node);
    m_mutex.unlock();

    // 处理节点
    // 对链表的添加，可能是当前线程也可能是其他线程（主线程）
    // 若是修改，则是当前线程发起，当前线程处理
    // 若是添加，则是由主线程发起的，子线程处理
    // 不能让主线程处理任务队列，需要由当前子线程去处理
    if (m_threadID == this_thread::get_id())
    {
        // 子线程--处理当前线程
        processTask();
    }
    else
    {
        // 主线程--告诉子线程处理任务队列中的任务，让子线程解除阻塞
        taskWakeup();
    }
    return 0;
}

int EventLoop::processTask()
{
    while (!m_taskQ.empty())
    {
        m_mutex.lock();
        ChannelElement *node = m_taskQ.front();
        m_taskQ.pop();
        m_mutex.unlock();
        Channel *channel = node->channel;
        if (node->type == ElemType::ADD)
        {
            add(channel);
        }
        else if (node->type == ElemType::DELETE)
        {

            remove(channel);
        }
        else if (node->type == ElemType::MODIFY)
        {
            mod(channel);
        }

        delete node;
    }

    return 0;
}

int EventLoop::add(Channel *channel)
{
    // 1.把channel的fd和channel以键值对形式存储在channelmap中
    // 2.把channel对应的fd和event上epoll树
    int fd = channel->getSocket();
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        m_channelMap.insert(make_pair(fd, channel));
        m_dispatcher->setChannel(channel);
        int ret = m_dispatcher->add();
        return ret;
    }
    return -1;
}

int EventLoop::remove(Channel *channel)
{
    // 把channel对应的fd和event从epoll树上移除
    int fd = channel->getSocket();
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        // channel不在检测集合中
        return -1;
    }
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->remove();
    return ret;
}

int EventLoop::mod(Channel *channel)
{
    // 把channel对应的event更改设置到epoll树上
    int fd = channel->getSocket();
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    m_dispatcher->modify();
    return 0;
}

int EventLoop::freeChannel(Channel *channel)
{
    auto it = m_channelMap.find(channel->getSocket());
    if (it == m_channelMap.end())
        return -1;
    // 删除channel和fd的队友关系
    m_channelMap.erase(it);
    // 关闭fd
    close(channel->getSocket());
    // 释放channel地址
    delete channel;
    return 0;
}

void EventLoop::taskWakeup()
{
    const char *msg = "write";
    write(m_socketPair[0], msg, sizeof(msg));
}

int EventLoop::readLocalMessage(void *arg)
{
    EventLoop *evLoop = static_cast<EventLoop *>(arg);
    char buf[256];
    read(evLoop->m_socketPair[1], buf, sizeof(buf));
    return 0;
}

int EventLoop::readMessage()
{
    char buf[256];
    read(m_socketPair[1], buf, sizeof(buf));
    return 0;
}
