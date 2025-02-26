#include "connectionPool.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <functional>


using json = nlohmann::json;

ConnectionPool* ConnectionPool::getInstance()
{
    static ConnectionPool connPool;
    return &connPool;
}

bool ConnectionPool::parseJsonFile()
{
    std::ifstream ifs("mysql.json");
    if(!ifs.is_open())
    {
        std::cout << "open json file failed" << std::endl;
        return false;
    }
    json j;
    ifs >> j;
    m_ip = j.at("ip");
    m_port = j.at("port").get<int>();
    m_user = j.at("username");
    m_pwd = j.at("password");
    m_dbname = j.at("dbname");
    m_maxSize = j.at("maxSize").get<int>();
    m_minSize = j.at("minSize").get<int>();
    m_maxIdleTime = j.at("maxIdleTime").get<int>();
    m_timeout = j.at("timeout").get<int>();
    
    return true;
    
}

void ConnectionPool::addConnection()
{
    MySQLConn *conn = new MySQLConn;
    if(conn->connect(m_ip, m_user, m_pwd, m_dbname))
    {
        conn->refreshAliveTime();
        connQueue.push(conn);
        std::cout << "add connection success" << std::endl;
    }
    else
    {
        std::cout << "connect mysql failed" << std::endl;
        delete conn;
    }
}

ConnectionPool::ConnectionPool()
{
    if(!parseJsonFile())
    {
        exit(EXIT_FAILURE);
    }
    
    for(int i = 0; i < m_minSize; ++i)
    {
        addConnection();
    }
    std::thread produceConn(&ConnectionPool::produceConnection, this);
    std::thread resycleConn(&ConnectionPool::resycleConnection, this);
    produceConn.detach();
    resycleConn.detach();
    
}

void ConnectionPool::produceConnection()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (connQueue.size() >= m_minSize)
        {
            producer_cond.wait(lock);
        }
        addConnection();
        consumer_cond.notify_all();
    }
    
}
void ConnectionPool::resycleConnection()
{
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::lock_guard<std::mutex> lock(m_mutex);
        while (connQueue.size() > m_maxSize)
        {
            MySQLConn *conn = connQueue.front();
            if(conn->getAliveTime() > m_maxIdleTime)
            {
                connQueue.pop();
                delete conn;
            }
            else
            {
                break;
            }
            
        }
        
    }
}

std::shared_ptr<MySQLConn> ConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (connQueue.empty())
    {
        if(consumer_cond.wait_for(lock, std::chrono::seconds(m_timeout)) == std::cv_status::timeout)
        {
            if(connQueue.empty())
            {
                continue;
            }
        }
    }
    std::shared_ptr<MySQLConn> connptr(connQueue.front(), [&](MySQLConn *p)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        p->refreshAliveTime();
        connQueue.push(p);
        
    });
    connQueue.pop();
    producer_cond.notify_all();
    return connptr;
    
}

ConnectionPool::~ConnectionPool()
{
    while (!connQueue.empty())
    {
        MySQLConn *conn = connQueue.front();
        connQueue.pop();
        delete conn;
    }
}