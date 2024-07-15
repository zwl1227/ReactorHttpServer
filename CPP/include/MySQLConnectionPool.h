#pragma once
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "MySQLConn.h"
using namespace std;

class MySQLConnectionPool
{
public:
    static MySQLConnectionPool *getConnectionPool();
    MySQLConnectionPool(const MySQLConnectionPool &obj) = delete;
    MySQLConnectionPool &operator=(const MySQLConnectionPool &obj) = delete;
    shared_ptr<MySQLConn> getConnection();
    ~MySQLConnectionPool();

private:
    MySQLConnectionPool();
    bool parseJsonFile();
    void produceConnection();
    void recycleConnection();
    void addConnection();
    string m_ip;
    string m_user;
    string m_password;
    string m_dbname;
    unsigned short m_port;
    int m_minSize;     // 连接池最小连接数
    int m_maxSize;     // 连接池最大连接数
    int m_timeout;     // 若当前没有连接可以使用，则阻塞当前调用连接池的线程m_timeout时间，过后再尝试连接
    int m_maxIdleTime; // 若连接空闲时间超时m_maxIdleTime，则关闭
    queue<MySQLConn *> m_connectionQ;
    mutex m_mutexQ;
    condition_variable m_cond;
    bool m_isStop;
    thread m_producer;
    thread m_recycler;
};