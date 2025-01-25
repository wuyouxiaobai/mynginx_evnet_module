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


namespace WYXB
{

class CThreadPool
{
public:
    CThreadPool();

    ~CThreadPool();

public:
    bool Create(int threadNum);
    void StopAll(); // 使线程池中的所有线程停止并退出

    void inMsgRecvQueueAndSignal(char* buf);// 收到消息后，将消息入队，并触发线程池中的线程来处理该消息
    void Call(); //唤醒线程

    int getRecvMsgQueueCount(){return m_iRecvMsgQueueCount;} // 获取接收到的消息队列的数量

private:
    static void* ThreadFunc(void* threadData); // 新线程的线程回调函数
    void clearMsgRecvQueue(); // 清空接收到的消息队列


private:
// 定义一个线程的结构
    struct ThreadItem
    {
        pthread_t _Handle; // 线程句柄
        CThreadPool* _pThis;// 记录线程池
        bool ifrunning;// 标记是否正式启动

        ThreadItem(CThreadPool* pThis) : _pThis(pThis), ifrunning(false) {}

        ~ThreadItem() {}
    };

private:
    std::mutex m_pthreadMutex; // 互斥锁
    std::condition_variable m_cv; // 条件变量
    bool m_shutdown;// 线程退出标志，false不退出，true退出

    int m_iThreadNum; // 线程池中线程的数量

    std::atomic<int> m_iRunningThreadNum; // 正在运行的线程数量
    time_t  m_iLastEmgTime; // 上次发生线程不够用【紧急事件】的时间，防止日志报的太频繁
    
    std::vector<ThreadItem*> m_threadVector;// 线程容器

    // 接受消息队列
    std::list<char*> m_MsgRecvQueue; // 接收到的消息队列
    int m_iRecvMsgQueueCount; // 接收到的消息队列的数量

};

    
}