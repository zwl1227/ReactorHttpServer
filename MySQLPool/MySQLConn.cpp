#include "MySQLConn.h"

MySQLConn::MySQLConn()
{
    m_conn = mysql_init(nullptr);
    // 设置查询的结果编码方式
    mysql_set_character_set(m_conn, "utf8");
}

MySQLConn::~MySQLConn()
{
    if (m_conn != nullptr)
    {
        mysql_close(m_conn);
    }
    freeResult();
}

bool MySQLConn::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
    MYSQL *ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
    return ptr ? true : false;
}

bool MySQLConn::update(string sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        return false;
    }
    return true;
}

bool MySQLConn::query(string sql)
{
    // 清空上次查询结果
    freeResult();
    if (mysql_query(m_conn, sql.c_str()))
    {
        return false;
    }
    m_result = mysql_store_result(m_conn);
    return true;
}

bool MySQLConn::next()
{
    if (m_result != nullptr)
    {
        m_row = mysql_fetch_row(m_result);
        if (m_row != nullptr)
            return true;
    }
    return false;
}

string MySQLConn::value(int index)
{
    int rowCount = mysql_num_fields(m_result);
    if (index >= rowCount || index < 0)
        return string();
    char *val = m_row[index];
    unsigned long length = mysql_fetch_lengths(m_result)[index];
    return string(val, length);
}

bool MySQLConn::transaction()
{
    // 设置手动提交
    return mysql_autocommit(m_conn, false);
}

bool MySQLConn::commit()
{
    return mysql_commit(m_conn);
}

bool MySQLConn::rollback()
{
    return mysql_rollback(m_conn);
}

void MySQLConn::refreshAliveTime()
{
    m_alivetime = steady_clock::now();
}

long long MySQLConn::getAliveTime()
{
    nanoseconds res = steady_clock::now() - m_alivetime;
    // 纳秒转毫秒
    milliseconds millsec = duration_cast<milliseconds>(res);
    return millsec.count();
}

void MySQLConn::freeResult()
{
    if (m_result != nullptr)
    {
        mysql_free_result(m_result);
        m_result = nullptr;
    }
}
