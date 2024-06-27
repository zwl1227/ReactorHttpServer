#include "TcpMySQL.h"

MYSQL_RES *queryOnce(const char *execcmd)
{
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    conn = mysql_init(NULL);
    if (conn == NULL)
    {
        printf("mysql_init failed\n");
        return NULL;
    }

    if (mysql_real_connect(conn, "localhost", "root", "123456", "resume_system", 0, NULL, 0) == NULL)
    {
        printf("mysql_real_connect failed\n");
        mysql_close(conn);
        return NULL;
    }
    if (mysql_query(conn, "SELECT * FROM resume_info "))
    {
        printf("mysql_query failed\n");
        mysql_close(conn);
        return NULL;
    }
    res = mysql_store_result(conn);
    mysql_close(conn);
    return res;
}

void freeSQLRes(MYSQL_RES *res)
{
    mysql_free_result(res);
}
