#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "TcpServer.h"
#include "Log.h"
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("./a.out prot path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换工作路径
    chdir(argv[2]);
    // 启动服务器
    Debug("初始化服务器。。。");
    TcpServer *server = new TcpServer(port, 4);
    server->run();
    return 0;
}