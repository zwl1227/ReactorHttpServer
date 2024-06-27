#define _GNU_SOURCE
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
Buffer *bufferInit(int size)
{
    Buffer *buffer = (Buffer *)malloc(sizeof(Buffer));
    if (buffer != NULL)
    {
        buffer->data = (char *)malloc(size);
        buffer->capacity = size;
        buffer->readpos = 0;
        buffer->writepos = 0;
        memset(buffer->data, '\0', size);
    }
    return buffer;
}

void bufferDestroy(Buffer *buffer)
{
    if (buffer != NULL)
    {
        if (buffer->data != NULL)
        {
            free(buffer->data);
        }
        free(buffer);
    }
}

void bufferExtendRoom(Buffer *buffer, int size)
{
    // 内存够用 - 不需要扩容
    if (bufferWritableSize(buffer) >= size)
        return;
    // 内存需要合并才够用
    else if ((buffer->readpos + bufferWritableSize(buffer)) >= size)
    {
        // 得到未读的内存大小
        int readable = bufferReadableSize(buffer);
        memcpy(buffer->data, buffer->data + buffer->readpos, readable);
        // 更新位置
        buffer->readpos = 0;
        buffer->writepos = readable;
    }
    // 内存不够用
    else
    {
        void *temp = realloc(buffer->data, buffer->capacity + size);
        if (temp == NULL)
            return;
        // 只初始化新添加的空间
        memset(temp + buffer->capacity, 0, size);
        // 更新数据
        buffer->data = temp;
        buffer->capacity = buffer->capacity + size;
    }
}

int bufferWritableSize(Buffer *buffer)
{

    return buffer->capacity - buffer->writepos;
}

int bufferReadableSize(Buffer *buffer)
{
    return buffer->writepos - buffer->readpos;
}

int bufferAppendData(Buffer *buffer, const char *data, int size)
{
    if (buffer == NULL || data == NULL || data <= 0)
    {
        return -1;
    }
    // 扩容
    bufferExtendRoom(buffer, size);
    // 数据拷贝
    memcpy(buffer->data + buffer->writepos, data, size);
    buffer->writepos += size;
    return size;
}

int bufferAppendString(Buffer *buffer, const char *data)
{
    int size = strlen(data);
    int ret = bufferAppendData(buffer, data, size);
    return ret;
}

int bufferSocketRead(Buffer *buffer, int fd)
{
    struct iovec vec[2];
    // 初始化
    int writeable = bufferWritableSize(buffer);
    vec[0].iov_base = buffer->data + buffer->writepos;
    vec[0].iov_len = writeable;
    char *tmpbuf = (char *)malloc(40960);
    vec[1].iov_base = buffer->data + buffer->writepos;
    vec[1].iov_len = writeable;
    int result = readv(fd, vec, 2);
    if (result == -1)
    {
        return -1;
    }
    else if (result <= writeable)
    {
        buffer->writepos += result;
    }
    else
    {
        buffer->writepos = buffer->capacity;
        bufferAppendData(buffer, tmpbuf, result - writeable);
    }
    return result;
}

char *bufferFindCRLF(Buffer *buffer)
{
    // strstr 大字符串中匹配子字符串 strstr(buffer, "\r\n")
    // memmem 从大数据块中匹配子数据块，需要指定子数据块大小
    return memmem(buffer->data + buffer->readpos, bufferReadableSize(buffer), "\r\n", 2);
}

int bufferSendData(Buffer *buffer, int socket)
{

    // 判断buffer有无数据
    int readable = bufferReadableSize(buffer);
    if (readable > 0)
    {
        int count = send(socket, buffer->data + buffer->readpos, readable, MSG_NOSIGNAL);
        if (count)
        {
            buffer->readpos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}
