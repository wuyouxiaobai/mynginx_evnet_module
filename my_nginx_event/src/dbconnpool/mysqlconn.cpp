#include "mysqlconn.h"

namespace WYXB
{
MySQLConn::MySQLConn()
{
    conn_ = mysql_init(NULL);
    mysql_set_character_set(conn_, "utf8");
}

MySQLConn::~MySQLConn()
{
    if(conn_ != NULL)
    {
        mysql_close(conn_);
    }
    freeResult();
}
bool MySQLConn::connect(std::string ip, std::string user, std::string pwd, std::string dbname, unsigned int port)
{
    
    MYSQL *p = mysql_real_connect(conn_, ip.c_str(), user.c_str(), pwd.c_str(), dbname.c_str(), port, NULL, 0);
    return p != NULL;

}
bool MySQLConn::update(std::string sql)
{
    if(mysql_query(conn_, sql.c_str()) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
    
}
bool MySQLConn::query(std::string sql)
{
    freeResult();
    if(mysql_query(conn_, sql.c_str()) == 0)
    {
        result_ = mysql_store_result(conn_);
        return true;
    }
    else
    {
        return false;
    }
}
// 遍历查询的结果
bool MySQLConn::next()
{
    // 直接调用 mysql_fetch_row，并将结果赋值给 row_
    row_ = mysql_fetch_row(result_);
    
    // 检查 row_ 是否为 NULL，并返回结果
    return row_ != NULL;
}

// 获取查询结果
std::string MySQLConn::getValue(int index)
{
    int num = mysql_num_fields(result_);
    if(row_ != NULL && index < num)
    {
        return row_[index];
    }
    return "NULL";
}
// 事务操作
bool MySQLConn::transact()
{
    return mysql_autocommit(conn_, false);
}
// 提交事务
bool MySQLConn::submmit()
{
    return mysql_commit(conn_);
}
// 事务回滚
bool MySQLConn::rollback()
{
    return mysql_rollback(conn_);
}

void MySQLConn::freeResult()
{
    if(result_ != NULL)
    {
        mysql_free_result(result_);
        result_ = NULL;
    }
    
}

void MySQLConn::refreshAliveTime()
{
    aliveTime_ = std::chrono::steady_clock::now();
}
long long MySQLConn::getAliveTime()
{
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - aliveTime_);
    return ms.count();
}
}