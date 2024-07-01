#include "MySQLConnectionPool.h"
#include "../Json/json.h"
#include <fstream>
#include "Log.h"
using namespace Json;
// 单例线程安全
MySQLConnectionPool *MySQLConnectionPool::getConnectionPool()
{
    static MySQLConnectionPool pool;
    return &pool;
}

shared_ptr<MySQLConn> MySQLConnectionPool::getConnection()
{
    unique_lock<mutex> locker(m_mutexQ);
    while (m_connectionQ.empty())
    {
        if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout)))
        {
            if (m_connectionQ.empty())
            {
                // wait 超时且队列依然为空，则再去查询申请
                continue;
            }
        }
    }
    // 指定为共享指针，并为其指定删除器，删除器用于回收
    shared_ptr<MySQLConn> connptr(m_connectionQ.front(), [this](MySQLConn *conn)
                                  { 
                                    lock_guard<mutex> locker(m_mutexQ);
                                    conn->refreshAliveTime();
                                    m_connectionQ.push(conn); });

    m_connectionQ.pop();
    m_cond.notify_all();
    return connptr;
}

MySQLConnectionPool::~MySQLConnectionPool()
{
    {
        lock_guard<mutex> locker(m_mutexQ);
        m_isStop = true;
    }
    m_cond.notify_all();
    if (m_producer.joinable())
        m_producer.join();
    if (m_recycler.joinable())
        m_recycler.join();
    while (!m_connectionQ.empty())
    {
        MySQLConn *conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }
}

MySQLConnectionPool::MySQLConnectionPool()
{

    m_isStop = false;
    // 加载配置文件
    if (!parseJsonFile())
    {
        return;
    }
    for (int i = 0; i < m_minSize; i++)
    {
        addConnection();
    }

    // 若空闲链接太少则创建连接
    m_producer = thread(&MySQLConnectionPool::produceConnection, this);
    // 若空闲连接太多则销毁连接
    m_recycler = thread(&MySQLConnectionPool::recycleConnection, this);
}

bool MySQLConnectionPool::parseJsonFile()
{
    // 工作目录已经切换
    ifstream ifs("dbconf.json");
    if (!ifs.is_open())
    {
        Debug("dbconf文件打开失败");
        return false;
    }
    Reader rd;
    Value root;
    rd.parse(ifs, root);
    if (root.isObject())
    {
        m_ip = root["ip"].asString();
        m_port = root["port"].asInt();
        m_user = root["userName"].asString();
        m_password = root["password"].asString();
        m_dbname = root["dbName"].asString();
        m_minSize = root["minSize"].asInt();
        m_maxSize = root["maxSize"].asInt();
        m_maxIdleTime = root["maxIdletime"].asInt();
        m_timeout = root["timeout"].asInt();
        return true;
    }
    return false;
}

void MySQLConnectionPool::produceConnection()
{
    while (true)
    {
        // 若当前可用连接数小于minsize则表示连接不够用，则创建连接
        // 若大于则表示还够用则阻塞producer
        unique_lock<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() >= m_minSize && !m_isStop)
        {
            m_cond.wait(locker);
        }
        if (m_isStop)
            break;
        addConnection();
        m_cond.notify_all();
    }
}

void MySQLConnectionPool::recycleConnection()
{
    while (true)
    {
        this_thread::sleep_for(chrono::seconds(1));
        // 必须维持可用连接数大于最小连接数才可以删除空闲超时的链接
        lock_guard<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() > m_minSize && !m_isStop)
        {
            MySQLConn *conn = m_connectionQ.front();
            if (conn->getAliveTime() >= m_maxIdleTime)
            {
                m_connectionQ.pop();
                delete conn;
            }
            else
            {
                break;
            }
        }
        if (m_isStop)
            break;
    }
}

void MySQLConnectionPool::addConnection()
{
    MySQLConn *conn = new MySQLConn;
    conn->connect(m_user, m_password, m_dbname, m_ip, m_port);
    // 记录建立时间戳
    conn->refreshAliveTime();
    m_connectionQ.push(conn);
}
