#pragma once
#include "mysqlconn.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <nlohmann/json.hpp>

class ConnectionPool
{
public:
    static ConnectionPool* getInstance();
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    std::shared_ptr<MySQLConn> getConnection();
    ~ConnectionPool();

private:
    ConnectionPool();
    bool parseJsonFile();
    void produceConnection();
    void resycleConnection();
    void addConnection();
    
    
    std::string m_ip;
    std::string m_user;
    std::string m_pwd;
    std::string m_dbname;
    int m_port;
    int m_maxSize;
    int m_minSize;
    int m_maxIdleTime;
    int m_timeout;
    
    std::queue<MySQLConn*> connQueue;
    std::mutex m_mutex;
    std::condition_variable consumer_cond;
    std::condition_variable producer_cond;
};