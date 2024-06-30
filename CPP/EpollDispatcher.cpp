#include "EpollDispatcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include "Log.h"
#include "TcpConnection.h"
EpollDispatcher::EpollDispatcher(EventLoop *evLoop) : Dispatcher(evLoop)
{
    m_epfd = epoll_create(10);
    if (m_epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    // calloc 初始化地址并且赋初值0
    m_events = (struct epoll_event *)calloc(m_maxNode, sizeof(struct epoll_event));
    m_name = "Epoll";
}

EpollDispatcher::~EpollDispatcher()
{
    close(m_epfd);
    delete[] m_events;
}

int EpollDispatcher::add()
{
    int ret = epollCtl(EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("epoll_ctl_add");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::remove()
{
    int ret = epollCtl(EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("epoll_ctl_del");
        exit(0);
    }
    Debug("~Tcp error");
    m_channel->m_destroyCallback(const_cast<void *>(m_channel->getArg()));
    return ret;
}

int EpollDispatcher::modify()
{
    int ret = epollCtl(EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("epoll_ctl_mod");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
    int count = epoll_wait(m_epfd, m_events, m_maxNode, timeout * 1000);
    for (int i = 0; i < count; i++)
    {

        int events = m_events[i].events;
        int fd = m_events[i].data.fd;
        // EPOLLERR对端断开连接，EPOLLHUP对端断开连接仍然通信
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            // 断开连接
            // epollRemove(channel, evLoop);
            continue;
        }
        if (events & EPOLLIN)
        {

            m_evLoop->eventActive(fd, static_cast<int>(FDEvent::ReadEvent));
        }
        if (events & EPOLLOUT)
        {
            m_evLoop->eventActive(fd, static_cast<int>(FDEvent::WriteEvent));
        }
    }

    return 0;
}

int EpollDispatcher::epollCtl(int op)
{

    struct epoll_event ev;
    ev.data.fd = m_channel->getSocket();
    int events = 0;
    if (m_channel->getEvent() & static_cast<int>(FDEvent::ReadEvent))
        events |= EPOLLIN;
    if (m_channel->getEvent() & static_cast<int>(FDEvent::WriteEvent))
        events |= EPOLLOUT;
    ev.events = events;
    int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
    return ret;
    return 0;
}
