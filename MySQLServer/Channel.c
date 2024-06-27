#include "Channel.h"
#include <stdlib.h>
Channel *channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyCallback, void *arg)
{
    Channel *channel = (Channel *)malloc(sizeof(Channel));
    channel->fd = fd;
    channel->events = events;
    channel->readCallback = readFunc;
    channel->writeCallback = writeFunc;
    channel->destroyCallback = destroyCallback;
    channel->arg = arg;
    return channel;
}

void writeEventEnable(Channel *channel, bool flag)
{
    if (flag)
        channel->events |= WriteEvent;
    else
        channel->events = channel->events & ~WriteEvent;
}

bool isWriteEventEnable(Channel *channel)
{

    return channel->events & WriteEvent;
}
