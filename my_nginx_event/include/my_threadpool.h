// TODO: 使用自己实现的线程池

#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <my_memory.h>
#include <list>
#include <vector>
#include "my_func.h"
#include <unistd.h>
#include "my_global.h"
#include <deque>


namespace WYXB
{

class CThreadPool : public std::enable_shared_from_this<CThreadPool>
{
    using Message = std::vector<uint8_t>;
public:
    CThreadPool();

    ~CThreadPool();

public:
    bool Create(int threadNum);
    void StopAll(); // 使线程池中的所有线程停止并退出

    void inMsgRecvQueueAndSignal(std::vector<uint8_t>&& buf);// 收到消息后，将消息入队，并触发线程池中的线程来处理该消息
    void Call(); //唤醒线程

    int getRecvMsgQueueCount(){return m_iRecvMsgQueueCount;} // 获取接收到的消息队列的数量

private:
    static void* ThreadFunc(void* threadData); // 新线程的线程回调函数
    void clearMsgRecvQueue(); // 清空接收到的消息队列


private:
// 定义一个线程的结构
    struct ThreadItem
    {
        std::thread _Thread;
        // pthread_t _Handle; // 线程句柄
        std::weak_ptr <CThreadPool> _pThis; // 记录所属线程池
        bool ifrunning; // 线程是否正常启动，启动起来后，才允许调用stopAll()

        ThreadItem(std::shared_ptr<CThreadPool>& pThis);

        ~ThreadItem();
    };

private:

    std::condition_variable m_cv; // 条件变量
    std::atomic<bool> m_shutdown{false};    // 原子操作保证线程安全

    std::atomic<int> m_iThreadNum; // 线程池中线程的数量

    std::atomic<int> m_iRunningThreadNum{0}; // 正在运行的线程数量
    std::atomic<time_t>  m_iLastEmgTime{0}; // 上次发生线程不够用【紧急事件】的时间，防止日志报的太频繁
    
    // 所有线程线程
    std::mutex m_mutex;
    std::vector<std::shared_ptr<ThreadItem>> m_threadVector;// 线程容器

    // 接受消息队列
    std::mutex m_pthreadMutex; // 互斥锁
    std::deque<Message> m_MsgRecvQueue; // 接收到的消息队列
    std::atomic<int> m_iRecvMsgQueueCount{0}; // 接收到的消息队列的数量
    

};

    
}