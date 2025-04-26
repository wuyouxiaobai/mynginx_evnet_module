#include "my_socket.h"
#include "my_memory.h"
#include "my_global.h"
#include <unistd.h>
#include "my_server.h"

namespace WYXB {

// 设置踢出时钟，将用户连接加入定时器队列
void CSocket::AddToTimmerQueue(lpngx_connection_t pConn) {
    const auto expiration = time(nullptr) + m_iWaitTime;
    auto pMsgHeader = std::make_shared<STRUC_MSG_HEADER>();
    pMsgHeader->pConn = pConn;
    pMsgHeader->iCurrsequence = pConn->iCurrsequence;

    {
        std::lock_guard<std::mutex> lock(m_timequeueMutex);
        m_timeQueuemap.emplace(expiration, pMsgHeader);
        m_cur_size_++;
        m_timer_value_ = GetEarliestTime();
    }
}

// 获取当前时间队列中最早的过期时间
time_t CSocket::GetEarliestTime() {
    return !m_timeQueuemap.empty() ? m_timeQueuemap.begin()->first : 0;
}

// 移除时间队列中最早的元素并返回对应的消息头
LPSTRUC_MSG_HEADER CSocket::RemoveFirstTimer() {
    if (m_cur_size_ <= 0) {
        return nullptr;
    }
    auto pos = m_timeQueuemap.begin();
    LPSTRUC_MSG_HEADER pMsgHeader = pos->second;
    m_timeQueuemap.erase(pos);
    m_cur_size_--;
    return pMsgHeader;
}

// 获取超时的定时器，如果存在则取出
LPSTRUC_MSG_HEADER CSocket::GetOverTimeTimer(time_t currTime) {
    LPSTRUC_MSG_HEADER pMsgHeader;
    {
        std::lock_guard<std::mutex> lock(m_timequeueMutex);
        if (m_timeQueuemap.empty() || GetEarliestTime() > currTime) {
            return nullptr;
        }

        pMsgHeader = RemoveFirstTimer();

        if (!m_ifTimeOutKick) {
            LPSTRUC_MSG_HEADER newHeader = pMsgHeader;
            m_timeQueuemap.emplace(currTime + m_iWaitTime, newHeader);
            m_cur_size_++;
        }

        m_timer_value_ = m_timeQueuemap.empty() ? 0 : GetEarliestTime();
    }
    return pMsgHeader;
}

// 从时间队列中删除指定连接
void CSocket::DeleteFromTimerQueue(lpngx_connection_t pconn) {
    std::lock_guard<std::mutex> lock(m_timequeueMutex);
    for (auto it = m_timeQueuemap.begin(); it != m_timeQueuemap.end(); ) {
        if (it->second->pConn == pconn) {
            it = m_timeQueuemap.erase(it);
            m_cur_size_--;
        } else {
            ++it;
        }
    }
    m_timer_value_ = GetEarliestTime();
}

// 清空整个时间队列
void CSocket::clearAllFromTimerQueue() {
    std::lock_guard<std::mutex> lock(m_timequeueMutex);
    m_timeQueuemap.clear();
    m_cur_size_ = 0;
}

// 连接超时监控线程
void* CSocket::ServerTimerQueueMonitorThread(void* threadData) {
    auto pThreadItem = static_cast<ThreadItem*>(threadData);
    auto pSocket = pThreadItem->_pThis.lock();
    if (!pSocket) return nullptr;

    while (Server::instance().g_stopEvent == 0) {
        if (pSocket->m_cur_size_ > 0) {
            time_t absolute_time = pSocket->m_timer_value_;
            time_t curr_time = time(nullptr);

            if (curr_time >= absolute_time) {
                std::list<LPSTRUC_MSG_HEADER> lstMsgHeader;
                LPSTRUC_MSG_HEADER result;

                while ((result = pSocket->GetOverTimeTimer(curr_time)) != nullptr) {
                    std::lock_guard<std::mutex> lock(pSocket->m_timequeueMutex);
                    lstMsgHeader.push_back(result);
                }

                while (!lstMsgHeader.empty()) {
                    auto pMsgHeader = lstMsgHeader.front();
                    lstMsgHeader.pop_front();
                    pSocket->procPingTimeOutChecking(pMsgHeader, curr_time);
                }
            }
        }
        usleep(500 * 1000);
    }
    return nullptr;
}

// 超时心跳检测处理（子类可自定义）
void CSocket::procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t cur_time) {
    // 默认空实现，可被子类重写
}

} // namespace WYXB
