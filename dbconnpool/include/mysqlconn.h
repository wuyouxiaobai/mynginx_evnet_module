#pragma once
#include <mysql.h>
#include <string>
#include <chrono>

class MySQLConn
{
public:
	MySQLConn();
	~MySQLConn();
	bool connect(std::string ip, std::string user, std::string pwd, std::string dbname, unsigned int port=3306);
	bool update(std::string sql);
    bool query(std::string sql);
    // 遍历查询的结果
    bool next();
    // 获取查询结果
    std::string getValue(int index);
    // 事务操作
    bool transact();
    // 提交事务
    bool submmit();
    // 事务回滚
    bool rollback();
    void refreshAliveTime();
    long long getAliveTime();

private:
    void freeResult();
	MYSQL* conn_ = nullptr;
    MYSQL_RES* result_ = nullptr;
    MYSQL_ROW row_;
    std::chrono::steady_clock::time_point aliveTime_;
};