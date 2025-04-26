#include "my_socket.h"
#include "my_memory.h"
#include <sys/unistd.h>
#include "my_global.h"
#include <iostream>
#include <string.h>
#include <algorithm>
#include "my_server.h"

namespace WYXB {

// 构造函数：初始化 HttpContext
ngx_connection_s::ngx_connection_s() : context_(std::make_shared<HttpContext>()) {}

// 析构函数：关闭 socket
ngx_connection_s::~ngx_connection_s() {
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}

// 获取一个连接时初始化内容
void ngx_connection_s::GetOneToUse(int sockfd) {
    inRecyTime = {};
    lastPingTime = time_t{0};
    FloodKickLastTime = 0;
    FloodAttackCount = 0;
    iSendCount = 0;
    ishttpClose = true;
    context_->reset();
    sequence = 0;
    ++iCurrsequence;
    fd = sockfd;
}

// 回收连接时清理内容
void ngx_connection_s::PutOneToFree() {
    ++iCurrsequence;
}

// 获取 HttpContext
std::shared_ptr<HttpContext> ngx_connection_s::getContext() {
    return context_;
}

// 初始化连接池
void CSocket::initconnection() {
    for (int i = 0; i < m_worker_connections; ++i) {
        lpngx_connection_t p_Conn = std::make_shared<ngx_connection_s>();
        p_Conn->GetOneToUse(-1);
        p_Conn->id = i;
        m_connectionList.emplace_back(p_Conn);
        m_freeConnectionList.emplace_back(p_Conn);
    }
    m_total_connection_n = m_free_connection_n = m_connectionList.size();
}

// 清理连接池
void CSocket::clearconnection() {
    m_connectionList.clear();
    m_freeConnectionList.clear();
    m_total_connection_n = m_free_connection_n = 0;
}

// 获取空闲连接
lpngx_connection_t CSocket::ngx_get_connection(int isock) {
    std::lock_guard<std::mutex> lock(m_connectionListMutex);
    do {
        auto create_connection = [&] {
            for (int i = 0; i < m_worker_connections; ++i) {
                lpngx_connection_t p_Conn = std::make_shared<ngx_connection_s>();
                p_Conn->GetOneToUse(-1);
                p_Conn->id = i + m_total_connection_n;
                m_connectionList.emplace_back(p_Conn);
                m_freeConnectionList.emplace_back(p_Conn);
                ++m_free_connection_n;
            }
            m_total_connection_n = m_connectionList.size();
        };

        if (!m_freeConnectionList.empty()) {
            auto conn = m_freeConnectionList.front().lock();
            m_freeConnectionList.pop_front();
            --m_free_connection_n;
            conn->GetOneToUse(isock);
            ++m_onlineUserCount;
            return conn;
        }
        create_connection();
    } while (true);
}

// 释放连接
void CSocket::ngx_free_connection(std::weak_ptr<ngx_connection_s> p_Conn) {
    std::lock_guard<std::mutex> lock(m_connectionListMutex);
    auto pConn = p_Conn.lock();
    pConn->PutOneToFree();
    m_freeConnectionList.push_back(p_Conn);
    ++m_free_connection_n;
    Logger::ngx_log_stderr(0, "m_free_connection_n: %d", m_freeConnectionList.size());
}

// 放入回收连接队列
void CSocket::inRecyConnectQueue(lpngx_connection_t p_Conn) {
    if (!p_Conn) return;
    {
        std::lock_guard<std::mutex> lock(m_recyconnqueueMutex);
        auto it = std::find_if(m_recyconnectionList.begin(), m_recyconnectionList.end(), [&p_Conn](const std::weak_ptr<ngx_connection_s>& weak_conn) {
            auto existing_conn = weak_conn.lock();
            return existing_conn && existing_conn->id == p_Conn->id;
        });
        if (it != m_recyconnectionList.end()) {
            Logger::ngx_log_stderr(0, "Connection already in Recy queue, size: %u", m_recyconnectionList.size());
            return;
        }
        p_Conn->inRecyTime = std::time(nullptr);
        ++p_Conn->iCurrsequence;
        std::atomic_fetch_sub(&m_total_recyconnection_n, 1);
        std::atomic_fetch_sub(&m_onlineUserCount, 1);
        m_recyconnectionList.push_back(p_Conn);
        Logger::ngx_log_stderr(0, "Recy queue size: %u", m_recyconnectionList.size());
    }
}

// 回收连接线程函数
void* CSocket::ServerRecyConnectionThread(void* threadData) {
    Logger::ngx_log_stderr(errno, "ServerRecyConnectionThread...............");
    auto pThreadItem = static_cast<ThreadItem*>(threadData);
    auto pSocket = pThreadItem->_pThis.lock();
    if (!pSocket) return nullptr;

    while (Server::instance().g_stopEvent == 0) {
        usleep(200 * 1000);
        if (pSocket->m_total_connection_n > 0) {
            auto currtime = std::time(nullptr);
            {
                std::lock_guard<std::mutex> lock(pSocket->m_recyconnqueueMutex);
                for (auto it = pSocket->m_recyconnectionList.begin(); it != pSocket->m_recyconnectionList.end(); ) {
                    auto conn = it->lock();
                    if (!conn || (conn->inRecyTime + pSocket->m_RecyConnectionWaitTime <= currtime)) {
                        Logger::ngx_log_stderr(0, "test ,,, m_free_connection_n: ");
                        pSocket->ngx_free_connection(*it);
                        Logger::ngx_log_stderr(0, "m_recyconnectionList: %d", pSocket->m_recyconnectionList.size());
                        it = pSocket->m_recyconnectionList.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }
    }
    return nullptr;
}

// 关闭连接封装函数
void CSocket::ngx_close_connection(lpngx_connection_t p_Conn) {
    if (p_Conn->fd != -1) {
        close(p_Conn->fd);
        p_Conn->fd = -1;
    }
    ngx_free_connection(p_Conn);
}

} // namespace WYXB
