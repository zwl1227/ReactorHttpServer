#define _GNU_SOURCE
#include "HttpRequest.h"
#include <stdio.h>
#include <stdlib.h>
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
#define BUF_SIZE 1024
#define HTTP_REPLY_BUF_SIZE 4096
#define HeaderSize 12
HttpRequest *httpRequestInit()
{
    HttpRequest *request = (HttpRequest *)malloc(sizeof(HttpRequest));
    httpRequestReset(request);
    request->reqHeaders = (RequestHeader *)malloc(sizeof(RequestHeader) * HeaderSize);
    return request;
}

void httpRequestReset(HttpRequest *request)
{
    request->curState = ParseReqLine;
    request->method = NULL;
    request->url = NULL;
    request->version = NULL;
    request->reqHeadersNum = 0;
}

void httpRequestResetEx(HttpRequest *request)
{

    free(request->method);
    free(request->url);
    free(request->version);
    httpRequestReset(request);
    if (request->reqHeaders != NULL)
    {
        for (int i = 0; i < request->reqHeadersNum; i++)
        {
            free(request->reqHeaders[i].key);
            free(request->reqHeaders[i].value);
        }
        free(request->reqHeaders);
    }
    httpRequestReset(request);
}

void httpRequestDestroy(HttpRequest *request)
{
    if (request != NULL)
    {
        httpRequestResetEx(request);
        free(request);
    }
}

enum HttpRequestState httpRequestState(HttpRequest *request)
{
    return request->curState;
}

void httpRequestAddHeader(HttpRequest *request, const char *key, const char *value)
{
    // requestheaders key为空不能使用strcpy
    request->reqHeaders[request->reqHeadersNum].key = (char *)key;
    request->reqHeaders[request->reqHeadersNum].value = (char *)value;
    request->reqHeadersNum++;
}

char *httpRequestGetHeader(HttpRequest *request, const char *key)
{
    if (request != NULL)
    {
        for (int i = 0; i < request->reqHeadersNum; i++)
        {
            if (strncasecmp(request->reqHeaders[i].key, key, strlen(key)) == 0)
            {
                return request->reqHeaders[i].value;
            }
        }
    }
    return NULL;
}

char *splitRequestLine(const char *start, const char *end, const char *sub, char **ptr)
{
    int lineSize = end - start;
    char *space = (char *)end;
    if (sub != NULL)
    {
        space = memmem(start, lineSize, sub, strlen(sub));
        assert(space != NULL);
    }
    int length = space - start;
    char *tmp = (char *)malloc(length + 1);
    strncpy(tmp, start, length);
    tmp[length] = '\0';
    *ptr = tmp;
    return space + 1;
}

bool parseHttpRequestLine(HttpRequest *request, Buffer *buf)
{
    // 读出请求行
    char *end = bufferFindCRLF(buf);
    char *start = buf->data + buf->readpos;
    // 请求行总长度
    int lineSize = end - start;
    if (lineSize)
    {
        // get /xxx/xx.txt http/1.1
#if 0
        // 请求方式
        char *space = memmem(start, lineSize, " ", 1);
        assert(space != NULL);
        int methodSize = space - start;
        request->method = (char *)malloc(methodSize + 1);
        strncpy(request->method, start, methodSize);
        request->method[methodSize] = "\0";
        // 请求静态资源
        start = space + 1;
        space = memmem(start, end - start, " ", 1);
        assert(space != NULL);
        int urlSize = space - start;
        request->url = (char *)malloc(urlSize + 1);
        strncpy(request->method, start, urlSize);
        request->url[urlSize] = "\0";
        // 版本
        start = space + 1;
        int versionSize = start - end;
        request->version = (char *)malloc(versionSize + 1);
        strncpy(request->version, start, versionSize);
        request->url[versionSize] = "\0";
#else
        start = splitRequestLine(start, end, " ", &request->method);
        start = splitRequestLine(start, end, " ", &request->url);
        splitRequestLine(start, end, NULL, &request->version);
#endif
        // 为请求头做准备
        buf->readpos += lineSize;
        buf->readpos += 2;
        request->curState = ParseReqHeaders;
        return true;
    }
    return false;
}

bool parseHttpRequestHeader(HttpRequest *request, Buffer *buf)
{
    char *end = bufferFindCRLF(buf);
    if (end != NULL)
    {
        char *start = buf->data + buf->readpos;
        int lineSize = end - start;
        char *middle = memmem(start, lineSize, ": ", 2);
        if (middle != NULL)
        {

            char *key = (char *)malloc(middle - start + 1);
            strncpy(key, start, middle - start);
            key[middle - start] = '\0';

            char *value = (char *)malloc(end - middle - 2 + 1);
            strncpy(value, middle + 2, end - middle - 2);
            value[end - middle - 2] = '\0';
            httpRequestAddHeader(request, key, value);
            // 移动读数据的位置
            buf->readpos += lineSize;
            buf->readpos += 2;
        }
        else
        {
            // 请求头被解析完了，跳过空行
            buf->readpos += 2;
            // 读数据块

            if (strncasecmp(request->method, "get", strlen(request->method)) == 0)
            {
                // 如果是get请求没有数据块，无需解析
                request->curState = ParseReqDone;
            }
            else if (strncasecmp(request->method, "post", strlen(request->method)) == 0)
            {

                request->curState = ParseReqBody;
            }
        }
        return true;
    }

    return false;
}

