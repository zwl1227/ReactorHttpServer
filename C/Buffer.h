#pragma once
typedef struct Buffer
{
    char *data;   // 指向内存的地址
    int capacity; // buffer的容量
    int readpos;  // 读位置
    int writepos; // 写位置
} Buffer;
// 初始化
Buffer *bufferInit(int size);
// 销毁内存
void bufferDestroy(Buffer *buffer);
// 扩容
void bufferExtendRoom(Buffer *buffer, int size);
// 得到剩余可写的内存容量
int bufferWritableSize(Buffer *buffer);
// 得到剩余可读的内存容量
int bufferReadableSize(Buffer *buffer);
// 写内存 1.直接写
int bufferAppendData(Buffer *buffer, const char *data, int size);
int bufferAppendString(Buffer *buffer, const char *data);
// 2.接受套接字数据
int bufferSocketRead(Buffer *buffer, int fd);
// 根据\r\n取出一行,找到其在数据块中的位置，返回该位置
char *bufferFindCRLF(Buffer *buffer);
// 发送数据
int bufferSendData(Buffer *buffer, int socket);