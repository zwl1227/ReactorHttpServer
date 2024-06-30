#include "SelectDispatcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

SelectDispatcher::SelectDispatcher(EventLoop *evLoop) : Dispatcher(evLoop)
{
    FD_ZERO(&m_readfds);
    FD_ZERO(&m_writefds);
    m_name = "Select";
}

SelectDispatcher::~SelectDispatcher()
{
}

int SelectDispatcher::add()
{
    if (m_channel->getSocket() >= m_maxSize)
    {
        return -1;
    }
    setFdSet();
    return 0;
}

int SelectDispatcher::remove()
{

    clearFdSet();
    m_channel->m_destroyCallback(const_cast<void *>(m_channel->getArg()));
    return 0;
}

int SelectDispatcher::modify()
{

    clearFdSet();
    setFdSet();
    return 0;
}

int SelectDispatcher::dispatch(int timeout)
{

    struct timeval val;
    val.tv_sec = timeout;
    val.tv_usec = 0;
    fd_set rdtmp = m_readfds;
    fd_set wrtmp = m_writefds;
    int count = select(m_maxSize, &rdtmp, &wrtmp, NULL, &val);
    if (count == -1)
    {
        perror("select");
        return -1;
    }
    for (int i = 0; i < m_maxSize; i++)
    {

        if (FD_ISSET(i, &rdtmp))
        {
            m_evLoop->eventActive(i, static_cast<int>(FDEvent::ReadEvent));
        }
        if (FD_ISSET(i, &wrtmp))
        {
            m_evLoop->eventActive(i, static_cast<int>(FDEvent::WriteEvent));
        }
    }
    return 0;
}

void SelectDispatcher::setFdSet()
{
    if (m_channel->getEvent() & static_cast<int>(FDEvent::ReadEvent))
        FD_SET(m_channel->getSocket(), &m_readfds);
    if (m_channel->getEvent() & static_cast<int>(FDEvent::WriteEvent))
        FD_SET(m_channel->getSocket(), &m_writefds);
}

void SelectDispatcher::clearFdSet()
{
    if (m_channel->getEvent() & static_cast<int>(FDEvent::ReadEvent))
        FD_CLR(m_channel->getSocket(), &m_readfds);
    if (m_channel->getEvent() & static_cast<int>(FDEvent::WriteEvent))
        FD_CLR(m_channel->getSocket(), &m_writefds);
}