bool parseHttpRequest(HttpRequest *request, Buffer *buf, HttpResponse *response, Buffer *sendbuf, int socket)
{
    bool flag = true;
    // 读取readbuf中的数据，解析数据存储给request
    while (request->curState != ParseReqDone)
    {
        switch (request->curState)
        {
        case ParseReqLine:
            flag = parseHttpRequestLine(request, buf);

            break;
        case ParseReqHeaders:
            flag = parseHttpRequestHeader(request, buf);

            break;
        case ParseReqBody:
            break;
        default:
            break;
        }
        if (!flag)
            return flag;
        // 判断是否解析完毕，若解析完毕，则需要回复数据
        // request->response
        if (request->curState == ParseReqDone)
        {
            Debug("解析请求头完成");
            // 1.组织响应头
            processHttpRequest(request, response);
            // 2.将状态行、响应头和响应数据body写入sendbuf
            httpResponsePrepareMsg(response, sendbuf, socket);
        }
    }
    request->curState = ParseReqLine; // 状态还原，保证还能继续处理第二条以后得需求

    return flag;
}
static int hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}
static void decodeMsg(char *to, char *from)
{

    for (; *from != ' '; ++to, ++from)
    {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[1]))
        {
            // 16进制转为10进制
            *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);
            from += 2;
        }
        else
        {
            *to = *from;
        }
    }
}
static const char *getFileType(const char *name)
{
    // 从右往左找
    const char *dot = strrchr(name, '.');
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
static void sendFile(const char *fileName, Buffer *sendbuf, int cfd)
{
    // 打开文件
    int fd = open(fileName, O_RDONLY);
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
            bufferAppendData(sendbuf, buf, len);
#ifndef MSG_SEND_AUTO
            bufferSendData(sendbuf, cfd);
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
static int
compare_filenames(const struct dirent **a, const struct dirent **b)
{
    return strcasecmp((*a)->d_name, (*b)->d_name);
}
static void sendDir(const char *dirName, Buffer *sendbuf, int cfd)
{
    char replyBuf[HTTP_REPLY_BUF_SIZE] = {0};
    sprintf(replyBuf, "<html><head><title>%s</title></head><body><table>", dirName);
    struct dirent **namelist;
    // 遍历目录
    int num = scandir(dirName, &namelist, NULL, compare_filenames);
    for (int i = 0; i < num; i++)
    {
        // 取出文件名
        char *name = namelist[i]->d_name;
        struct stat st;
        char subPath[1024] = {0};
        sprintf(subPath, "%s/%s", dirName, name);
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

        bufferAppendString(sendbuf, replyBuf);
#ifndef MSG_SEND_AUTO
        bufferSendData(sendbuf, cfd);
#endif
        memset(replyBuf, 0, sizeof(replyBuf));
        free(namelist[i]);
    }
    // test执行一次sql
    MYSQL_RES *res = queryOnce(NULL);
    int num_fields = mysql_num_fields(res);
    MYSQL_ROW row;
    while (row = mysql_fetch_row(res))
    {
        sprintf(replyBuf + strlen(replyBuf), "<tr>");
        for (int i = 1; i < num_fields; i++)
        {
            if (i == 5)
                sprintf(replyBuf + strlen(replyBuf), "<td><a href = \"%s\">%s</a></td>", row[i], row[i]);
            else
                sprintf(replyBuf + strlen(replyBuf), "<td>%s</td>", row[i]);
        }
        sprintf(replyBuf + strlen(replyBuf), "</tr>");
        bufferAppendString(sendbuf, replyBuf);
#ifndef MSG_SEND_AUTO
        bufferSendData(sendbuf, cfd);
#endif
        memset(replyBuf, 0, sizeof(replyBuf));
    }
    freeSQLRes(res);
    sprintf(replyBuf + strlen(replyBuf), "</table></body></html>");
    bufferAppendString(sendbuf, replyBuf);
#ifndef MSG_SEND_AUTO
    bufferSendData(sendbuf, cfd);
#endif
    free(namelist);
}
bool processHttpRequest(HttpRequest *request, HttpResponse *response)
{

    if (strcasecmp(request->method, "get") != 0)
        return false;

    // 解析中文路径
    // decodeMsg(request->url, request->url);
    // Debug("开始处理解析，请求文件为：%s", request->url);
    char *file = NULL;
    // 若path是根目录，则是当前工作路径
    if (strcmp(request->url, "/") == 0)
    {
        file = "./";
    }
    // 若path不是根目录，则是当前工作路径下的一个相对路径，把第一个'/'给去掉
    else
    {
        file = request->url + 1;
    }

    // 获取文件属性
    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1)
    {
        // 文件不存在 -- 回复404
        // sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
        // sendFile("404.html", cfd);
        strcpy(response->filename, "404.html");
        response->statusCode = NotFound;
        strcpy(response->statusMsg, "Not Found");
        // 响应头
        httpResponseAddHeader(response, "Content-type", getFileType(".html"));
        response->sendDataFunc = sendFile;
        return true;
    }
    strcpy(response->filename, file);
    response->statusCode = OK;
    strcpy(response->statusMsg, "OK");
    if (S_ISDIR(st.st_mode))
    {
        // 把目录发给客户端
        // sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
        // sendDir(file, cfd);
        // 响应头
        httpResponseAddHeader(response, "Content-type", getFileType(".html"));
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
        httpResponseAddHeader(response, "Content-type", getFileType(file));
        httpResponseAddHeader(response, "Content-length", tmp);
        response->sendDataFunc = sendFile;
    }

    return false;
}
