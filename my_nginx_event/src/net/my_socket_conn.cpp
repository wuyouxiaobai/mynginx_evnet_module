#include "my_socket.h"
#include "my_memory.h"
#include <sys/unistd.h>
#include "my_global.h"
#include <iostream>
#include <string.h>
#include <algorithm>
#include "my_server.h"

namespace WYXB
{


// ----------------------- 创建连接 ----------
ngx_connection_s::ngx_connection_s(): context_(std::make_shared<HttpContext>()){    /* iCurrsequence = 0;*/}
ngx_connection_s::~ngx_connection_s()
{
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
}

// 分配出去一个连接的时候初始化一些内容，原来的内容放在ngx_get_connection
void ngx_connection_s::GetOneToUse(int sockfd)
{
    ++iCurrsequence;
    fd = sockfd;
    // fd = -1; // 还没有分配fd
    // curStat = _PKG_HD_INIT; // 收包状况处于初始状态，准备接受数据包头
    // precvbuf = dataHeadInfo;  // 将包头先收到这里
    // irecvlen = sizeof(COMM_PKG_HEADER); // 指定接收的包头长度

    // precvMemPointer = NULL; // 还没有收到任何数据包
    // iThrowsendCount = 0;
    // psendMemPointer = NULL; // 还没有发送任何数据包
    // events = 0; // 还没有事件
    // lastPingTime = time(NULL); // 记录上次发送ping的时间

    // FloodKickLastTime = 0; // 记录上次Flood攻击的时间
    // FloodAttackCount = 0; // Flood攻击在该时间内收到包的次数统计
    // iSendCount = 0; // 发送队列中数据条目数，若client只发不收，此数可能过大，依据此数做踢出处理


    // // http相关
    // context_ = std::make_shared<HttpContext>();
    // ishttp = false;

}

// 回收连接做的事
void ngx_connection_s::PutOneToFree()
{
    // if(precvMemPointer != NULL) // 给连接分配过接受数据的内存，释放内存
    // {
    //     CMemory::getInstance().FreeMemory(precvMemPointer);
    //     precvMemPointer = NULL;       
    // }
    // if(psendMemPointer != NULL) //如果发送数据的缓冲区里有内容，则要释放内存
    // {
    //     CMemory::getInstance().FreeMemory(psendMemPointer);
    //     psendMemPointer = NULL;
    // }

    // iThrowsendCount = 0;

    ++iCurrsequence; // 记录连接的序列号
// 释放资源
    // Socket 相关状态重置
    if(fd != -1)
    {
        close(fd); // 关闭fd
        fd = -1; // 标记fd为-1，表示连接已经关闭
    }
    listening = nullptr;            // 清空监听对象指针

    // 协议控制字段重置
    memset(&s_sockaddr, 0, sizeof(sockaddr));  // 清空对端地址信息

    // Epoll 事件相关清理
    rhandler = nullptr;             // 重置读事件处理器
    whandler = nullptr;             // 重置写事件处理器
    events = 0;                     // 清除事件标记

    // 计时器状态重置
    inRecyTime = {};                // C++11 统一初始化清零时间
    lastPingTime = time_t{0};       // 明确初始化心跳时间

    // 网络安全状态清理
    FloodKickLastTime = 0;          // 重置攻击计时
    FloodAttackCount = 0;           // 清空攻击计数器
    iSendCount = 0;  //重置发送计数

    // HTTP 协议标识重置
    ishttp = false;  // 设置HTTP状态
    context_->reset();
    context_ = nullptr;
   
// 可选：内存缓冲区清理
    //    if (precvMemPointer) {          // 接收缓冲区清理
    //        delete[] precvMemPointer;
    //        precvMemPointer = nullptr;
    //        irecvlen = 0;
    //    }
    //    if (psendMemPointer) {           // 发送缓冲区清理
    //        delete[] psendMemPointer;
    //        psendMemPointer = nullptr;
    //        isendlen = 0;
    //    }

}


// http获得
std::shared_ptr<HttpContext> ngx_connection_s::getContext()
{
    return context_;
}



//---------------------------- 连接池相关函数实现 ----------

// 初始化连接池
void CSocket::initconnection()
{
    // lpngx_connection_t p_Conn;
    // CMemory memory = CMemory::getInstance();

    // int ilenconn = sizeof(ngx_connection_t);
    // for(int i = 0; i < m_worker_connections; i++) // 创建多个连接
    // {
    //     p_Conn = (lpngx_connection_t)memory.AllocMemory(ilenconn, true); // 清理内存，因为这里分配内存new char，无法执行构造函数，所以使用
    //     // 手工调用构造函数，因为AllocMemory无法调用构造函数
    //     p_Conn = new(p_Conn)ngx_connection_t(); 
    //     p_Conn->GetOneToUse(); // 初始化连接
    //     m_connectionList.push_back(p_Conn); // 所有连接都放在这里
    //     m_freeConnectionList.push_back(p_Conn); // 加入空闲连接池

    // }
    // m_free_connection_n = m_total_connection_n = m_connectionList.size(); // 记录当前连接数
    // return;

    // 使用智能指针管理连接对象
    auto create_connection = [] {
        lpngx_connection_t p_Conn = std::make_shared<ngx_connection_s>();;
        // p_Conn->GetOneToUse(-1);
        return p_Conn;
    };

    // 预分配连接对象
    m_connectionList.reserve(m_worker_connections);
    for (int i = 0; i < m_worker_connections; ++i) {
        auto conn = create_connection();
        m_connectionList.emplace_back(conn); // 总连接池
        m_freeConnectionList.emplace_back(conn); // 空闲连接池
    }

    // 更新计数
    m_total_connection_n = m_free_connection_n = m_connectionList.size();

}

// 回收连接池，释放内存
void CSocket::clearconnection()
{
    // lpngx_connection_t p_Conn;
    // CMemory memory = CMemory::getInstance();
    // while(!m_connectionList.empty()) // 遍历连接池，释放内存
    // {
    //     p_Conn = m_connectionList.front();
    //     m_connectionList.pop_front();
    //     p_Conn->~ngx_connection_t(); // 回收连接
    //     memory.FreeMemory(p_Conn); // 释放内存
    // }

    m_connectionList.clear();
    m_freeConnectionList.clear();
    m_total_connection_n = m_free_connection_n = m_connectionList.size();

}

// 从连接池中取出空闲连接【当一个客户端Tcp连接，将这个连接和连接池中的一个连接对象绑定，后序通过连接池的连接，把对象拿到，对象中记录了客户端的信息】
lpngx_connection_t CSocket::ngx_get_connection(int isock)
{
    // //因为可能有其他线程要访问m_freeconnectionList，m_connectionList【比如可能有专门的释放线程要释放/或者主线程要释放】之类的，所以应该临界一下/ 
    // std::lock_guard<std::mutex> lock(m_connectionListMutex); // 临界区
    // if(!m_freeConnectionList.empty())
    // {
    //     // 有空闲连接，从空闲连接池中取出一个连接
    //     lpngx_connection_t p_Conn = m_freeConnectionList.front();
    //     m_freeConnectionList.pop_front();
    //     p_Conn->GetOneToUse();
    //     --m_free_connection_n; // 空闲连接数减1
    //     ++m_onlineUserCount; //用户加1
    //     p_Conn->fd = isock; // 绑定fd
    //     return p_Conn;
    
    // }
    // //没有空闲连接，创建新的连接
    // CMemory memory = CMemory::getInstance();
    // lpngx_connection_t p_Conn = (lpngx_connection_t)memory.AllocMemory(sizeof(ngx_connection_t), true);
    // p_Conn = new(p_Conn)ngx_connection_t();
    // p_Conn->GetOneToUse();
    // m_connectionList.push_back(p_Conn); // 所有连接都放在这里
    // ++m_total_connection_n; // 总连接数加1
    // ++m_onlineUserCount; //用户加1
    // p_Conn->fd = isock; // 绑定fd
    // return p_Conn;


    std::lock_guard<std::mutex> lock(m_connectionListMutex);

    // 连接创建
    auto create_connection = [isock] {
        lpngx_connection_t p_Conn;
        p_Conn->GetOneToUse(isock);
        return p_Conn;
    };

    // 优先从空闲池获取连接
    if (!m_freeConnectionList.empty()) {
        auto conn = m_freeConnectionList.front();
        m_freeConnectionList.pop_front();
        --m_free_connection_n;
        conn->GetOneToUse(isock);
        ++m_onlineUserCount; 
        return conn;
    }

    // 创建新连接
    auto conn = create_connection();
    m_connectionList.emplace_back(conn);
    ++m_total_connection_n; 
    conn->GetOneToUse(isock);
    ++m_onlineUserCount; 
    return conn;
}

// 将连接放回连接池
void CSocket::ngx_free_connection(lpngx_connection_t p_Conn)
{
    std::lock_guard<std::mutex> lock(m_connectionListMutex); // 临界区

    p_Conn->PutOneToFree(); // 释放接受缓冲区和发送缓冲区

    m_freeConnectionList.push_back(p_Conn); // 放回空闲连接池

    ++m_free_connection_n; // 空闲连接数加1

    return;

}

// 将要回收的连接放到一个队列中，后序有专门的线程处理队列中的连接回收
// 有些连接希望过一段时间在释放，将这些连接放到另一个队列
void CSocket::inRecyConnectQueue(lpngx_connection_t p_Conn)
{
    // bool iffind = false;

    // std::lock_guard<std::mutex> lock(m_connectionListMutex); // 临界区
    // //如下判断防止连接被多次扔到回收站中来
    // for(auto it = m_recyconnectionList.begin(); it != m_recyconnectionList.end(); it++)
    // {
    //     if((*it) == p_Conn)
    //     {
    //         iffind = true;
    //         break;
    //     }
    // }
    // if(iffind)
    // {
    //     return;
    // }

    // p_Conn->inRecyTime = time(NULL);
    // ++p_Conn->iCurrsequence;
    // m_recyconnectionList.push_back(p_Conn); // 等待SeverRecyConnectionThread线程会自动处理
    // ++m_total_recyconnection_n;
    // --m_onlineUserCount;
    // ngx_log_stderr(errno,"inRecyConnectQueue   m_recyconnectionList size: %d", m_recyconnectionList.size());

    // return;




    if (!p_Conn) return;  // 空指针检查

    // 存在性检查

    std::lock_guard<std::mutex> lock(m_recyconnqueueMutex);
    auto it = std::find(m_recyconnectionList.begin(), m_recyconnectionList.end(), p_Conn);
    if (it != m_recyconnectionList.end()) {  // 已经存在
        return;
    }
    

    //准备回收数据（无锁区）
    p_Conn->inRecyTime = time(NULL);
    ++p_Conn->iCurrsequence;
    ++m_total_recyconnection_n;
    --m_onlineUserCount;

    // 最终提交
    m_recyconnectionList.push_back(p_Conn);
    Logger::ngx_log_stderr(0, "Recy queue size: %zu", m_recyconnectionList.size());


}

// 处理连接回收的线程函数
void* CSocket::ServerRecyConnectionThread(void* threadData)
{
//     ngx_log_stderr(errno,"ServerRecyConnectionThread");
//     auto pThreadItem = static_cast<ThreadItem*>(threadData);
//     if(pThreadItem->_pThis.lock() == nullptr)
//     {
//         return nullptr;
//     }
//     std::shared_ptr<CSocket> pSocket = pThreadItem->_pThis.lock();
    
//     time_t currtime;
//     int err;
//     std::list<lpngx_connection_t>::iterator pos, posend;
//     lpngx_connection_t p_Conn;

//     while(g_stopEvent == 0)
//     {
//         // 每次直接休息200ms
//         // ngx_log_stderr(errno,"ServerRecyConnectionThread before sleep m_total_connection_n: %d", pSocket->m_total_connection_n.load());
//         usleep(200 * 1000);
//         // g_socket.printTDInfo();
//         if(pSocket->m_total_connection_n > 0)
//         {
//             currtime = time(NULL);
//             std::lock_guard<std::mutex> lock(pSocket->m_recyconnqueueMutex); // 临界区
// lblRRTD:
//             // 遍历连接池，检查是否有连接需要回收
//             pos = pSocket->m_recyconnectionList.begin();
//             posend = pSocket->m_recyconnectionList.end();
//             for(; pos != posend; pos++)
//             {
//                 p_Conn = (*pos);
//                 if((p_Conn->inRecyTime + pSocket->m_RecyConnectionWaitTime > currtime) && (g_stopEvent == 0))
//                     continue;



                
//                 if(p_Conn->iThrowsendCount > 0)
//                 {
//                     //这确实不应该，打印个日志吧；
//                     ngx_log_stderr(0,"CSocekt::ServerRecyConnectionThread()中到释放时间却发现p_Conn.iThrowsendCount!=0，这个不该发生");
//                     //其他先暂时啥也不做，路程继续往下走，继续去释放吧。
//                 }

//                 // 可以释放
//                 --pSocket->m_total_recyconnection_n; // 总回收连接数减1
//                 pSocket->m_recyconnectionList.erase(pos); // 从回收连接池中删除
                
//                 pSocket->ngx_free_connection(p_Conn); // 归还连接
//                 goto lblRRTD; // 继续下一个连接
//             }
            
//         }

//         if(g_stopEvent != 0) // 退出整个程序
//         {
//             if(pSocket->m_total_connection_n > 0)
//             {
//                 // 直接释放所有资源即可
//                 std::lock_guard<std::mutex> lock(pSocket->m_connectionListMutex); // 临界区

//         lblRRTD2:
//                 pos = pSocket->m_connectionList.begin();
//                 posend = pSocket->m_connectionList.end();
//                 for(; pos != posend; pos++)
//                 {
//                     p_Conn = (*pos);
//                     --pSocket->m_total_connection_n; // 总连接数减1
//                     pSocket->m_connectionList.erase(pos); // 从连接池中删除
//                     // pSocket->ngx_free_connection(p_Conn); // 可能有异常
//                     goto lblRRTD2; // 进行下一次
//                 }

//             }   
//             break; // 退出线程
            
//         }


//     }

//     return (void*)0;


        
    auto pThreadItem = static_cast<ThreadItem*>(threadData);
    auto pSocket = pThreadItem->_pThis.lock();
    if (!pSocket) return nullptr;

    std::vector<lpngx_connection_t> batch_cleanup; // 批量处理容器
    batch_cleanup.reserve(64); // 预分配空间

    while (Server::instance().g_stopEvent == 0) {
        // 每次直接休息200ms
        usleep(200 * 1000);

        //定期回收处理
        if (pSocket->m_total_connection_n > 0) {
            auto currtime = time(NULL);
            
            // 批量提取待处理连接
            {
                std::lock_guard<std::mutex> lock(pSocket->m_recyconnqueueMutex);
                auto& recyList = pSocket->m_recyconnectionList;
                
                // 提取过期连接
                auto expired = [&](auto conn) {
                    return conn->inRecyTime + pSocket->m_RecyConnectionWaitTime <= currtime;
                };
                
                std::copy_if(recyList.begin(), recyList.end(), 
                        std::back_inserter(batch_cleanup), expired);
                recyList.erase(std::remove_if(recyList.begin(), recyList.end(), expired), 
                            recyList.end());
            }

            //批量处理过期连接（无锁区）
            for (auto conn : batch_cleanup) {
                // if (conn->iThrowsendCount.load(std::memory_order_relaxed) > 0) {
                //     ngx_log_stderr(0, "Unexpected send count: %d", 
                //                 conn->iThrowsendCount.load());
                // }
                pSocket->ngx_free_connection(conn);
            }
            batch_cleanup.clear();
        }

        // // 程序退出处理
        // if (g_stopEvent != 0) {
        //     std::lock_guard<std::mutex> lock(pSocket->m_connectionListMutex);
            
        //     // 转移所有权
        //     std::vector<lpngx_connection_t> all_conns;
        //     all_conns.reserve(pSocket->m_connectionList.size());
        //     std::move(pSocket->m_connectionList.begin(), 
        //             pSocket->m_connectionList.end(),
        //             std::back_inserter(all_conns));
            
        //     // 并行释放连接（C++17并行算法）
        //     std::for_each(std::execution::par, 
        //                 all_conns.begin(), all_conns.end(),
        //                 [&](auto conn) { pSocket->ngx_free_connection(conn); });
            
        //     break;
        // }
    }
    return nullptr;




}

void CSocket::ngx_close_connection(lpngx_connection_t p_Conn)
{
    if(p_Conn->fd != -1)
    {
        close(p_Conn->fd); // 关闭fd
        p_Conn->fd = -1; // 标记fd为-1，表示连接已经关闭
    }
    ngx_free_connection(p_Conn); // 释放连接资源
}



}