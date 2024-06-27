#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Log.h"
typedef struct MySQLContext
{
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
} MySQLContext;

// 一次查询
MYSQL_RES *queryOnce(const char *execcmd);
void freeSQLRes(MYSQL_RES *res);