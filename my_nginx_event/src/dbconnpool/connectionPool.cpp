#include "connectionPool.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <functional>
#include "my_logger.h"
#include <unistd.h>
#include <filesystem>
namespace fs = std::filesystem;

namespace WYXB
{

using json = nlohmann::json;

ConnectionPool* ConnectionPool::getInstance()
{
    static ConnectionPool connPool;
    return &connPool;
}

bool ConnectionPool::parseJsonFile()
{
    // 获取可执行文件所在目录（而非当前工作目录）
    char exe_path[65535];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
    if (len == -1) {
        Logger::ngx_log_stderr(0, "Failed to get executable path");
        return false;
    }
    exe_path[len] = '\0';
    fs::path exe_dir = fs::path(exe_path).parent_path();

    // 向上回溯到项目根目录（假设可执行文件在 build/bin 等子目录）
    fs::path config_path = exe_dir.parent_path() / "config";  // 从 build/bin 回退到项目根目录
    fs::path json_file = config_path / "mysql.json";

    if (!fs::exists(json_file)) {
        Logger::ngx_log_stderr(0, "Config file not found: %s", json_file.c_str());
        return false;
    }
    // 若 config 目录不存在，则直接尝试当前目录
    if (!fs::exists(config_path) || !fs::is_directory(config_path)) {
        json_file = fs::current_path() / "mysql.json";
        Logger::ngx_log_stderr(0, "config directory not found, trying current directory");
    }

    std::ifstream ifs(json_file);
    if (!ifs.is_open()) {
        Logger::ngx_log_stderr(0, "Failed to open json file: %s", json_file.c_str());
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
        Logger::ngx_log_stderr(0, "add connection success");
    }
    else
    {
        Logger::ngx_log_stderr(0, "connect mysql failed");
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
}