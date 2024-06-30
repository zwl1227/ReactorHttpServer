#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include "Log.h"
Buffer::Buffer(int size) : m_capacity(size)
{
    m_data = (char *)malloc(size);
    bzero(m_data, size);
}

Buffer::~Buffer()
{
    free(m_data);
}

void Buffer::extendRoom(int size)
{
    // 内存够用 - 不需要扩容
    if (writableSize() >= size)
        return;
    // 内存需要合并才够用
    else if ((m_readpos + writableSize()) >= size)
    {
        // 得到未读的内存大小
        int readable = readableSize();
        memcpy(m_data, m_data + m_readpos, readable);
        // 更新位置
        m_readpos = 0;
        m_writepos = readable;
    }
    // 内存不够用
    else
    {
        void *temp = realloc(m_data, m_capacity + size);
        if (temp == NULL)
            return;
        // 只初始化新添加的空间
        memset((char *)temp + m_capacity, 0, size);
        // 更新数据
        m_data = static_cast<char *>(temp);
        m_capacity = m_capacity + size;
    }
}

int Buffer::writableSize()
{
    return m_capacity - m_writepos;
}

int Buffer::readableSize()
{
    return m_writepos - m_readpos;
}

int Buffer::appendData(const char *data, int size)
{
    if (data == NULL || size <= 0)
    {
        return -1;
    }
    // 扩容
    extendRoom(size);
    // 数据拷贝
    memcpy(m_data + m_writepos, data, size);
    m_writepos += size;
    return size;
}

int Buffer::appendString(const char *data)
{
    int size = strlen(data);
    int ret = appendData(data, size);
    return ret;
}

int Buffer::appendString(string data)
{

    int ret = appendString(data.data());
    return ret;
}

int Buffer::socketRead(int fd)
{
    struct iovec vec[2];
    // 初始化
    int writeable = writableSize();
    vec[0].iov_base = m_data + m_writepos;
    vec[0].iov_len = writeable;
    char *tmpbuf = (char *)malloc(40960);
    vec[1].iov_base = m_data + m_writepos;
    vec[1].iov_len = writeable;
    int result = readv(fd, vec, 2);
    if (result == -1)
    {
        return -1;
    }
    else if (result <= writeable)
    {
        m_writepos += result;
    }
    else
    {
        m_writepos = m_capacity;
        appendData(tmpbuf, result - writeable);
    }
    return result;
}

char *Buffer::findCRLF()
{
    // strstr 大字符串中匹配子字符串 strstr(buffer, "\r\n")
    // memmem 从大数据块中匹配子数据块，需要指定子数据块大小
    return static_cast<char *>(memmem(m_data + m_readpos, readableSize(), "\r\n", 2));
}

int Buffer::sendData(int socket)
{
    // 判断buffer有无数据
    int readable = readableSize();
    if (readable > 0)
    {
        int count = send(socket, m_data + m_readpos, readable, MSG_NOSIGNAL);
        if (count)
        {
            m_readpos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}
