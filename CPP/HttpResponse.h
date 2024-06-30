#pragma once
#include "Buffer.h"
#include <map>
#include <functional>
#include <string>
using namespace std;
// 状态码枚举
enum class StatusCode
{
    Unknow,
    OK = 200,               // 请求处理完成
    MovedPermanently = 301, // 持久重定向
    MovedTemporarily = 302, // 临时重定向
    BadRequest = 400,       // 客户端发出的请求错误
    NotFound = 404          // 客户端请求资源不存在
};
// 组织回复给客户端的数据块
class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    // 添加响应头
    void addHeader(const string &key, const string &value);
    // 组织http响应数据
    void prepareMsg(Buffer *sendbuf, int socket);
    function<void(string, Buffer *, int)> sendDataFunc;

    inline void setStatus(StatusCode code)
    {
        m_statusCode = code;
    }
    inline void setFileName(string name)
    {
        m_filename = name;
    }

private:
    // 状态行：状态码 状态描述
    StatusCode m_statusCode;
    string m_filename;
    // 响应头-键值对
    map<string, string> m_headers;
    // 定义状态码和描述
    map<int, string> m_info = {
        {200, "OK"},
        {301, "MovedPermanently"},
        {302, "MovedTemporarily"},
        {400, "BadRequest"},
        {404, "NotFound"}};
};
