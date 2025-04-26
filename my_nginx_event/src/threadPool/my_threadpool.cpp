#include "my_threadpool.h"
#include "my_server.h"

#include <algorithm>
#include <thread>
#include <utility>

namespace WYXB {

void CThreadPool::ThreadItem::tie(std::shared_ptr<CThreadPool> pThis) {
    _pThis = pThis;
}

CThreadPool::ThreadItem::ThreadItem() : ifrunning(false) {}
CThreadPool::ThreadItem::~ThreadItem() {}

CThreadPool::CThreadPool() {}

CThreadPool::~CThreadPool() {
    clearMsgRecvQueue();
}

bool CThreadPool::Create(int threadNum) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_iThreadNum = threadNum;
    m_threadVector.reserve(threadNum);

    try {
        for (int i = 0; i < m_iThreadNum; ++i) {
            auto pItem = std::make_unique<ThreadItem>();
            pItem->tie(shared_from_this());
            pItem->_Thread = std::thread([pItem = pItem.get()] {
                ThreadFunc(pItem);
            });
            m_threadVector.push_back(std::move(pItem));
        }
    } catch (const std::exception& e) {
        Logger::ngx_log_stderr(0, "线程创建失败: %s", e.what());
        return false;
    }

    m_cv_init.wait(lock, [this] {
        return std::all_of(m_threadVector.begin(), m_threadVector.end(),
            [](const auto& item) { return item->ifrunning; });
    });

    return true;
}

void CThreadPool::StopAll() {
    if (m_shutdown.exchange(true)) {
        return;
    }

    m_cv.notify_all();

    for (auto& pItem : m_threadVector) {
        if (pItem && pItem->_Thread.joinable()) {
            try {
                pItem->_Thread.join();
            } catch (const std::exception& e) {
                Logger::ngx_log_stderr(0, "线程等待异常: %s", e.what());
            }
        }
    }

    m_threadVector.clear();

    Logger::ngx_log_stderr(0, "CThreadPool::StopAll() 成功，线程池已关闭!");
}

void CThreadPool::inMsgRecvQueueAndSignal(STRUC_MSG_HEADER msghead, std::vector<uint8_t> buf) {
    m_MsgRecvQueue.Enqueue(std::pair<STRUC_MSG_HEADER, std::vector<uint8_t>>(msghead, buf));
    ++m_iRecvMsgQueueCount;
    Call();
}

void CThreadPool::Call() {
    m_cv.notify_one();

    if (m_iThreadNum == m_iRunningThreadNum) {
        time_t now = time(NULL);
        if (now - m_iLastEmgTime > 10) {
            m_iLastEmgTime = now;
            Logger::ngx_log_stderr(0, "CThreadPool::Call()中发现线程池中当前空闲线程数量为0，要考虑扩容线程池了!");
        }
    }
}

void* CThreadPool::ThreadFunc(void* threadData) {
    auto pThread = static_cast<ThreadItem*>(threadData);
    if (pThread->_pThis.expired()) {
        Logger::ngx_log_stderr(0, "CThreadPool::ThreadFunc() 中发现 _pThis 已过期!");
        return nullptr;
    }

    auto pThreadPool = pThread->_pThis.lock();
    std::atomic_bool& shutdown = pThreadPool->m_shutdown;

    while (!shutdown.load(std::memory_order_acquire)) {
        Message buf;
        STRUC_MSG_HEADER msghead;

        if (pThread->ifrunning == false) {
            pThread->ifrunning = true;
            pThreadPool->m_cv_init.notify_one();
        }

        if (shutdown) break;

        std::pair<STRUC_MSG_HEADER, std::vector<uint8_t>> Node;
        if (!pThreadPool->m_MsgRecvQueue.TryDequeue(Node)) continue;
        buf = Node.second;
        msghead = Node.first;
        pThreadPool->m_iRecvMsgQueueCount.fetch_sub(1, std::memory_order_relaxed);

        pThreadPool->m_iRunningThreadNum.fetch_add(1, std::memory_order_relaxed);

        try {
            if (!buf.empty()) {
                Server::instance().g_socket->threadRecvProcFunc(msghead, buf);
            }
        } catch (...) {
            Logger::ngx_log_stderr(0, "异常处理.");
        }

        pThreadPool->m_iRunningThreadNum.fetch_sub(1, std::memory_order_release);
    }

    return nullptr;
}

void CThreadPool::clearMsgRecvQueue() {
    // 队列清空后，由系统回收资源
}

} // namespace WYXB
