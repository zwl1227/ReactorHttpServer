#pragma once
#include <string>
using namespace std;
class Buffer
{
public:
    Buffer(int size);
    ~Buffer();
    // 扩容
    void extendRoom(int size);
    // 得到剩余可写的内存容量
    int writableSize();
    // 得到剩余可读的内存容量
    int readableSize();
    // 写内存 1.直接写
    int appendData(const char *data, int size);
    int appendString(const char *data);
    int appendString(string data);
    // 2.接受套接字数据
    int socketRead(int fd);
    // 根据\r\n取出一行,找到其在数据块中的位置，返回该位置
    char *findCRLF();
    // 发送数据
    int sendData(int socket);
    inline char *data()
    {
        return m_data;
    }
    inline int getReadPos()
    {
        return m_readpos;
    }
    inline int getWritePos()
    {
        return m_writepos;
    }
    inline int readPosIncrease(int count)
    {
        m_readpos += count;
        return m_readpos;
    }
    inline int writePosIncrease(int count)
    {
        m_writepos += count;
        return m_writepos;
    }

private:
    char *m_data;       // 指向内存的地址
    int m_capacity;     // buffer的容量
    int m_readpos = 0;  // 读位置
    int m_writepos = 0; // 写位置
};
