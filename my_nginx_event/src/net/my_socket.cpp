#include "my_socket.h"
#include "my_conf.h"
#include <arpa/inet.h>
#include <cstring>
#include "my_func.h"
#include <unistd.h>
#include "my_macro.h"
#include <sys/ioctl.h>
#include "my_memory.h"
#include <sys/time.h>
#include "my_global.h"
#include <thread>
#include <iostream>
#include <string_view>
#include "my_server.h"


namespace WYXB
{
void CSocket::ThreadItem::tie(std::shared_ptr<CSocket> pThis)
{
    _pThis = pThis;
}
CSocket::ThreadItem::ThreadItem(){}
CSocket::ThreadItem::~ThreadItem(){}

CSocket::CSocket()
{
    // 配置相关
    m_worker_connections = 1; // epoll连接最大项数
    m_ListenPortCount = 1; // 监听一个端口
    m_RecyConnectionWaitTime = 600; //等待60s后才会回收
    
    //epoll相关
    m_epollhandle = -1; // epoll返回的句柄

    // 和网络通信相关的变量
    m_iLenMsgHeader = sizeof(STRUC_MSG_HEADER); // 消息头的长度
    // m_iLenPkgHeader = sizeof(COMM_PKG_HEADER); // 包头的长度

    // 各种队列相关
    m_iSendMsgQueueCount = 0; // 发送消息队列大小
    m_total_recyconnection_n = 0;// 待释放连接队列大小
    m_cur_size_ = 0;// 当前计时队列尺寸
    m_timer_value_ = 0; // 当前计时队列头部时间值
    m_iDiscardSendPkgCount = 0;//丢弃的发送数据包数量

    // 在线用户相关
    m_onlineUserCount = 0; // 当前在线用户数量
    m_lastprintTime = 0; // 上次打印时间


}

CSocket::~CSocket()
{
    // 监听端口相关内存释放
    // std::vector<lpngx_listening_t>::iterator pos;
    // for(pos = m_ListenSocketList.begin(); pos !=m_ListenSocketList.end(); pos++)
    // {
    //     delete(*pos);
    // }
    // ngx_close_listening_sockets();
    m_ListenSocketList.clear();
    
}


// 初始化函数，【fork子线程之前做的】
//成功返回true，失败返回false
bool CSocket::Initialize()
{
    ReadConf();
    // m_httpGetProcessor = std::make_shared<HttpGetProcessor>();
    // if(ngx_open_listening_sockets() == false)
    // {
    //     return false;
    // }
    return true;
}

void CSocket::ReadConf()
{
    MyConf* config = MyConf::getInstance();
    m_worker_connections = config->GetIntDefault("worker_connections", m_worker_connections); // epoll连接的最大项数
    m_ListenPortCount = config->GetIntDefault("ListenPortCount", m_ListenPortCount); // 取得要监听的端口数量
    m_RecyConnectionWaitTime = config->GetIntDefault("Sock_RecyConnectionWaitTime", m_RecyConnectionWaitTime); // 等待60s后才会回收

    m_ifkickTimeCount = config->GetIntDefault("Sock_WaitTimeEnable", 0); // 是否开启踢人时钟
    m_iWaitTime = config->GetIntDefault("Sock_MaxWaitTime", m_iWaitTime); // 多少次检测是否心跳超时
    m_iWaitTime = (m_iWaitTime > 5)?m_iWaitTime:5; // 不建议低于5秒
    m_ifTimeOutKick = config->GetIntDefault("Sock_TimeOutKick", 0); // 当时间到达Sock_TimeOutKick指定时间，直接踢出

    m_floodAkEnable = config->GetIntDefault("Sock_FloodAttackKickEnable", 0); // Flood攻击检测是否开启
    m_floodTimeInterval = config->GetIntDefault("Sock_FloodTimeInterval", 100) ; // 每次收到数据包时间间隔100ms
    m_floodKickCount = config->GetIntDefault("Sock_FloodKickCounter", 5) ; // 收到攻击的上限，超过就会踢人


}


// 监听端口【支持多个端口】，与nginx命名一样
// 在创建worker进程之前就创建好
lpngx_listening_t CSocket::ngx_open_listening_sockets()
{
    int isock; // socket
    struct sockaddr_in serv_addr; // 服务器地址
    int iport; // 端口号
    // char strinfo[100]; // 临时字符串
    
    // 初始化相关
    memset(&serv_addr, 0, sizeof(serv_addr)); // 先初始一下
    serv_addr.sin_family = AF_INET; // 地址族为IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);// 监听本地所有ip地址
    
    // 中途用的配置信息
    // MyConf* config = MyConf::getInstance();
    // int WorkerProcesses = config->GetIntDefault("WorkerProcesses", 1);
    // for(int i = 0; i < WorkerProcesses; i++) // 创建多个套接字，监听同一个端口
    // {
    isock = socket(AF_INET, SOCK_STREAM, 0); // 创建socket
    if(isock == -1)
    {
        Logger::ngx_log_stderr(errno, "CSocket::Initialize()中socket()失败.");
        return nullptr;
    }

    int reuseaddr = 1; 
    if(setsockopt(isock,SOL_SOCKET,SO_REUSEADDR,(const void*)&reuseaddr, sizeof(reuseaddr)) == -1)
    {
        Logger::ngx_log_stderr(errno,"CSocket::Initialize()中setsockopt(SO_REUSEADDR)失败.");
        return nullptr;
    }

    //为处理惊群问题使用reuseport
    int reuseport = 1;
    if(setsockopt(isock, SOL_SOCKET, SO_REUSEPORT,(const void*)&reuseport, sizeof(int))==-1)
    {
        Logger::ngx_log_stderr(errno,"CSocket::Initialize()中setsockopt(SO_REUSEPORT)失败.");
    }


    //设置为非阻塞
    if(setnonblocking(isock) == false)
    {
        Logger::ngx_log_stderr(errno,"CSocket::Initialize()中setnonblocking()失败.");
        close(isock);
        return nullptr;
    
    }

    // 设置本服务器要监听的地址和端口，这样客户端连接
    // strinfo[0] = 0;
    // sprintf(strinfo, "ListenPort%d", 0);
    MyConf* config = MyConf::getInstance();
    iport = config->GetIntDefault("ListenPort", 10000); // 取得端口号
    serv_addr.sin_port = htons((in_port_t)iport);

    //绑定服务器地址结构
    if(bind(isock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
        Logger::ngx_log_stderr(errno,"CSocket::Initialize()中bind()失败.");
        close(isock);
        return nullptr;
    }

    // 监听端口
    if(listen(isock, NGX_LISTEN_BACKLOG) == -1)
    {
        Logger::ngx_log_stderr(errno,"CSocket::Initialize()中listen()失败.");
        close(isock);
        return nullptr;
    }

    // 可以，放在列表中
    lpngx_listening_t  p_listensocketitem = std::make_shared<ngx_listening_s>();
    // memset(p_listensocketitem, 0, sizeof(ngx_listening_s));
    p_listensocketitem->port = iport;
    p_listensocketitem->fd = isock;
    Logger::ngx_log_error_core(NGX_LOG_INFO, 0, "监听端口%d成功",iport);
    m_ListenSocketList.push_back(p_listensocketitem);



    // }
    // if(m_ListenSocketList.size() <= 0)
    //     return false;
    return p_listensocketitem;

}


bool CSocket::setnonblocking(int sockfd)
{
    int nb = 1; //0: 清除，1：设置
    if(ioctl(sockfd, FIONBIO, &nb) == -1)
    {
        return false;
    }
    return true;

}


//子进程中执行的初始化函数
bool CSocket::Initialize_subproc()
{
    Logger::ngx_log_stderr(0,"Initialize_subproc");
    //     //发消息互斥量初始化
    // if(pthread_mutex_init(&m_sendMessageQueueMutex, NULL)  != 0)
    // {        
    //     ngx_log_stderr(0,"CSocket::Initialize_subproc()中pthread_mutex_init(&m_sendMessageQueueMutex)失败.");
    //     return false;    
    // }
    // //连接相关互斥量初始化
    // if(pthread_mutex_init(&m_connectionMutex, NULL)  != 0)
    // {
    //     ngx_log_stderr(0,"CSocket::Initialize_subproc()中pthread_mutex_init(&m_connectionMutex)失败.");
    //     return false;    
    // }    
    // //连接回收队列相关互斥量初始化
    // if(pthread_mutex_init(&m_recyconnqueueMutex, NULL)  != 0)
    // {
    //     ngx_log_stderr(0,"CSocket::Initialize_subproc()中pthread_mutex_init(&m_recyconnqueueMutex)失败.");
    //     return false;    
    // } 
    // //和时间处理队列有关的互斥量初始化
    // if(pthread_mutex_init(&m_timequeueMutex, NULL)  != 0)
    // {
    //     ngx_log_stderr(0,"CSocket::Initialize_subproc()中pthread_mutex_init(&m_timequeueMutex)失败.");
    //     return false;    
    // }
// 创建发送线程池
MyConf* config = MyConf::getInstance(); //初始化配置文件
int tmpSendThread = config->GetIntDefault("ProcMsgSendWorkThreadCount", 5);
    if(sem_init(&m_semEventSendQueue,0,0) == -1)
    {
        Logger::ngx_log_stderr(0,"CSocket::Initialize_subproc()中sem_init(&m_semEventSendQueue_empty,0,0)失败.");
        return false;
    }


for(int i = 0; i < tmpSendThread; i++)
{
    auto pSendThread = std::make_shared<ThreadItem>(); //专门用来发送数据的线程
    pSendThread->tie(shared_from_this());
    // 使用 lambda 捕获 shared_ptr，确保线程内对象存活
    pSendThread->_Thread = std::thread([pSendThread]() {
        ServerSendQueueThread(pSendThread.get());  // 传递原始指针或智能指针
    });
    m_threadVector.push_back(std::move(pSendThread)); //创建新线程并存入线程池
}


    // ThreadItem* pRecyconn; //专门用来回收连接的线程
    // m_threadVector.push_back(pRecyconn = new ThreadItem(this)); //创建新线程并存入
    // err = pthread_create(&pRecyconn->_Handle, NULL, ServerRecyConnectionThread, pRecyconn);
    // if(err != 0)
    // {
    //     ngx_log_stderr(0,"CSocket::Initialize_subproc()中pthread_create(ServerRecyConnectionThread)失败.");
    //     return false;
    // }

// 创建回收线程
    auto pRecyThread = std::make_shared<ThreadItem>(); //专门用来发送数据的线程
    pRecyThread->tie(shared_from_this());
    pRecyThread->_Thread = std::thread([pRecyThread]() {
        ServerRecyConnectionThread(pRecyThread.get());  // 传递原始指针或智能指针
    });
    m_threadVector.push_back(std::move(pRecyThread)); //创建新线程并存入
    // if (!pRecyconn) {
    //     ngx_log_stderr(0, "CSocket::Initialize_subproc()中new ThreadItem失败.");
    //     return false;
    // }
    // // 创建线程
    // try {
    //     pRecyconn->_Thread = std::thread(ServerRecyConnectionThread, pRecyconn);
    // } catch (const std::exception& e) {
    //     ngx_log_stderr(0, "CSocket::Initialize_subproc()中std::thread创建失败: %s", e.what());
    //     delete pRecyconn; // 释放内存
    //     return false;
    // }
    // // 将线程对象存入容器
    // m_threadVector.push_back(pRecyconn);


    if(m_ifkickTimeCount == 1)  //是否开启踢人时钟，1：开启   0：不开启
    {
// 创建超时踢人线程
        auto pTimemonitor = std::make_shared<ThreadItem>(); //专门用来发送数据的线程
        pTimemonitor->tie(shared_from_this());
        pTimemonitor->_Thread = std::thread([pTimemonitor]() {
            ServerTimerQueueMonitorThread(pTimemonitor.get());  // 传递原始指针或智能指针
        });
        m_threadVector.push_back(std::move(pTimemonitor)); //创建新线程并存入
        // std::shared_ptr<ThreadItem> pTimemonitor = std::make_shared<ThreadItem>(shared_from_this()); // 专门用来处理到时间不发心跳包的用户踢出
        // // 传递裸指针到线程，但需手动管理生命周期
        // void* arg = new std::shared_ptr<ThreadItem>(pTimemonitor);  // 堆上创建 shared_ptr 的拷贝

        // err = pthread_create(&pTimemonitor->_Handle,NULL, ServerTimerQueueMonitorThread,arg);
        // if(err != 0)
        // {
        //     ngx_log_stderr(0,"CSocket::Initialize_subproc()中pthread_create(ServerTimerQueueMonitorThread)失败.");
        //     return false;
        // }
        // m_threadVector.push_back(pTimemonitor ); //创建新线程并存入
    }

    return true;
}

// 关闭退出函数【工作进程中执行】
void CSocket::Shotdown_subproc()
{
    // 1. 发送信号量通知线程退出
    MyConf* config = MyConf::getInstance(); //初始化配置文件
    int tmpSendThread = config->GetIntDefault("ProcMsgSendWorkThreadCount", 5);
    for(int i = 0; i < tmpSendThread; i++)
    {
        if (sem_post(&m_semEventSendQueue) == -1) {
            Logger::ngx_log_stderr(0, "CSocket::Shutdown_subproc()中sem_post m_semEventSendQueue_empty 失败: errno=%d", errno);
        }
    }


    // 2. 安全等待所有线程退出（RAII 管理）
    for (auto& pItem : m_threadVector) {
        if (pItem->_Thread.joinable()) {
            try {
                pItem->_Thread.join();
            } catch (const std::exception& e) {
                Logger::ngx_log_stderr(0, "线程等待失败: %s", e.what());
            }
        }
    }

    // 3. 自动释放内存（unique_ptr 管理）
    m_threadVector.clear();  // unique_ptr 自动析构

    // 4. 清理其他资源
    clearMsgSendQueue();
    clearconnection();
    clearAllFromTimerQueue();


}

//清理Tcp发送队列
void CSocket::clearMsgSendQueue()
{	 

}

// 关闭Socket
void CSocket::ngx_close_listening_sockets()
{
    // MyConf* config = MyConf::getInstance();
    // int WorkerProcesses = config->GetIntDefault("WorkerProcesses", 1);
    // for(int i = 0; i < m_ListenPortCount; i++) //关闭多个套接字
    // {  
    //     //ngx_log_stderr(0,"端口是%d,socketid是%d.",m_ListenSocketList[i]->port,m_ListenSocketList[i]->fd);
    //     close(m_ListenSocketList[i]->fd);
    //     Logger::ngx_log_error_core(NGX_LOG_INFO, 0,"关闭监听端口%d!",m_ListenSocketList[i]->port); //显示一些信息到日志中
    // }//end for(int i = 0; i < m_ListenPortCount; i++)
    // return;
}

// 将一个待发送消息写入发送队列中
void CSocket::msgSend(std::string psendbuf, lpngx_connection_t pConn)
{
    // std::lock_guard<std::mutex> lock(m_sendMessageQueueMutex);
    
    if (m_MsgSendQueue.size > 50000) {
        ++m_iDiscardSendPkgCount;
        // 可以记录日志，提示队列已满并丢弃了消息
        Logger::ngx_log_stderr(0, "CSocket::msgSend() 发送队列已满，丢弃了一条消息.");
        return;
    }

    // 检查发送计数
    if (pConn->iSendCount > 400) {
        Logger::ngx_log_stderr(0, "CSocket::msgSend()中发现某用户%d积压了大量待发送数据包，切断与他的连接！", pConn->fd);   
        ++m_iDiscardSendPkgCount;

        // 关闭连接
        zdClosesocketProc(pConn);
        return;
    }

    // 增加发送计数
    ++pConn->iSendCount;

    // 创建消息头
    STRUC_MSG_HEADER msgHeader;
    msgHeader.pConn = pConn;
    msgHeader.iCurrsequence = pConn->iCurrsequence;


    // 加入发送队列
    m_MsgSendQueue.Enqueue(std::pair(msgHeader, std::vector<uint8_t>(psendbuf.begin(), psendbuf.end())));
    ++m_iSendMsgQueueCount;

    // 发送信号量通知发送线程
    if (sem_post(&m_semEventSendQueue) == -1) {
        Logger::ngx_log_stderr(0, "CSocket::msgSend()中sem_post(&m_semEventSendQueue)失败: %s", std::strerror(errno));
    }



}

// 主动发送一个连接时要做到善后操作
// 这个函数可能被多线程调用
void CSocket::zdClosesocketProc(lpngx_connection_t p_Conn)
{

    if(m_ifkickTimeCount == 1) //是否开启踢人时钟，1：开启   0：不开启
    {
        DeleteFromTimerQueue(p_Conn); //从计时队列中删除该连接
    }
    if(p_Conn->fd != -1)
    {
        // 从 epoll 中删除文件描述符 fd
        ngx_epoll_oper_event(p_Conn->fd, EPOLL_CTL_DEL, 0, 0, p_Conn.get());
        close(p_Conn->fd); //关闭socket后，epoll就会被从红黑树中删除，之后就无法收到任何epoll事件
        p_Conn->fd = -1; // 标记该连接已经关闭
    }

    inRecyConnectQueue(p_Conn); // 加入待回收连接队列
    return;
}

// 测试是否被洪水攻击
bool CSocket::TestFlood(lpngx_connection_t pConn) // 测试是否受到flood攻击
{
    struct timeval sCurrTime;//当前时间结构
    uint16_t iCurrTime;// 当前时间
    bool reco = false; // 是否受到攻击

    gettimeofday(&sCurrTime, NULL); // 获取当前时间
    iCurrTime = sCurrTime.tv_sec * 1000 + sCurrTime.tv_usec / 1000; // 转换成毫秒
    if((iCurrTime - pConn->FloodKickLastTime) < m_floodTimeInterval) // 两次收到包的时间 < 100ms
    {
        // 发包太频繁
        pConn->FloodAttackCount++; // 累计次数
        pConn->FloodKickLastTime = iCurrTime; // 更新时间
    }
    else
    {
        //发包时间不频繁
        pConn->FloodAttackCount = 0; // 重置次数
        pConn->FloodKickLastTime = iCurrTime; // 更新时间
    
    }

    if(pConn->FloodAttackCount >= m_floodKickCount)
    {
        // 可以踢人
        reco = true;
    }
    return reco;

}

// 打印统计信息
void CSocket::printTDInfo()
{
    time_t currtime = time(NULL);
    if(currtime - m_lastprintTime > 10)
    {
        // 超过10s打印一次
        int tmprmqc = Server::instance().g_threadpool->getRecvMsgQueueCount(); 

        m_lastprintTime = currtime;
        int tmpoLUC = m_onlineUserCount;
        int tmpsmqc = m_iSendMsgQueueCount;
        Logger::ngx_log_stderr(0,"------------------------------------begin--------------------------------------");
        Logger::ngx_log_stderr(0,"当前在线人数/总人数(%d/%d)。",tmpoLUC,m_worker_connections);        
        Logger::ngx_log_stderr(0,"连接池中空闲连接/总连接/要释放的连接(%d/%d/%d)。",m_freeConnectionList.size(),m_connectionList.size(),m_recyconnectionList.size());
        Logger::ngx_log_stderr(0,"当前时间队列大小(%d)。",m_timeQueuemap.size());        
        Logger::ngx_log_stderr(0,"当前收消息队列/发消息队列大小分别为(%d/%d)，丢弃的待发送数据包数量为%d。",tmprmqc,tmpsmqc,m_iDiscardSendPkgCount);   

        if(tmprmqc > 100000)
        {
            // 接收队列过大，提醒一下
            Logger::ngx_log_stderr(0,"接收队列条目数量过大(%d)，要考虑限速或者增加处理线程数量了！！！！！！",tmprmqc);
        }
        Logger::ngx_log_stderr(0,"------------------------------------end--------------------------------------");

    }

    return;

}










// epoll相关的-------------------------

//epoll功能初始化，子进程中进行，本函数被ngx_worker_process_init()所调用
int CSocket::ngx_epoll_init()// epoll初始化
{
    //创建epoll对象，创建一个红黑树，创建一个双向链表
    m_epollhandle = epoll_create(m_worker_connections); //最大连接数为m_worker_connections
    if(m_epollhandle == -1)
    {
        Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_init()中epoll_create()失败.");
        exit(2); //这是致命问题了，直接退，资源由系统释放吧，这里不刻意释放了，比较麻烦
    }

    // 创建连接池【数组】，用于处理后续客户端连接
    initconnection();

    // 遍历所有监听socket【监听端口】，为每一个socket分配一个连接池中的连接
    // std::vector<lpngx_listening_t>::iterator iter;


    lpngx_listening_t listening = ngx_open_listening_sockets();

    // for(iter = m_ListenSocketList.begin(); iter != m_ListenSocketList.end(); iter++)
    // {
    lpngx_connection_t pConn = ngx_get_connection(listening->fd); // 从连接池中取出一个空闲连接
    if(pConn == NULL)
    {
        //分配连接失败，致命问题了，直接退出吧
        Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_init()中ngx_get_connection()分配连接失败.");
        exit(2);
    }
    pConn->listening = listening;
    // (*iter)->connection = pConn;

    // 对监听端口的读事件设置处理方法
    pConn->rhandler = std::bind(&CSocket::ngx_event_accept, this, std::placeholders::_1); ;

    // 往监听socket上增加监听事件，从而开始监听端口履行其职责
    // 将监听socket上红黑树
    if(ngx_epoll_oper_event(
                            listening->fd,         //socekt句柄
                            EPOLL_CTL_ADD,      //事件类型，这里是增加
                            EPOLLIN|EPOLLRDHUP, //标志，这里代表要增加的标志,EPOLLIN：可读，EPOLLRDHUP：TCP连接的远端关闭或者半关闭
                            0,                  //对于事件类型为增加的，不需要这个参数
                            pConn.get()              //连接池中的连接 
                            ) == -1)
    {
        Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_init()中ngx_epoll_oper_event()初始化监听事件失败.");
        exit(2); //致命问题，退出吧
    }
    
    // }
    return 1;
}

// 对epoll事件的具体操作
// 成功返回1，失败返回-1
int CSocket::ngx_epoll_oper_event(int fd,               //句柄，一个socket
                        uint32_t eventtype,        //事件类型，一般是EPOLL_CTL_ADD，EPOLL_CTL_MOD，EPOLL_CTL_DEL ，说白了就是操作epoll红黑树的节点(增加，修改，删除)
                        uint32_t flag,             //标志，具体含义取决于eventtype
                        int bcaction,         //补充动作，用于补充flag标记的不足  :  0：增加   1：去掉 2：完全覆盖 ,eventtype是EPOLL_CTL_MOD时这个参数就有用
                        ngx_connection_s* pConn             //pConn：一个指针【其实是一个连接】，EPOLL_CTL_ADD时增加到红黑树中去，将来epoll_wait时能取出来用)// epoll操作
                        )
{
    struct epoll_event ev;
    memset(&ev,0,sizeof(ev));

    if(eventtype == EPOLL_CTL_ADD)
    {
        ev.events = flag;
    }
    else if(eventtype == EPOLL_CTL_MOD)
    {
        ev.events = pConn->events;
        if(bcaction == 0)
        {
            // 增加某个标记
            ev.events |= flag;
        }
        else if(bcaction == 1)
        {
            // 去掉某个标记
            ev.events &= (~flag);
        }
        else
        {
            // 覆盖所有标记
            ev.events = flag;
        }
        pConn->events = ev.events;
    }
    else if (eventtype == EPOLL_CTL_DEL)
    {
        //TODO: 删除红黑树节点

        // return 1;
    }

    ev.data.ptr = pConn;
    if(epoll_ctl(m_epollhandle,eventtype,fd,&ev) == -1)
    {
        Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_oper_event()中epoll_ctl(%d,%ud,%ud,%d)失败.",fd,eventtype,flag,bcaction);    
        return -1;
    }
    return 1;
}

// 开始获得发生的事件消息
// unsigned int timer: epoll_wait的超时时间，单位是毫秒
// 返回值：1正常，0错误
// 本函数被ngx_process_events_and_timers()调用，而ngx_process_events_and_timers()是在子进程的死循环中被反复调用
// EPOLLHUP：表示被监视的文件描述符发生挂起（hang up）。例如，对于 TCP 连接，这通常意味着连接已经关闭。
//EPOLLERR：表示发生了错误。通常在文件描述符上进行操作时发生错误，比如尝试在关闭的 socket 上读写。
//EPOLLRDHUP：这是一个 TCP 特有的事件，表示对端关闭了连接的读半部分。可以在应用层使用它来检测对端是否正常关闭连接。
int CSocket::ngx_epoll_process_events(int timer)
{
    //等待事件，事件返回到m_events里，最多返回NGX_MAX_EVENTS个事件；
    //如果两次调用epoll_wait时间间隔较长，可能是epoll双向链表中累计太多事件
    //阻塞timer这么长的时间：a）阻塞时间到；b）有事件发生【比如收到新用户连接】；c）被信号打断，比如用kill -1 pid测试
    //如果timer为-1，则一直阻塞，直到有事件发生；如果为0，则立即返回，不管有没有事件发生；如果为其他值，则阻塞timer这么长的时间
    //返回值：1）有错误发生，返回-1，错误信息【4：Interrupted system call】；2）有事件发生，返回n【n是发生的事件个数】；3）超时，返回0
    int events = epoll_wait(m_epollhandle,m_events,NGX_MAX_EVENTS,timer);

    if(events == -1)
    {
        // 有错误发生
        if(errno == EINTR)
        {
            // 被信号打断，比如用kill -1 pid测试
            Logger::ngx_log_error_core(NGX_LOG_INFO,errno,"CSocekt::ngx_epoll_process_events()中epoll_wait()被信号打断.");
            return 1;
        }
        else
        {
            Logger::ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocekt::ngx_epoll_process_events()中epoll_wait()失败!"); 
            return 0;  //非正常返回 
        }
    }

    if(events == 0) // 超时
    {
        if(timer != -1)
        {
            return 1; //阻塞时间到
        }
        // 无限等待，但没有返回任何事件就退出阻塞，认为有问题
        Logger::ngx_log_error_core(NGX_LOG_ALERT,0,"CSocekt::ngx_epoll_process_events()中epoll_wait()没超时却没返回任何事件!"); 
        return 0; //非正常返回 
    }



    // 有事件收到了
    lpngx_connection_t pConn;
    uint32_t revents;
    for(int i = 0; i < events; i++) //遍历本次epoll_wait返回的所有事件，events才是返回的实际事件数量
    {
        auto Connptr = static_cast<ngx_connection_s*>(m_events[i].data.ptr);
        lpngx_connection_t pConn = Connptr->shared_from_this();
        

        //事件没有过期
        revents = m_events[i].events;

        if(revents & EPOLLIN) //可读事件
        {
            // 读事件发生，处理读事件
            Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_process_events()中EPOLLIN事件发生.");
            if(revents & (EPOLLHUP|EPOLLERR|EPOLLRDHUP)) // 连接断开事件
            {
                // --pConn->iThrowsendCount; // 连接断开
                Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_process_events()中EPOLLIN事件发生，连接断开.");
                // zdClosesocketProc(pConn);
            }
            else
            {
                Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_process_events()中EPOLLIN事件发生，正常读事件.");
                Logger::ngx_log_stderr(errno,"pConn %d, pConn->rhandlder %d, pConn id %d", &pConn, &(pConn->rhandler), pConn->id);
                pConn->rhandler(pConn); 
            }

        }

        if(revents & EPOLLOUT) //可写事件
        {
            // 写事件发生，处理写事件
            Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_process_events()中EPOLLOUT事件发生.");
            if(revents & (EPOLLHUP|EPOLLERR|EPOLLRDHUP)) // 连接断开事件
            {
                // --pConn->iThrowsendCount; // 连接断开
                Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_process_events()中EPOLLOUT事件发生，连接断开.");
                // zdClosesocketProc(pConn);
            }
            else
            {
                Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_process_events()中EPOLLOUT事件发生，正常写事件.");
                pConn->whandler(pConn); //如果有数据没有发送完毕，由系统驱动来发送
            }
        }

    }                
    // Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_process_events()中EPOLLIN事件发生，正常读事件后面1.");
    return 1; //正常返回
}


// epoll相关的-------------------------





// 处理发送消息队列的线程
void* CSocket::ServerSendQueueThread(void* threadData) // 专门用来发送数据的线程
{
// 根据响应体，决定是否关闭连接zdClosesocketProc

// 优化一下代码，将sendmsg提出


    Logger::ngx_log_stderr(errno, "ServerSendQueueThread...............");
    auto pThreadItem = static_cast<ThreadItem*>(threadData);
    auto pSocket = pThreadItem->_pThis.lock();
    if (!pSocket) return nullptr;
    Logger::ngx_log_stderr(errno, "ServerSendQueueThread1...............");
    while (Server::instance().g_stopEvent == 0) {
        // 等待信号量
        if (sem_wait(&pSocket->m_semEventSendQueue) == -1 && errno != EINTR) {
            Logger::ngx_log_stderr(errno, "sem_wait失败");
        }

        if (Server::instance().g_stopEvent != 0) break;
        Logger::ngx_log_stderr(errno, "ServerSendQueueThread2...............");
        // 消息队列处理
        {
            // std::lock_guard<std::mutex> lock(pSocket->m_sendMessageQueueMutex);
            std::pair<STRUC_MSG_HEADER, std::vector<uint8_t>> tmp;
            pSocket->m_MsgSendQueue.TryDequeue(tmp);
            
            // while (it != pSocket->m_MsgSendQueue.end())
            {
                std::vector<uint8_t> buf = tmp.second;
                STRUC_MSG_HEADER pMsghead = tmp.first;
                // 消息头验证
                if ( pMsghead.pConn->iCurrsequence != pMsghead.iCurrsequence) 
                {
                    // it = pSocket->m_MsgSendQueue.erase(it);
                    --pSocket->m_iSendMsgQueueCount;
                    continue;
                }

                // // 消息处理核心逻辑
                // const size_t headerSize = sizeof(STRUC_MSG_HEADER);
                // std::string payload_str(
                //     pMsgbuf->data() + headerSize, 
                //     pMsgbuf->size() - headerSize
                // ); 

                // 发送处理
                Buffer tmpbuf;
                tmpbuf.append(reinterpret_cast<char*>(buf.data()), buf.size());
                // pConn->psendbuf.append(payload_str.c_str(), payload_str.size());
                ssize_t sendsize = pSocket->sendproc(pMsghead.pConn, tmpbuf);
                // Logger::ngx_log_error_core(NGX_LOG_INFO, 0, "ServerSendQueueThread tmpbuf is: %s", tmpbuf.peek());
                // 结果处理
                if (sendsize > 0) {
                    if (sendsize == tmpbuf.readableBytes()) {
                        tmpbuf.retrieveAll();
                        // 仅在完整发送时检查400响应
                        if ((buf.size() == strlen("HTTP/1.1 400 Bad Request\r\n\r\n")) && 
                            (memcmp(buf.data(), "HTTP/1.1 400 Bad Request\r\n\r\n", buf.size()) == 0)) {
                            Logger::ngx_log_stderr(0, "发送400错误后关闭连接 fd=%d", pMsghead.pConn->fd);
                            pSocket->zdClosesocketProc(pMsghead.pConn);
                        }
                        if(pMsghead.pConn->ishttpClose)
                        {
                            Logger::ngx_log_stderr(0, "服务器主动断开连接 fd=%d", pMsghead.pConn->fd);
                            pSocket->zdClosesocketProc(pMsghead.pConn);
                        }
                    } else {
                        tmpbuf.retrieve(sendsize);
                        // 添加新消息头到剩余数据前
                        // STRUC_MSG_HEADER newHeader{
                        //     .pConn = pConn,
                        //     .iCurrsequence = pConn->iCurrsequence
                        // };
                        pMsghead.pConn->psendbuf.append(tmpbuf.peek(), tmpbuf.readableBytes());
                        pMsghead.pConn->sendCount = pMsghead.iCurrsequence;
                        pSocket->ngx_epoll_oper_event(pMsghead.pConn->fd, EPOLL_CTL_MOD, 
                                                    EPOLLOUT, 0, pMsghead.pConn.get());
                        tmpbuf.retrieveAll(); 
                    }
                } else if (sendsize == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {

                        pMsghead.pConn->psendbuf.append(tmpbuf.peek(), tmpbuf.readableBytes());
                        pMsghead.pConn->sendCount = pMsghead.iCurrsequence;
                        // 发送缓冲区已满，保留未发送数据
                        Logger::ngx_log_stderr(0, "发送缓冲区已满，保留未发送数据");
                        pSocket->ngx_epoll_oper_event(pMsghead.pConn->fd, EPOLL_CTL_MOD,
                                                    EPOLLOUT, 0, pMsghead.pConn.get());
                    } else {
                        // 其他错误处理（如ECONNRESET）
                        pMsghead.pConn->psendbuf.retrieveAll();
                        Logger::ngx_log_stderr(0, "其他错误处理（如ECONNRESET）");
                        pSocket->zdClosesocketProc(pMsghead.pConn);
                        // CloseConnection(pConn);
                    }
                } else {
                    if(sendsize == 0 && pMsghead.pConn->psendbuf.readableBytes() == 0)
                        Logger::ngx_log_stderr(0, " 发送空数据 ");
                    else
                    {
                        pMsghead.pConn->psendbuf.retrieveAll();
                        Logger::ngx_log_stderr(0, "其他错误处理（如ECONNRESET）");
                        pSocket->zdClosesocketProc(pMsghead.pConn);
                    }

                }

                // 移除已处理消息
                // it = pSocket->m_MsgSendQueue.erase(it);
                --pSocket->m_iSendMsgQueueCount;
            }
        }
    }
    return nullptr;







    // ngx_log_stderr(errno,"ServerSendQueueThread");
    // auto pThreadItem = static_cast<ThreadItem*>(threadData);
    // if(pThreadItem->_pThis.lock() == nullptr)
    // {
    //     return nullptr;
    // }
    // std::shared_ptr<CSocket> pSocket = pThreadItem->_pThis.lock();
    // int err;
    // std::list<std::shared_ptr<std::vector<char>>>::iterator pos,pos2,posend;

    // std::shared_ptr<std::vector<char>> pMsgbuf;
    // LPSTRUC_MSG_HEADER pMsgHeader;
    // LPCOMM_PKG_HEADER pPkgHeader;
    // lpngx_connection_t p_Conn;
    // uint16_t itmp;
    // ssize_t sendsize;

    // // CMemory memory = CMemory::getInstance();

    // while(g_stopEvent == 0) // 线程不退出
    // {
    //     ngx_log_stderr(0,"ServerSendQueueThread looping ... ...");
    //     //如果信号量值>0，则 -1(减1) 并走下去，否则卡这里卡着【为了让信号量值+1，可以在其他线程调用sem_post达到，实际上在CSocekt::msgSend()调用sem_post就达到了让这里sem_wait走下去的目的】
    //     //如果被某个信号中断，sem_wait也可能过早的返回，错误为EINTR；
    //     //整个程序退出之前，也要sem_post()一下，确保如果本线程卡在sem_wait()，也能走下去从而让本线程成功返回
    //     if(sem_wait(&pSocket->m_semEventSendQueue) == -1)
    //     {
    //         //失败？及时报告
    //         if(errno != EINTR)
    //         {
    //             ngx_log_stderr(errno,"CSocekt::ServerSendQueueThread()中sem_wait(&pSocketObj->m_semEventSendQueue)失败.");    
    //         }
    //     }

    //     // 处理数据收发
    //     if(g_stopEvent != 0)
    //         break;
        
    //     if(pSocket->m_iSendMsgQueueCount > 0)
    //     {
    //         std::lock_guard<std::mutex> lock(pSocket->m_sendMessageQueueMutex);

    //         pos = pSocket->m_MsgSendQueue.begin();
    //         posend = pSocket->m_MsgSendQueue.end();
    //         while(pos!= posend)
    //         {
    //             pMsgbuf = *pos; // 拿到的消息
    //             // 提取头部
    //             STRUC_MSG_HEADER header;
    //             std::memcpy(&header, pMsgbuf->data(), sizeof(STRUC_MSG_HEADER));

    //             // 提取数据体
    //             size_t headerSize = sizeof(STRUC_MSG_HEADER);
    //             size_t dataSize = pMsgbuf->size() - headerSize;

    //             // 判断是http消息还是tcp连接的消息
    //             lpngx_connection_t headptr =  header.pConn.lock();
    //             if (!headptr) {
    //                 ngx_log_stderr(0, "无效的连接指针");
    //                 continue;  // 或其他错误处理
    //             }
    //             if(headptr->ishttp)
    //             {
  
    //                 p_Conn = headptr; // 指向连接

    //                 if(p_Conn->iCurrsequence != headptr->iCurrsequence)
    //                 {
    //                     // 包序号错误，丢弃该包
    //                     pos2 = pos;
    //                     pos++;
    //                     pSocket->m_MsgSendQueue.erase(pos2);
    //                     --pSocket->m_iSendMsgQueueCount;
    //                     // memory.FreeMemory(pMsgbuf);
    //                     continue;
    //                 }
    
    //                 // if(p_Conn->iThrowsendCount > 0)
    //                 // {
    //                 //     pos++;
    //                 //     continue;
    //                 // }
    //                 size_t dataSize = pMsgbuf->size() - headerSize;
    //                 std::string dataBody(pMsgbuf->data() + headerSize, dataSize);
    //                 --p_Conn->iSendCount; // 发送计数减1
    
    //                 //可以发送消息了，一些必要的信息记录，要发送的东西也要从发送队列干掉
    //                 pos2 = pos;
    //                 pos++;
    //                 pSocket->m_MsgSendQueue.erase(pos2);
    //                 --pSocket->m_iSendMsgQueueCount; // 发送队列计数减1
    //                 p_Conn->psendbuf.append(dataBody.c_str(), dataBody.size());
    //                 sendsize = pSocket->sendproc(p_Conn, p_Conn->psendbuf); // 发送数据

    //             }

    //             // else
    //             // {
    //             //     pPkgHeader = (LPCOMM_PKG_HEADER)(pMsgbuf + pSocket->m_iLenMsgHeader); // 指向包头
    //             //     p_Conn = pMsgHeader->pConn; // 指向连接
    
    //             //     if(p_Conn->iCurrsequence != pMsgHeader->iCurrsequence)
    //             //     {
    //             //         // 包序号错误，丢弃该包
    //             //         pos2 = pos;
    //             //         pos++;
    //             //         pSocket->m_MsgSendQueue.erase(pos2);
    //             //         --pSocket->m_iSendMsgQueueCount;
    //             //         memory.FreeMemory(pMsgbuf);
    //             //         continue;
    //             //     }
    
    //             //     if(p_Conn->iThrowsendCount > 0)
    //             //     {
    //             //         pos++;
    //             //         continue;
    //             //     }
    
    //             //     --p_Conn->iSendCount; // 发送计数减1
    
    //             //     //可以发送消息了，一些必要的信息记录，要发送的东西也要从发送队列干掉
    //             //     p_Conn->psendMemPointer = pMsgbuf; //用来释放内存
    //             //     pos2 = pos;
    //             //     pos++;
    //             //     pSocket->m_MsgSendQueue.erase(pos2);
    //             //     --pSocket->m_iSendMsgQueueCount; // 发送队列计数减1
    //             //     p_Conn->psendbuf = (char*)pPkgHeader; // 要发送数据的缓冲区指针
    //             //     itmp = ntohs(pPkgHeader->pkgLen); // 包长度【包头+包体】
    //             //     p_Conn->isendlen = itmp; // 要发送的数据长度
    
    //             //     sendsize = pSocket->sendproc(p_Conn, p_Conn->psendbuf, p_Conn->isendlen); // 发送数据
    
    //             // }


 



    //             if(sendsize > 0)
    //             {
    //                 if(sendsize == p_Conn->psendbuf.readableBytes()) // 全部发送
    //                 {
    //                     // 发送成功，释放内存
    //                     p_Conn->psendbuf.retrieveAll();

    //                     // memory.FreeMemory(pMsgbuf);
    //                     // p_Conn->psendMemPointer = NULL; // 释放标志
    //                     // p_Conn->iThrowsendCount = 0; 

                        
    //                 }
    //                 else // 没有完全发送【发送缓冲区满了】
    //                 {
    //                     // 记录发送了多少数据，下次sendproc时继续
    //                     p_Conn->psendbuf.retrieve(sendsize);
    //                     // p_Conn->psendbuf = p_Conn->psendbuf + sendsize;
    //                     // p_Conn->isendlen = p_Conn->isendlen - sendsize;
    //                     // 发送缓冲区满了，需要依赖系统通知来发送数据
    //                     // ++p_Conn->iThrowsendCount; //ThrowsendCount用来标记连接还有未发送的数据注册到epoll上，保证后序在将所有数据都发送后才能释放Conn
    //                     // 依赖ngx_write_request_handler()来发送数据，这里不做处理，等待系统通知
    //                     if(pSocket->ngx_epoll_oper_event(
    //                             p_Conn->fd,         //socket句柄
    //                             EPOLL_CTL_MOD,      //事件类型，这里是增加【因为我们准备增加个写通知】
    //                             EPOLLOUT,           //标志，这里代表要增加的标志,EPOLLOUT：可写【可写的时候通知我】
    //                             0,                  //对于事件类型为增加的，EPOLL_CTL_MOD需要这个参数, 0：增加   1：去掉 2：完全覆盖
    //                             p_Conn.get()              //连接池中的连接
    //                             ) == -1)
    //                     {
    //                         ngx_log_stderr(errno,"CSocekt::ServerSendQueueThread()ngx_epoll_oper_event()失败.");
    //                     }
    //                 }
    //                 continue;

    //             }
    //             // 出现问题了
    //             else if(sendsize == 0)
    //             {
    //                 // memory.FreeMemory(p_Conn->psendMemPointer);  //释放内存
    //                 // p_Conn->psendMemPointer = NULL;
    //                 // p_Conn->iThrowsendCount = 0;  //这行其实可以没有，因此此时此刻这东西就是=0的    
    //                 p_Conn->psendbuf.retrieveAll();
    //                 continue;
    //             }
    //             else if(sendsize == -1)
    //             {
    //                 // 发送缓冲区满了，一个也没有发送出去，需要依赖系统通知来发送数据
    //                 // ++p_Conn->iThrowsendCount; // 标记发送缓冲区满了。需要通过epoll事件驱动消息来继续发送
    //                 // 依赖ngx_write_request_handler()来发送数据，这里不做处理，等待系统通知
    //                 if(pSocket->ngx_epoll_oper_event(
    //                         p_Conn->fd,         //socket句柄
    //                         EPOLL_CTL_MOD,      //事件类型，这里是增加【因为我们准备增加个写通知】
    //                         EPOLLOUT,           //标志，这里代表要增加的标志,EPOLLOUT：可写【可写的时候通知我】
    //                         0,                  //对于事件类型为增加的，EPOLL_CTL_MOD需要这个参数, 0：增加   1：去掉 2：完全覆盖
    //                         p_Conn.get()              //连接池中的连接
    //                         ) == -1)
    //                 {
    //                     ngx_log_stderr(errno,"CSocekt::ServerSendQueueThread()ngx_epoll_oper_event()失败.");
    //                 }
    //                 continue;
    //             }
    //             else
    //             {
    //                 // 返回值应该是-2，一般认为对端断开，等待recv来断开socket以及回收资源
    //                 // memory.FreeMemory(p_Conn->psendMemPointer);  //释放内存
    //                 // p_Conn->psendMemPointer = NULL;
    //                 // p_Conn->iThrowsendCount = 0;  //这行其实可以没有，因此此时此刻这东西就是=0的    
    //                 p_Conn->psendbuf.retrieveAll();
    //                 continue;
    //             }
                
            

    //         }

            

    //     }


    // }
    // return (void*)0;
}



}
