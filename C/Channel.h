#pragma once
#include <stdbool.h>
typedef int (*handleFunc)(void *arg);
// 定义读写事件
enum FDEvent
{
    TimeOut = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};
typedef struct Channel
{
    int fd;                   // 文件描述
    int events;               // 事件
    handleFunc readCallback;  // 读回调函数
    handleFunc writeCallback; // 写回调函数
    handleFunc destroyCallback;
    void *arg; // 回调函数参数
} Channel;
// channel 初始化
struct Channel *channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyCallback, void *arg);
// 修改fd的写事件,检测或不检测
void writeEventEnable(Channel *channel, bool flag);
// 判断是否需要检测文件描述符的写事件
bool isWriteEventEnable(Channel *channel);