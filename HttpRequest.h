#pragma once
#include <stdbool.h>
#include "Buffer.h"
#include "HttpResponse.h"
#define MSG_SEND_AUTO
// 请求键值对
typedef struct RequestHeader
{
    char *key;
    char *value;
} RequestHeader;
// 当前解析状态
enum HttpRequestState
{
    ParseReqLine,
    ParseReqHeaders,
    ParseReqBody,
    ParseReqDone
};
typedef struct HttpRequest
{
    char *method;              // 请求方法
    char *url;                 // 请求文件
    char *version;             // 请求版本
    RequestHeader *reqHeaders; // 请求头键值对
    int reqHeadersNum;         // 请求头键值对个数
    enum HttpRequestState curState;
} HttpRequest;

HttpRequest *httpRequestInit();
void httpRequestReset(HttpRequest *request);
void httpRequestResetEx(HttpRequest *request);
void httpRequestDestroy(HttpRequest *request);
// 获取处理状态
enum HttpRequestState httpRequestState(HttpRequest *request);
// 添加请求头
void httpRequestAddHeader(HttpRequest *request, const char *key, const char *value);
// 根据key取value
char *httpRequestGetHeader(HttpRequest *request, const char *key);
// 解析请求行
char *splitRequestLine(const char *start, const char *end, const char *sub, char **ptr);
bool parseHttpRequestLine(HttpRequest *request, Buffer *buf);
// 解析请求头
bool parseHttpRequestHeader(HttpRequest *request, Buffer *buf);
// 解析http请求协议
bool parseHttpRequest(HttpRequest *request, Buffer *buf, HttpResponse *response, Buffer *sendbuf, int socket);
// 处理http请求协议
bool processHttpRequest(HttpRequest *request, HttpResponse *response);
