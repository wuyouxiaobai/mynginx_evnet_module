#include "my_threadpool.h"


namespace WYXB
{





CThreadPool::CThreadPool()
{
    m_iRunningThreadNum = 0; // 正在运行的线程数
    m_iLastEmgTime = 0; //上次线程不够的时间
    m_iRecvMsgQueueCount = 0; // 接收到的消息队列的数量
}

CThreadPool::~CThreadPool()
{
    //接收消息队列中内容释放
    clearMsgRecvQueue();

}


bool CThreadPool::Create(int threadNum)
{
    ThreadItem* pNew;
    int err;
    m_iThreadNum = threadNum;

    for(int i = 0; i < m_iThreadNum; i++)
    {
        m_threadVector.push_back(pNew = new ThreadItem(this));
        err = pthread_create(&pNew->_Handle, NULL, ThreadFunc, pNew);
        if(err != 0)
        {
            ngx_log_stderr(err,"CThreadPool::Create()创建线程%d失败，返回的错误码为%d!",i,err);
            return false;
        
        } 
        else
        {

        }
    }


    // 需要保证每个线程都启动并运行到pthread_create()返回后才算创建成功
    std::vector<ThreadItem*>::iterator it;
lblfor:
    for(it = m_threadVector.begin(); it != m_threadVector.end(); it++)
    {
        if((*it)->ifrunning == false)
        {
            usleep(100 * 1000); // 100ms
            goto lblfor;
        }
    }

    return true;

}
void CThreadPool::StopAll() // 使线程池中的所有线程停止并退出
{
    if(m_shutdown == true)
        return;

    m_shutdown = true;

    m_cv.notify_all(); // 通知所有等待线程


    std::vector<ThreadItem*>::iterator iter;
	for(iter = m_threadVector.begin(); iter != m_threadVector.end(); iter++)
    {
        pthread_join((*iter)->_Handle, NULL); //等待一个线程终止
    }



    //释放一下new出来的ThreadItem【线程池中的线程】    
	for(iter = m_threadVector.begin(); iter != m_threadVector.end(); iter++)
	{
		if(*iter)
			delete *iter;
        *iter = NULL;
	}
	m_threadVector.clear();

    ngx_log_stderr(0,"CThreadPool::StopAll()成功返回，线程池中线程全部正常结束!");
    return;    


}

void CThreadPool::inMsgRecvQueueAndSignal(char* buf)// 收到消息后，将消息入队，并触发线程池中的线程来处理该消息
{
    {
        std::lock_guard<std::mutex> lock(m_pthreadMutex);
        m_MsgRecvQueue.push_back(buf);
        ++m_iRecvMsgQueueCount;
    }
    
    Call();
    return;
}
void CThreadPool::Call() //唤醒线程
{
    m_cv.notify_one(); // 通知一个等待线程

    if(m_iThreadNum == m_iRunningThreadNum) // 所有线程都在工作，不需要再唤醒其他线程了
    {
        time_t now = time(NULL);
        if(now - m_iLastEmgTime > 10)
        {
            m_iLastEmgTime = now;
            ngx_log_stderr(0,"CThreadPool::Call()中发现线程池中当前空闲线程数量为0，要考虑扩容线程池了!");
        }
    }

    return;
}

//线程入口函数，当用pthread_create()创建线程后，这个ThreadFunc()函数都会被立即执行；
void* CThreadPool::ThreadFunc(void* threadData) // 新线程的线程回调函数
{
    ThreadItem* pThread = (ThreadItem*)threadData;
    CThreadPool* pThreadPool = pThread->_pThis;

    CMemory pMemory = CMemory::getInstance();
    int err;

    pthread_t tid = pthread_self();
    while(true)
    {
        std::unique_lock<std::mutex> lock(pThreadPool->m_pthreadMutex);

        if(pThread->ifrunning == false)
            pThread->ifrunning = true;

        while ((pThreadPool->m_MsgRecvQueue.size() == 0) && (pThreadPool->m_shutdown == false))
        {

            
            pThreadPool->m_cv.wait(lock); // 等待消息队列
        }

        if(pThreadPool->m_shutdown == true)
        {
            lock.unlock();
            break;
        
        }

        // 继续处理【消息队列】中的消息
        char* buf = pThreadPool->m_MsgRecvQueue.front();
        pThreadPool->m_MsgRecvQueue.pop_front();
        --pThreadPool->m_iRecvMsgQueueCount;

        lock.unlock();

        // 处理消息
        ++pThreadPool->m_iRunningThreadNum; // 工作的线程数量+1

        g_socket.threadRecvProFunc(buf); // 处理消息队列中的消息

        pMemory.FreeMemory(buf); // 释放消息队列中的消息
        --pThreadPool->m_iRunningThreadNum; // 工作的线程数量-1

    }

    return nullptr;

}
void CThreadPool::clearMsgRecvQueue() // 清空接收到的消息队列
{
    char* buf = NULL;
    CMemory pMemory = CMemory::getInstance();

    while (!m_MsgRecvQueue.empty())
    {
        /* code */
        buf = m_MsgRecvQueue.front();
        m_MsgRecvQueue.pop_front();
        pMemory.FreeMemory(buf);
    }
    
    // std::lock_guard<std::mutex> lock(m_pthreadMutex);
    // m_cv.notify_all(); // 通知所有等待线程

    // // 等待线程终止
    // std::vector<ThreadItem*>::iterator it;
    // for(it = m_threadVector.begin(); it != m_threadVector.end(); it++)
    // {
    //     pthread_join((*it)->_Handle, NULL); // 等待线程终止
    // }

    // // 到这里时，所有线程池中线程已经全部返回了
    // // 释放ThreadItem对象
    // for(it = m_threadVector.begin(); it != m_threadVector.end(); it++)
    // {
    //     if(*it != NULL)
    //     {
    //         delete *it;
    //         *it = NULL;
    //     }
    // }
    // m_threadVector.clear(); // 清空线程池

    // ngx_log_stderr(0,"CThreadPool::StopAll()成功返回，线程池中线程全部正常结束!");

    // return;
    

}






}