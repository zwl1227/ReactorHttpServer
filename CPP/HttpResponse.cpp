#include "HttpResponse.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Log.h"
#define MAX_HEADERS_SIZE 16

HttpResponse::HttpResponse()
{
    m_statusCode = StatusCode::Unknow;
    m_headers.clear();
    m_filename = string();
    sendDataFunc = nullptr;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addHeader(const string &key, const string &value)
{
    if (key == string() || value == string())
    {
        return;
    }
    m_headers.insert(make_pair(key, value));
}
void HttpResponse::prepareMsg(Buffer *sendbuf, int socket)
{
    // 状态行
    char tmp[1024] = {0};
    int code = static_cast<int>(m_statusCode);
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", (int)m_statusCode, m_info.at(code).data());
    sendbuf->appendString(tmp);
    // 响应头
    for (auto it = m_headers.begin(); it != m_headers.end(); it++)
    {
        sprintf(tmp, "%s: %s\r\n", it->first.data(), it->second.data());
        sendbuf->appendString(tmp);
    }
    // 空行
    sendbuf->appendString("\r\n");
#ifndef MSG_SEND_AUTO
    sendbuf->sendData(socket);
#endif
    // 回复数据
    sendDataFunc(m_filename, sendbuf, socket);
}
