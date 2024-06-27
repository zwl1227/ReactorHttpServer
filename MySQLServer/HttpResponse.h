#pragma once
#include "Buffer.h"

// 状态码枚举
enum HttpStatusCode
{
    Unknow,
    OK = 200,               // 请求处理完成
    MovedPermanently = 301, // 持久重定向
    MovedTemporarily = 302, // 临时重定向
    BadRequest = 400,       // 客户端发出的请求错误
    NotFound = 404          // 客户端请求资源不存在
};
// 响应头
typedef struct ResponseHeader
{
    char key[32];
    char value[128];
} ResponseHeader;
// 组织回复给客户端的数据块
typedef void (*responseBody)(const char *filename, Buffer *sendbuf, int socket);
typedef struct HttpResponse
{
    // 状态行：状态码 状态描述
    enum HttpStatusCode statusCode;
    char statusMsg[128];
    char filename[128];
    // 响应头-键值对
    ResponseHeader *headers;
    int headerNum;
    responseBody sendDataFunc;
} HttpResponse;

HttpResponse *httpResponseInit();
void httpResponseDestroy(HttpResponse *response);
// 添加响应头
void httpResponseAddHeader(HttpResponse *response, const char *key, const char *value);
// 组织http响应数据
void httpResponsePrepareMsg(HttpResponse *response, Buffer *sendbuf, int socket);