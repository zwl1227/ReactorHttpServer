#include "ChannelMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
ChannelMap *channelMapInit(int size)
{
    ChannelMap *map = (ChannelMap *)malloc(sizeof(ChannelMap));
    map->size = size;
    map->list = (Channel **)malloc(size * sizeof(Channel *));
    return map;
}

void channelMapClear(ChannelMap *map)
{
    if (map != NULL)
    {
        for (int i = 0; i < map->size; i++)
        {
            if (map->list[i] != NULL)
            {
                free(map->list[i]);
            }
        }
        free(map->list);
        map->list = NULL;
    }
    map->size = 0;
}

bool makeMapRoom(ChannelMap *map, int newSize, int unitSize)
{
    if (map->size < newSize)
    {
        int curSize = map->size;

        while (curSize < newSize)
        {
            // 容量每次扩大原来的一倍
            curSize *= 2;
        }
        Channel **temp = realloc(map->list, curSize * unitSize);
        if (temp == NULL)
        {
            return false;
        }
        map->list = temp;
        memset(&map->list[map->size], 0, (curSize - map->size) * unitSize);
        map->size = curSize;
    }
    return true;
}
