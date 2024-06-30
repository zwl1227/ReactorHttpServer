#include "PollDispatcher.h"
#include <stdio.h>
#include <stdlib.h>

PollDispatcher::PollDispatcher(EventLoop *evLoop) : Dispatcher(evLoop)
{
    m_maxfd = 0;
    m_fds = new struct pollfd[m_maxNode];
    for (int i = 0; i < m_maxNode; ++i)
    {
        m_fds[i].fd = -1;
        m_fds[i].events = 0;
        m_fds[i].revents = 0;
    }
    m_name = "Poll";
}

PollDispatcher::~PollDispatcher()
{
    delete[] m_fds;
}

int PollDispatcher::add()
{

    int events = 0;
    if (m_channel->getEvent() & static_cast<int>(FDEvent::ReadEvent))
        events |= POLLIN;
    if (m_channel->getEvent() & static_cast<int>(FDEvent::WriteEvent))
        events |= POLLOUT;
    int i = 0;
    for (i; i < m_maxNode; i++)
    {
        if (m_fds[i].fd == -1)
        {
            m_fds[i].fd = m_channel->getSocket();
            m_fds[i].events = events;
            m_maxfd = m_maxfd < i ? i : m_maxfd;
            break;
        }
    }
    if (i >= m_maxNode)
    {
        perror("超过最大pollfd数");
        return -1;
    }
    return 0;
}

int PollDispatcher::remove()
{

    int events = 0;
    int i = 0;
    for (i; i < m_maxNode; i++)
    {
        if (m_fds[i].fd == m_channel->getSocket())
        {
            m_fds[i].fd = -1;
            m_fds[i].events = 0;
            m_fds[i].revents = 0;
            break;
        }
    }
    // 通过channel释放对应的TcpConnection
    m_channel->m_destroyCallback(const_cast<void *>(m_channel->getArg()));
    if (i >= m_maxNode)
    {
        perror("没有找到需要删除的pollfd");
        return -1;
    }
    return 0;
}

int PollDispatcher::modify()
{

    int events = 0;
    if (m_channel->getEvent() & static_cast<int>(FDEvent::ReadEvent))
        events |= POLLIN;
    if (m_channel->getEvent() & static_cast<int>(FDEvent::WriteEvent))
        events |= POLLOUT;
    int i = 0;
    for (i; i < m_maxNode; i++)
    {
        if (m_fds[i].fd == m_channel->getSocket())
        {
            m_fds[i].events = events;
            break;
        }
    }
    if (i >= m_maxNode)
    {
        perror("没有找到需要修改的pollfd");
        return -1;
    }
    return 0;
}

int PollDispatcher::dispatch(int timeout)
{

    int count = poll(m_fds, m_maxfd + 1, timeout * 1000);
    if (count == -1)
    {
        perror("poll");
        return -1;
    }
    for (int i = 0; i <= m_maxfd; i++)
    {

        if (m_fds[i].fd == -1)
            continue;
        if (m_fds[i].revents & POLLIN)
        {
            m_evLoop->eventActive(m_fds[i].fd, static_cast<int>(FDEvent::ReadEvent));
        }
        if (m_fds[i].revents & POLLOUT)
        {
            m_evLoop->eventActive(m_fds[i].fd, static_cast<int>(FDEvent::WriteEvent));
        }
    }
    return 0;
}
