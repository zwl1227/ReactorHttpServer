#include <iostream>
#include <memory>
#include "ConnectionPool.h"
#include <chrono>
using namespace std;

void op2(ConnectionPool *pool, int begin, int end)
{
    for (int i = begin; i < end; i++)
    {
        shared_ptr<MySQLConn> conn = pool->getConnection();
        char sql[1024] = {0};
        sprintf(sql, "select * from tb_user where id = %d", i);
        conn->query(sql);
        while (conn->next())
        {
            cout << "value:" << conn->value(3) << endl;
        }
    }
}
void test2()
{
    chrono::steady_clock::time_point start = chrono::steady_clock::now();
    ConnectionPool *pool = ConnectionPool::getConnectionPool();
    op2(pool, 12, 25);
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    auto len = end - start;
    cout << "毫秒" << len.count() / 1000000 << endl;
}
int query()
{
    MySQLConn conn;
    conn.connect("root", "123456", "itcast", "127.0.0.1");
    // string sql = "insert into tb_user values(25,'李白','17799990024','213123123@qq.com','软件工程',30,2,4,'2003-05-26 00:00:00') ";
    // bool flag = conn.update(sql);
    // cout << "flag value:" << flag << endl;
    string sql = "select * from tb_user";
    cout << "flag value:" << conn.query(sql) << endl;
    while (conn.next())
    {
        cout << "value:" << conn.value(3) << endl;
    }
    return 0;
}
int main()
{
    test2();
    return 0;
}