#pragma once
#include "Buffer.h"
#include "HttpResponse.h"
#include <string>
#include <map>
#include <functional>
using namespace std;
// #define MSG_SEND_AUTO
// 当前解析状态
enum class ProcessState : char
{
    ParseReqLine,
    ParseReqHeaders,
    ParseReqBody,
    ParseReqDone
};
class HttpRequest
{
public:
    HttpRequest();
    ~HttpRequest();
    void reset();
    // 获取处理状态
    ProcessState getState();
    // 添加请求头
    void addHeader(string key, string value);
    // 根据key取value
    string getHeader(string key);
    // 解析请求行
    bool parseHttpRequestLine(Buffer *buf);
    // 解析请求头
    bool parseHttpRequestHeader(Buffer *buf);
    // 获取请求头
    string httpRequestGetHeader(string key);
    // 解析http请求协议
    bool parseHttpRequest(Buffer *buf, HttpResponse *response, Buffer *sendbuf, int socket);
    // 处理http请求协议
    bool processHttpRequest(HttpResponse *response);

    inline void setMethod(string method)
    {
        m_method = method;
    }
    inline void setUrl(string url)
    {
        m_url = url;
    }
    inline void setVersion(string version)
    {
        m_version = version;
    }
    inline void setState(ProcessState state)
    {
        m_curState = state;
    }

private:
    char *splitRequestLine(const char *start, const char *end, const char *sub, function<void(string)> callback);
    int hexToDec(char c);
    string decodeMsg(string str);
    const string getFileType(string name);
    static void sendFile(string fileName, Buffer *sendbuf, int cfd);
    static void sendDir(string dirName, Buffer *sendbuf, int cfd);

private:
    string m_method;                  // 请求方法
    string m_url;                     // 请求文件
    string m_version;                 // 请求版本
    map<string, string> m_reqHeaders; // 请求头键值对
    int m_reqHeadersNum;              // 请求头键值对个数
    ProcessState m_curState;
};
