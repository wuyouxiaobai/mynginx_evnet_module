#include <memory>
#include <iostream>
#include "mysqlconn.h"
#include "connectionPool.h"
#include <functional>
#include <memory>
#include <chrono>
#include <thread>

using namespace std;

//使用连接池
void op1(int begin, int end)
{
    for(int i = begin; i <= end; i++)
    {
        std::shared_ptr<MySQLConn> conn = ConnectionPool::getInstance()->getConnection();
        if(!conn)
        {
            cerr << "get MySQLConn failed" << endl;
            return;
        }
        char sql[1024] = {0};
        sprintf(sql, "insert into User(id, name, password, state) values('%d', 'tom%d', '123456', 'offline')", i, i);
        if(!conn->update(sql))
        {
            cerr << "update failed" << endl;
        }
    
    }
}
// 不使用连接池
void op2(int begin, int end)
{
    for(int i = begin; i <= end; i++)
    {
        MySQLConn conn;
        if(!conn.connect("127.0.0.1", "root", "wuyouxiaobai1413", "chat"))
        {
            cerr << "connect failed" << endl;
        }
        char sql[1024] = {0};
        sprintf(sql, "insert into User(id, name, password, state) values('%d', 'tom%d', '123456', 'offline')", i, i);
        if(!conn.update(sql))
        {
            cerr << "update failed" << endl;
        }
    
    }
}

void test1()
{
#if 1
    //单线程模式使用连接池
    chrono::steady_clock::time_point t1 = chrono::steady_clock::now();
    op1(1, 5000);
    chrono::steady_clock::time_point t2 = chrono::steady_clock::now();
    chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
    cout << "单线程模式使用连接池: " << time_span.count() << "s" << endl;
#else
    //单线程不使用连接池
    chrono::steady_clock::time_point t1 = chrono::steady_clock::now();
    op2(1, 5000);
    chrono::steady_clock::time_point t2 = chrono::steady_clock::now();
    chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
    cout << "单线程不使用连接池: " << time_span.count() << "s" <<endl;
#endif
}

void test2()
{
#if 1
    //多线程模式使用连接池
    chrono::steady_clock::time_point time1 = chrono::steady_clock::now();
    thread t1(op1, 1, 1000);
    thread t2(op1, 1001, 2000);
    thread t3(op1, 2001, 3000);
    thread t4(op1, 3001, 4000);
    thread t5(op1, 4001, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    chrono::steady_clock::time_point time2 = chrono::steady_clock::now();
    chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(time2 - time1);
    cout << "多线程模式使用连接池: " << time_span.count() << "s" << endl;
#else
    //多线程不使用连接池
    chrono::steady_clock::time_point time1 = chrono::steady_clock::now();
    thread t1(op2, 1, 1000);
    thread t2(op2, 1001, 2000);
    thread t3(op2, 2001, 3000);
    thread t4(op2, 3001, 4000);
    thread t5(op2, 4001, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    
    chrono::steady_clock::time_point time2 = chrono::steady_clock::now();
    chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(time2 - time1);
    cout << "多线程模式不使用连接池: " << time_span.count() << "s" << endl;
#endif
}

int query()
{
    MySQLConn conn;
    if (!conn.connect("127.0.0.1", "root", "wuyouxiaobai1413", "chat"))
    {
        std::cerr << "connect failed" << std::endl;
        return -1;  // 返回错误代码
    }

    // 插入新用户
    if (!conn.update("INSERT INTO User (name, password, state) VALUES ('wuyouxiaobai', '123456', 'offline');"))
    {
        std::cerr << "insert failed" << std::endl;
        return -1;  // 返回错误代码
    }

    // 查询用户
    if (!conn.query("SELECT * FROM User"))
    {
        std::cerr << "query failed" << std::endl;
        return -1;  // 返回错误代码
    }

    // 输出查询结果
    while (conn.next())
    {
        std::cout << conn.getValue(0) << " " 
                  << conn.getValue(1) << " " 
                  << conn.getValue(2) << " " 
                  << conn.getValue(3) << std::endl;
    }

    return 0;  // 成功
}


int main()
{
    test2();
    return 0;
}
