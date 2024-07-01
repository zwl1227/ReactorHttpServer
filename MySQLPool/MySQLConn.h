#pragma once
#include <mysql/mysql.h>
#include <string>
#include <chrono>
using namespace std;
using namespace chrono;
class MySQLConn
{
public:
    MySQLConn();
    ~MySQLConn();
    // 链接数据库
    bool connect(string user, string passwd, string dbName, string ip, unsigned short port = 3306);
    // 更新数据库 insert,update,delete
    bool update(string sql);
    // 查询数据库 select
    bool query(string sql);
    // 遍历结果集
    bool next();
    // 得到结果集中的字段值
    string value(int index);
    // 事务操作
    bool transaction();
    // 提交事务
    bool commit();
    // 事务回滚
    bool rollback();
    // 刷新起始空闲时间点
    void refreshAliveTime();
    // 计算链接存活时长
    long long getAliveTime();

private:
    void freeResult();

private:
    MYSQL *m_conn;
    MYSQL_RES *m_result;
    MYSQL_ROW m_row;
    steady_clock::time_point m_alivetime;
};