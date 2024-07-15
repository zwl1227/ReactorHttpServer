#pragma once
#include <functional>
using namespace std;
// 定义读写事件
enum class FDEvent
{
    TimeOut = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};
// 可调用对象包装器 1.函数指针 2.可调用对象（可以像函数一样使用）
class Channel
{
public:
    using handleFunc = function<int(void *arg)>;
    Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyCallback, void *arg);
    handleFunc m_readCallback;  // 读回调函数
    handleFunc m_writeCallback; // 写回调函数
    handleFunc m_destroyCallback;
    // 修改fd的写事件,检测或不检测
    void writeEventEnable(bool flag);
    // 判断是否需要检测文件描述符的写事件
    bool isWriteEventEnable();
    // 取出私有成员
    inline int getEvent()
    {
        return m_events;
    }
    inline int getSocket()
    {
        return m_fd;
    }
    inline const void *getArg()
    {
        return m_arg;
    }

private:
    int m_fd;     // 文件描述
    int m_events; // 事件
    void *m_arg;  // 回调函数参数
};
