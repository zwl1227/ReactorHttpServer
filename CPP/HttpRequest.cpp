#include "HttpRequest.h"
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>
#include "Log.h"
#include "TcpMySQL.h"
#include "MySQLConnectionPool.h"
#define BUF_SIZE 1024
#define HTTP_REPLY_BUF_SIZE 4096
#define HeaderSize 12

static int
compare_filenames(const struct dirent **a, const struct dirent **b)
{
    return strcasecmp((*a)->d_name, (*b)->d_name);
}

HttpRequest::HttpRequest()
{
    reset();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::reset()
{
    m_curState = ProcessState::ParseReqLine;
    m_method = string();
    m_url = string();
    m_version = string();
    m_reqHeadersNum = 0;
}

ProcessState HttpRequest::getState()
{
    return m_curState;
}

void HttpRequest::addHeader(string key, string value)
{
    if (key.empty() || value.empty())
    {
        return;
    }
    m_reqHeaders.insert(make_pair(key, value));
}

string HttpRequest::getHeader(string key)
{
    auto item = m_reqHeaders.find(key);
    if (item == m_reqHeaders.end())
        return string();
    return item->second;
}

bool HttpRequest::parseHttpRequestLine(Buffer *buf)
{
    // 读出请求行
    char *end = buf->findCRLF();
    char *start = buf->data();
    // 请求行总长度
    int lineSize = end - start;
    if (lineSize > 0)
    {
        // get /xxx/xx.txt http/1.1
        auto methodFunc = bind(&HttpRequest::setMethod, this, placeholders::_1);
        start = splitRequestLine(start, end, " ", methodFunc);
        auto urlFunc = bind(&HttpRequest::setUrl, this, placeholders::_1);
        start = splitRequestLine(start, end, " ", urlFunc);
        auto versionFunc = bind(&HttpRequest::setVersion, this, placeholders::_1);
        splitRequestLine(start, end, NULL, versionFunc);
        // 为请求头做准备
        buf->readPosIncrease(lineSize + 2);
        setState(ProcessState::ParseReqHeaders);
        return true;
    }
    return false;
}

bool HttpRequest::parseHttpRequestHeader(Buffer *buf)
{
    char *end = buf->findCRLF();
    if (end != nullptr)
    {
        char *start = buf->data() + buf->getReadPos();
        int lineSize = end - start;
        char *middle = static_cast<char *>(memmem(start, lineSize, ": ", 2));
        if (middle != NULL)
        {
            int keyLen = middle - start;
            int valueLen = end - middle - 2;
            if (keyLen > 0 && valueLen > 0)
            {
                string key(start, keyLen);
                string value(middle + 2, valueLen);
                addHeader(key, value);
            }
            // 移动读数据的位置
            buf->readPosIncrease(lineSize + 2);
        }
        else
        {
            // 请求头被解析完了，跳过空行
            buf->readPosIncrease(2);
            // 读数据块
            if (m_method == "GET")
            {
                // 如果是get请求没有数据块，无需解析
                setState(ProcessState::ParseReqDone);
            }
            else if (m_method == "POST")
            {
                setState(ProcessState::ParseReqBody);
            }
        }
        return true;
    }

    return false;
}

string HttpRequest::httpRequestGetHeader(string key)
{
    auto it = m_reqHeaders.find(key);
    if (it != m_reqHeaders.end())
        return it->second;
    return string();
}

bool HttpRequest::parseHttpRequest(Buffer *buf, HttpResponse *response, Buffer *sendbuf, int socket)
{
    bool flag = true;
    // 读取readbuf中的数据，解析数据存储给request
    while (m_curState != ProcessState::ParseReqDone)
    {
        switch (m_curState)
        {
        case ProcessState::ParseReqLine:
            flag = parseHttpRequestLine(buf);
            break;
        case ProcessState::ParseReqHeaders:
            flag = parseHttpRequestHeader(buf);
            break;
        case ProcessState::ParseReqBody:
            break;
        default:
            break;
        }
        if (!flag)
            return flag;
        // 判断是否解析完毕，若解析完毕，则需要回复数据
        // request->response
        if (m_curState == ProcessState::ParseReqDone)
        {
            Debug("解析请求头完成");
            // 1.组织响应头
            processHttpRequest(response);
            // 2.将状态行、响应头和响应数据body写入sendbuf
            response->prepareMsg(sendbuf, socket);
        }
    }
    m_curState = ProcessState::ParseReqLine; // 状态还原，保证还能继续处理第二条以后得需求
    return flag;
}

bool HttpRequest::processHttpRequest(HttpResponse *response)
{

    if (m_method != "GET")
        return false;

    // 解析中文路径
    // m_url = decodeMsg(m_url);
    // Debug("开始处理解析，请求文件为：%s", request->url);
    const char *file = nullptr;
    // 若path是根目录，则是当前工作路径
    if (m_url == "/")
    {
        file = "./";
    }
    // 若path不是根目录，则是当前工作路径下的一个相对路径，把第一个'/'给去掉
    else
    {
        file = m_url.data() + 1;
    }
    Debug("%s", file);
    // 获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1)
    {
        // 文件不存在 -- 回复404
        // sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
        // sendFile("404.html", cfd);
        response->setFileName("404.html");
        response->setStatus(StatusCode::NotFound);
        // 响应头
        response->addHeader("Content-type", getFileType(".html"));
        response->sendDataFunc = sendFile;
        return true;
    }
    response->setFileName(file);
    response->setStatus(StatusCode::OK);
    if (S_ISDIR(st.st_mode))
    {
        // 把目录发给客户端
        // sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
        // sendDir(file, cfd);
        // 响应头
        response->addHeader("Content-type", getFileType(".html"));
        response->sendDataFunc = sendDir;
        return true;
    }
    else
    {
        // 把文件发给客户端
        // sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
        // sendFile(file, cfd);
        // 响应头
        Debug("开始发送文件");
        char tmp[12] = {0};
        sprintf(tmp, "%ld", st.st_size);
        response->addHeader("Content-type", getFileType(file).data());
        response->addHeader("Content-length", tmp);
        response->sendDataFunc = sendFile;
        return true;
    }

    return false;
}

char *HttpRequest::splitRequestLine(const char *start, const char *end, const char *sub, function<void(string)> callback)
{
    int lineSize = end - start;
    char *space = const_cast<char *>(end);
    if (sub != nullptr)
    {
        space = static_cast<char *>(memmem(start, lineSize, sub, strlen(sub)));
        assert(space != nullptr);
    }
    int length = space - start;
    callback(string(start, length));
    return space + 1;
}

int HttpRequest::hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}

