#include "Channel.h"
#include <stdlib.h>
Channel::Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyCallback, void *arg)
{
    m_arg = arg;
    m_fd = fd;
    m_events = (int)events;
    m_readCallback = readFunc;
    m_writeCallback = writeFunc;
    m_destroyCallback = destroyCallback;
}

void Channel::writeEventEnable(bool flag)
{
    if (flag)
        m_events |= static_cast<int>(FDEvent::WriteEvent);
    else
        m_events = m_events & ~static_cast<int>(FDEvent::WriteEvent);
}

bool Channel::isWriteEventEnable()
{
    return m_events & static_cast<int>(FDEvent::WriteEvent);
}
