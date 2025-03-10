#include "my_socket.h"
#include "my_memory.h"
#include "my_global.h"


namespace WYXB
{

// 和时间相关


// 设置踢出时钟（向multimap中增加内容），用户三次握手成功接入，开启踢人模式【Sock_WaitTimeEnable = 1】，那么本函数被调用
void CSocket::AddToTimmerQueue(lpngx_connection_t pConn)
{ // 设置踢出时钟（向map中增加内容）
    CMemory memory = CMemory::getInstance();

    time_t currTime = time(nullptr); // 获取当前时间
    currTime += m_iWaitTime; // 计算下次踢出时间，需要在指定时间前发送心跳包，否则踢出

    std::lock_guard<std::mutex> lock(m_timequeueMutex); // 互斥锁
    LPSTRUC_MSG_HEADER pMsgHeader = (LPSTRUC_MSG_HEADER)memory.AllocMemory(sizeof(STRUC_MSG_HEADER), false); // 分配内存
    pMsgHeader->pConn = pConn; // 记录连接指针
    pMsgHeader->iCurrsequence = pConn->iCurrsequence; // 记录当前序列号
    m_timeQueuemap.insert(std::make_pair(currTime, pMsgHeader)); // 按键自动排序
    m_cur_size_++; // 记录当前队列大小
    m_timer_value_ = GetEarliestTime(); // 计时队列头部时间值保存到m_timer_value_里


}
time_t CSocket::GetEarliestTime()
{ // 从multimap中获得最早时间返回

    std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos;
    pos = m_timeQueuemap.begin();
    return pos->first; // 返回最早时间

}
LPSTRUC_MSG_HEADER CSocket::RemoveFirstTimer()
{ // 从m_timeQueuemap中移除最早的时间，并把最早时间所指项的值所对应的指针返回，调用者负责互斥
    std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos;
    LPSTRUC_MSG_HEADER pMsgHeader = nullptr;
    if(m_cur_size_ <= 0)
    {
        return NULL;
    }
    pos = m_timeQueuemap.begin();
    pMsgHeader = pos->second; // 获得指针
    m_timeQueuemap.erase(pos); // 从multimap中移除
    m_cur_size_--; // 队列大小减一
    return pMsgHeader; // 返回指针


}
LPSTRUC_MSG_HEADER CSocket::GetOverTimeTimer(time_t currTime)
{ // 根据所给时间，从m_timerQueuemap中找到比这个时间更早的节点【1个】返回，这些节点都是时间超过，需要处理
    CMemory memory = CMemory::getInstance();
    LPSTRUC_MSG_HEADER pMsgHeader = nullptr;

    if(m_cur_size_ == 0 || m_timeQueuemap.empty())
    {
        return NULL;
    }

    time_t earliestTime = GetEarliestTime(); // 获得最早时间
    if(earliestTime <= currTime)
    {
        // 存在超时节点
        pMsgHeader = RemoveFirstTimer(); // 获得指针
        if(m_ifTimeOutKick != 1)
        {
            // 如果不是超时就踢人的情况，则把超时节点重新加入队列，并更新时间
            time_t newTime = currTime + m_iWaitTime; // 计算下次踢出时间
            LPSTRUC_MSG_HEADER tmpMsgHeader = (LPSTRUC_MSG_HEADER)memory.AllocMemory(sizeof(STRUC_MSG_HEADER), false); // 分配内存
            tmpMsgHeader->pConn = pMsgHeader->pConn; // 记录连接指针
            tmpMsgHeader->iCurrsequence = pMsgHeader->iCurrsequence; // 记录当前序列号
            m_timeQueuemap.insert(std::make_pair(newTime, tmpMsgHeader)); // 按键自动排序
            m_cur_size_++; // 记录当前队列大小
        }

        if(m_cur_size_ > 0)
        {
            m_timer_value_ = GetEarliestTime(); // 计时队列头部时间值保存到m_timer_value_里
        }
        return pMsgHeader; // 返回指针
    }
    return NULL; // 没有超时节点
}
void CSocket::DeleteFromTimerQueue(lpngx_connection_t pconn)
{// 把指定用户tcp连接从timer表中去除

    std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos, posend;
    CMemory memory = CMemory::getInstance();
    std::lock_guard<std::mutex> lock(m_timequeueMutex); // 互斥锁

lblMTQM:
    pos = m_timeQueuemap.begin();
    posend = m_timeQueuemap.end();
    for(; pos != posend; pos++)
    {
        if(pos->second->pConn == pconn)
        {
            memory.FreeMemory(pos->second);
            m_timeQueuemap.erase(pos);
            m_cur_size_--;
            goto lblMTQM;
        }
    }
    if(m_cur_size_ > 0)
    {
        m_timer_value_ = GetEarliestTime(); // 计时队列头部时间值保存到m_timer_value_里
    }
}
void CSocket::clearAllFromTimerQueue()
{ // 清理时间队列所有内容
    std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos, posend;
    CMemory memory = CMemory::getInstance();
    pos = m_timeQueuemap.begin();
    posend = m_timeQueuemap.end();
    for(; pos != posend; pos++)
    {
        memory.FreeMemory(pos->second);
        --m_cur_size_;
    }
    m_timeQueuemap.clear();
}

// 时间队列监听和处理线程，处理到期不发心跳包的用户踢出
void* CSocket::ServerTimerQueueMonitorThread(void* threadData)
{
    auto pThreadItem = static_cast<ThreadItem*>(threadData);
    if(pThreadItem->_pThis.lock() == nullptr)
    {
        return nullptr;
    }
    std::shared_ptr<CSocket> pSocket = pThreadItem->_pThis.lock();
    time_t absolute_time, curr_time;
    int err;

    while(g_stopEvent == 0) // 不退出
    {
        if(pSocket->m_cur_size_ > 0) // 队列不为空
        {
            // 将时间队列最早发生事件的时间放到absolute_time里
            absolute_time = pSocket->m_timer_value_;
            curr_time = time(nullptr); // 获取当前时间
            if(curr_time >= absolute_time) // 时间到了
            {
                // 处理超时用户
                std::list<LPSTRUC_MSG_HEADER> lstMsgHeader; //保存要处理的内容
                LPSTRUC_MSG_HEADER result;
                {
                    std::lock_guard<std::mutex> lock(pSocket->m_timequeueMutex); // 互斥锁
                    while((result = pSocket->GetOverTimeTimer(curr_time)) != nullptr)
                    {
                        lstMsgHeader.push_back(result); // 保存要处理的内容
                    }
                }

                LPSTRUC_MSG_HEADER pMsgHeader;
                while (!lstMsgHeader.empty())
                {
                    pMsgHeader = lstMsgHeader.front();
                    lstMsgHeader.pop_front();
                    pSocket->procPingTimeOutChecking(pMsgHeader, curr_time); // 心跳检查超时问题
                }
                
            }
        }
        usleep(500 * 1000);
    }
    return nullptr;
}

// 心跳包检查时间到，检查心跳包是否超时，子类函数需要重新实现该函数的具体判断
void CSocket::procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg,time_t cur_time)
{
	CMemory p_memory = CMemory::getInstance();
	p_memory.FreeMemory(tmpmsg);    
}


}