string HttpRequest::decodeMsg(string str)
{
    const char *from = str.data();
    string result = string();
    for (; *from != ' '; ++from)
    {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[1]))
        {
            // 16进制转为10进制
            result.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));
            from += 2;
        }
        else
        {
            result.append(1, *from);
        }
    }
    result.append(1, '\n');
    return result;
}

const string HttpRequest::getFileType(string name)
{
    // 从右往左找
    const char *dot = strrchr(name.data(), '.');
    if (dot == NULL)
        return "text/plain;charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html;charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    return "text/plain;charset=utf-8";
}

void HttpRequest::sendFile(string fileName, Buffer *sendbuf, int cfd)
{
    // 打开文件
    int fd = open(fileName.data(), O_RDONLY);
    assert(fd > 0);
#if 1
    while (1)
    {
        char buf[BUF_SIZE];
        int len = read(fd, buf, sizeof(buf));
        if (len > 0)
        {
            // send(cfd, buf, len, 0);
            // bufferAppendString有bug，这个函数内部计算buf的长度，而buf从fd文件描述符读数据时，在读满则没有'\0',如此计算buf长度失败
            sendbuf->appendData(buf, len);
#ifndef MSG_SEND_AUTO
            sendbuf->sendData(cfd);
#endif
        }
        else if (len == 0)
        {
            // 读完
            break;
        }
        else
        {
            close(fd);
            perror("read");
            return;
        }
    }
#else
    off_t offset = 0;
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    // 大文件循环发送
    while (offset < size)
    {
        int ret = sendfile(cfd, fd, &offset, size);
    }

#endif
    close(fd);
}

void HttpRequest::sendDir(string dirName, Buffer *sendbuf, int cfd)
{
    char replyBuf[HTTP_REPLY_BUF_SIZE] = {0};
    sprintf(replyBuf, "<html><head><title>%s</title></head><body><table>", dirName.data());
    struct dirent **namelist;
    // 遍历目录
    int num = scandir(dirName.data(), &namelist, NULL, compare_filenames);
    for (int i = 0; i < num; i++)
    {
        // 取出文件名
        char *name = namelist[i]->d_name;
        struct stat st;
        char subPath[1024] = {0};
        sprintf(subPath, "%s/%s", dirName.data(), name);
        stat(subPath, &st);
        if (S_ISDIR(st.st_mode))
        {
            sprintf(replyBuf + strlen(replyBuf),
                    "<tr><td><a href = \"/%s\">%s</a></td><td>%ld</td></tr>",
                    name, name, st.st_size);
        }
        else
        {
            sprintf(replyBuf + strlen(replyBuf),
                    "<tr><td><a href = \"%s\">%s</a></td><td>%ld</td></tr>",
                    name, name, st.st_size);
        }

        sendbuf->appendString(replyBuf);
#ifndef MSG_SEND_AUTO
        sendbuf->sendData(cfd);
#endif
        memset(replyBuf, 0, sizeof(replyBuf));
        free(namelist[i]);
    }
    // test执行一次sql
    Debug("sql  once");
    MySQLConnectionPool *pool = MySQLConnectionPool::getConnectionPool();
    shared_ptr<MySQLConn> conn = pool->getConnection();
    Debug("get sql connect  once");
    if (!conn->query("SELECT * FROM resume_info "))
    {
        Error("sql query error");
    }
    int num_fields = conn->getNumFields();
    if (num_fields == 0)
    {
        Debug("sql  error");
    }
    while (conn->next())
    {
        Debug("sql query");
        sprintf(replyBuf + strlen(replyBuf), "<tr>");
        for (int i = 1; i < num_fields; i++)
        {
            if (i == 5)
                sprintf(replyBuf + strlen(replyBuf), "<td><a href = \"%s\">%s</a></td>", conn->value(i).c_str(), conn->value(i).c_str());
            else
                sprintf(replyBuf + strlen(replyBuf), "<td>%s</td>", conn->value(i).c_str());
        }
        sprintf(replyBuf + strlen(replyBuf), "</tr>");
        sendbuf->appendString(replyBuf);
#ifndef MSG_SEND_AUTO
        sendbuf->sendData(cfd);
#endif
        memset(replyBuf, 0, sizeof(replyBuf));
    }
    sprintf(replyBuf + strlen(replyBuf), "</table></body></html>");
    sendbuf->appendString(replyBuf);
#ifndef MSG_SEND_AUTO
    sendbuf->sendData(cfd);
#endif
    free(namelist);
}
