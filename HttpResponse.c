#include "HttpResponse.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Log.h"
#define MAX_HEADERS_SIZE 16
HttpResponse *httpResponseInit()
{
    HttpResponse *response = (HttpResponse *)malloc(sizeof(HttpResponse));
    response->headerNum = 0;
    int size = sizeof(ResponseHeader) * MAX_HEADERS_SIZE;
    response->headers = (ResponseHeader *)malloc(size);
    response->statusCode = Unknow;
    bzero(response->headers, size);
    bzero(response->statusMsg, sizeof(response->statusMsg));
    bzero(response->filename, sizeof(response->filename));
    response->sendDataFunc = NULL;
    return response;
}

void httpResponseDestroy(HttpResponse *response)
{
    Debug("response destroy!");
    if (response != NULL)
    {
        free(response->headers);
        free(response);
    }
}

void httpResponseAddHeader(HttpResponse *response, const char *key, const char *value)
{
    if (response == NULL || key == NULL || value == NULL)
    {
        return;
    }
    Debug("response->headerNum:%d", response->headerNum);
    strcpy(response->headers[response->headerNum].key, key);
    strcpy(response->headers[response->headerNum].value, value);
    response->headerNum++;
}

void httpResponsePrepareMsg(HttpResponse *response, Buffer *sendbuf, int socket)
{
    // 状态行
    char tmp[1024] = {0};
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->statusCode, response->statusMsg);
    bufferAppendString(sendbuf, tmp);
    // 响应头
    for (int i = 0; i < response->headerNum; i++)
    {
        sprintf(tmp, "%s: %s\r\n", response->headers[i].key, response->headers[i].value);
        bufferAppendString(sendbuf, tmp);
    }
    // 空行
    bufferAppendString(sendbuf, "\r\n");
#ifndef MSG_SEND_AUTO
    bufferSendData(sendbuf, socket);
#endif
    // 回复数据
    response->sendDataFunc(response->filename, sendbuf, socket);
}
