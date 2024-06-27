#pragma once
#include "Channel.h"
typedef struct ChannelMap
{
    int size; // 记录指针指向数组的元素个数
    struct Channel **list;
} ChannelMap;
ChannelMap *channelMapInit(int size);
// 清空
void channelMapClear(ChannelMap *map);
// 重新分配空间,扩容
bool makeMapRoom(ChannelMap *map, int newSize, int unitSize);