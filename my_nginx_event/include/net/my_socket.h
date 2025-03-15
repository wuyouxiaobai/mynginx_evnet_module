#pragma once
#include <vector>
#include <deque>
#include <list>
#include <mutex>
#include <semaphore.h>
#include <atomic>
#include <sys/socket.h>
#include "my_comm.h"
#include <sys/epoll.h>
#include <map>
#include <thread>
#include "my_crc32.h"
#include <memory>
#include "my_http_context.h"
#include <functional>
#include "Buffer.h"



#define NGX_LISTEN_BACKLOG 511 // 已完成连接队列，nginx给511，我们也先按照这个来：不懂这个数字的同学参考第五章第四节
#define NGX_MAX_EVENTS 512     // epoll_wait一次最多接收这么多个事件，nginx中缺省是512，我们这里固定给成512就行，没太大必要修改

namespace WYXB
{
// typedef struct ngx_listening_s ngx_listening_t, *lpngx_listening_t;
// typedef struct ngx_connection_s ngx_connection_t, *lpngx_connection_t;
// typedef class CSocket CSocket;

// typedef void (CSocket::*ngx_event_handler_pt)(lpngx_connection_t c); // 定义成员函数指针





class CSocket;
struct ngx_listening_s;
struct ngx_connection_s;

using lpngx_listening_t = std::shared_ptr<ngx_listening_s>;
using lpngx_connection_t = std::shared_ptr<ngx_connection_s>;
using ngx_event_handler = std::function<void(lpngx_connection_t)>;



// // 消息头，引入的目的是当收到数据包时，额外记录一些内容以备将来使用
typedef struct _STRUC_MSG_HEADER
{
    std::weak_ptr<ngx_connection_s> pConn;  // 使用智能指针
    uint64_t iCurrsequence;   // 收到数据包时记录对应连接的序号，将来能用于比较是否连接已经作废用
    //......其他以后扩展
} STRUC_MSG_HEADER;
using LPSTRUC_MSG_HEADER = std::shared_ptr<STRUC_MSG_HEADER>;

// 一些专用结构定义放在这里，暂时不考虑放ngx_global.h里了
// struct ngx_listening_s // 和监听端口有关的结构
// {
//     int port;                      // 监听的端口号
//     int fd;                        // 套接字句柄socket
//     lpngx_connection_t connection; // 连接池中的一个连接，注意这是个指针
// };

// 监听结构体优化
struct ngx_listening_s {
    int port = -1;
    int fd = -1;
    
    // 使用weak_ptr避免循环引用
    std::weak_ptr<ngx_connection_s> connection;  // 替换原始指针
    
};




// // 该结构表示一个Tcp连接【客户端主动发起，nginx服务器被动接受Tcp连接】
// struct ngx_connection_s
// {
//     ngx_connection_s();
//     virtual ~ngx_connection_s();
//     void GetOneToUse(); // 分配出去的时候初始化
//     void PutOneToFree(); //回收回来的时候做一些事
//     std::shared_ptr<HttpContext> getContext(); // 获取HttpContext，用来解析http请求报文

//     int fd; // 套接字句柄
//     lpngx_listening_t listening; // 如果这个连接分配给了一个监听套接字，这里就指向监听套接字对应的lpngx_listening_t内存首地址

//     uint64_t iCurrsequence; 
//     sockaddr s_sockaddr; // 保存对方地址信息

//     // 和读有关
//     ngx_event_handler_pt rhandler; // 读事件处理方法
//     ngx_event_handler_pt whandler; // 写事件处理方法

//     // epoll相关
//     uint32_t events;

//     // 收包相关
//     unsigned char curStat; // 当前收包状态
//     char dataHeadInfo[_DATA_BUFSIZE_]; // 保存收到的包头数据
//     char *precvbuf; //接收数据的缓冲区
//     unsigned int irecvlen; // 收到多少数据由这个变量指定
//     char *precvMemPointer; // new出来的用于收包的内存首地址，释放用的

//     // std::mutex logicPorcMutex;

//     // 发包相关
//     std::atomic<int> iThrowsendCount; //iThrowsendCount的作用，用来标记
//     char *psendMemPointer; // 发送完成后释放【消息头 + 包头 + 包体】
//     char *psendbuf; // 发送数据缓冲区头指针，【包头+包体】
//     unsigned int isendlen; // 要发送多少数据

//     // 回收相关
//     time_t inRecyTime; 
//     // 心跳包
//     time_t lastPingTime;
//     // 网络安全
//     uint64_t FloodKickLastTime; // 上次受到攻击的时间
//     int FloodAttackCount; // 受到攻击的次数
//     std::atomic<int> iSendCount; //发送队列中的条目数

//     // lpngx_connection_t next;

//     // http相关
//     std::atomic_bool ishttp;
//     std::shared_ptr<HttpContext> context_;
// };
// 网络连接结构体现代化改造
struct ngx_connection_s : public std::enable_shared_from_this<ngx_connection_s> 
{
    ngx_connection_s();
    virtual ~ngx_connection_s();
    

    void GetOneToUse(int sockfd); // 分配出去的时候初始化
    void PutOneToFree(); //回收回来的时候做一些事
    std::shared_ptr<HttpContext> getContext(); // 获取HttpContext，用来解析http请求报文

// socket相关
    int fd{-1}; // 套接字句柄
    lpngx_listening_t listening; // 所属监听


    std::atomic<uint64_t> iCurrsequence{0};// 包的序号
    sockaddr s_sockaddr; // 保存对方地址信息

// epoll相关
    // 读写回调函数
    ngx_event_handler rhandler;
    ngx_event_handler whandler;
    // epoll唤醒的事件类型
    uint32_t events{0};

// 回收相关
    time_t inRecyTime; 

 // 心跳包
    time_t lastPingTime;

// 网络安全
    uint64_t FloodKickLastTime{0}; // 上次受到攻击的时间
    int FloodAttackCount{0}; // 受到攻击的次数
    std::atomic<int> iSendCount{0}; //发送队列中的条目数


// http相关
    std::atomic_bool ishttp{false};
    std::shared_ptr<HttpContext> context_; // 解析并暂存的http请求报文
    Buffer psendbuf;


// 删除拷贝操作，允许移动操作
    ngx_connection_s(const ngx_connection_s&) = delete;
    ngx_connection_s& operator=(const ngx_connection_s&) = delete;
    ngx_connection_s(ngx_connection_s&&) = default;
    ngx_connection_s& operator=(ngx_connection_s&&) = default;

};














//socket
class CSocket : public std::enable_shared_from_this<CSocket>
{
public:
    CSocket();
    virtual ~CSocket();
    virtual bool Initialize(); // 初始化函数【父进程】
    virtual bool Initialize_subproc(); // 初始化函数【子进程】
    virtual void Shotdown_subproc();// 关闭退出函数【子进程中执行】

    void printTDInfo(); // 打印统计信息
public:
    virtual void threadRecvProcFunc(std::vector<uint8_t>&& pMsgBuf){}; // 处理http请求
    virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t currTime); // 心跳包检测时间到，去检测心跳包是否超时，本函数只释放内存，子函数应该重写具体操作

public:
    int ngx_epoll_init();// epoll初始化
    int ngx_epoll_process_events(int timer);// epoll等待或者处理事件
    int ngx_epoll_oper_event(int fd, uint32_t eventtype, uint32_t flag, int bcaction, ngx_connection_s* pConn); // epoll操作

protected:
    void msgSend(std::string psendbuf, lpngx_connection_t pConn); // 把数据放到待发送队列
    void zdClosesocketProc(lpngx_connection_t pConn); // 主动关闭一个连接要做的善后的处理函数

private:
    void ReadConf(); // 读取配置文件
    lpngx_listening_t ngx_open_listening_sockets(); // 打开监听端口【支持多端口】
    void ngx_close_listening_sockets(); // 关闭监听端口
    bool setnonblocking(int sockfd); // 设置非阻塞
    
    // 一些业务的回调函数
    void ngx_event_accept(lpngx_connection_t oldc);// 建立连接
    void ngx_read_request_handler(lpngx_connection_t pConn); //设置数据来时的读回调函数
    void ngx_write_response_handler(lpngx_connection_t pConn); //设置数据发出时的写回调函数

    void ngx_close_connection(lpngx_connection_t pConn); //设置连接关闭时的回调函数

protected:
    ssize_t recvproc(lpngx_connection_t pConn, char *pBuf, ssize_t bufLen); //接受从客户端来的数据专用函数
private:
    void ngx_wait_request_handler_proc_p1(lpngx_connection_t pConn, bool &isflood);// 处理收到的数据【可能是只有包头】也可能【是包头+包体】
    void ngx_wait_request_handler_proc_plast(lpngx_connection_t pConn, bool &isflood); // 将处理过的数据放入消息队列，后序由各自线程处理

    void clearMsgSendQueue(); // 处理发送消息队列
protected:
    ssize_t sendproc(lpngx_connection_t c, Buffer buff);// 将数据发送
private:
    // 获得对端信息相关
    size_t ngx_sock_ntop(struct sockaddr *sa, int port, u_char *text, size_t len); // 根据参数1获得对端信息，获得地址端口字符串，返回这个字符串长度

    //连接池
    void initconnection(); // 初始化连接池
    void clearconnection(); // 清理连接池
    lpngx_connection_t ngx_get_connection(int isock); // 从连接池获得一个空闲连接
    void ngx_free_connection(lpngx_connection_t pConn); // 将连接放回连接池
    void inRecyConnectQueue(lpngx_connection_t pConn); // 将要回收的连接放到一个队列里

    // 和时间相关
    void AddToTimmerQueue(lpngx_connection_t pConn); // 设置踢出时钟（向map中增加内容）
    time_t GetEarliestTime(); // 从multimap中获得最早时间返回
    LPSTRUC_MSG_HEADER RemoveFirstTimer(); // 从m_timeQueuemap中移除最早的时间，并把最早时间所指项的值所对应的指针返回，调用者负责互斥
    LPSTRUC_MSG_HEADER GetOverTimeTimer(time_t currTime); // 根据所给时间，从m_timerQueuemap中找到比这个时间更早的节点【1个】返回，这些节点都是时间超过，需要处理
    void DeleteFromTimerQueue(lpngx_connection_t pconn);// 把指定用户tcp连接从timer表中去除
    void clearAllFromTimerQueue(); // 清理时间队列所有内容

    // 网络安全相关
    bool TestFlood(lpngx_connection_t pConn); // 测试是否受到flood攻击

    //线程相关函数
    static void* ServerSendQueueThread(void* threadData); // 专门用来发送数据的线程
    static void* ServerRecyConnectionThread(void* threadData); // 专门用来回收连接的线程
    static void* ServerTimerQueueMonitorThread(void* threadData); // 专门用来监控时间队列的线程【不发心跳包的用户踢出】

protected:
    // 一些和网络通信相关成员变量
    size_t m_iLenPkgHeader; // sizeof(COMM_PKG_HEADER))
    size_t m_iLenMsgHeader; // sizeof(STRUC_MSG_HEADER))

    //时间相关
    int m_ifTimeOutKick; // 当时间到达Sock_MaxWaitTime指定时间时，直接把客户踢出
    int m_iWaitTime; // 多少秒检测一次心跳是否超时

private:
    struct ThreadItem
    {
        std::thread _Thread;
        // pthread_t _Handle; // 线程句柄
        std::weak_ptr <CSocket> _pThis; // 记录所属socket
        // bool ifrunning; // 线程是否正常启动，启动起来后，才允许调用stopAll()

        void tie(std::shared_ptr<CSocket> pThis);

        ThreadItem();

        ~ThreadItem();
    };

    int m_worker_connections; // epoll连接的最大数量
    int m_ListenPortCount; // 监听端口数量
    int m_epollhandle; // epoll返回的句柄

    //连接池相关
    std::vector<lpngx_connection_t> m_connectionList; // 连接列表【连接池】
    std::deque<lpngx_connection_t> m_freeConnectionList; // 空闲连接列表【装的全是空闲连接】
    std::atomic<int> m_total_connection_n; // 连接池总数量
    std::atomic<int> m_free_connection_n; // 空闲连接数量
    std::mutex m_connectionListMutex; // 连接池互斥锁
    std::mutex m_recyconnqueueMutex; // 回收连接队列互斥锁
    std::vector<lpngx_connection_t> m_recyconnectionList; // 回收连接队列
    std::atomic<int> m_total_recyconnection_n;// 待回收连接队列大小
    int m_RecyConnectionWaitTime; // 等待一段时间才回收连接【慢回收？】

    std::vector<lpngx_listening_t> m_ListenSocketList; // 监听套接字列表
    struct epoll_event m_events[NGX_MAX_EVENTS]; // epoll_wait中接受返回的所发生的事件

    // 消息队列
    std::list<std::shared_ptr<std::vector<char>>> m_MsgSendQueue; // 发送数据消息队列
    std::atomic<int> m_iSendMsgQueueCount; // 发送消息队列大小

    // 多线程相关
    std::vector<std::shared_ptr<ThreadItem>> m_threadVector; // 线程列表
    std::mutex m_sendMessageQueueMutex; // 发送消息队列互斥锁
    sem_t m_semEventSendQueue; // 处理发送消息相关的信号量

    // 时间相关
    int m_ifkickTimeCount; // 是否开启踢人时钟，1：开启，0：不开启
    std::mutex m_timequeueMutex; // 时间队列互斥锁
    std::multimap<time_t, LPSTRUC_MSG_HEADER> m_timeQueuemap; // 时间队列map，key是时间，value是指针，指针指向一个结构体，结构体中记录了用户的tcp连接信息，以及用户的心跳包时间
    size_t m_cur_size_; // 时间队列尺寸
    time_t m_timer_value_; // 当前计时队列头部时间值

    //在线用户
    std::atomic<int> m_onlineUserCount; // 在线用户数量
    // 网络安全相关
    int m_floodAkEnable; // 是否开启网络安全功能，1：开启，0：不开启
    uint32_t m_floodTimeInterval; // 每次收到数据包的时间间隔100ms
    int m_floodKickCount; // 收到攻击的上限，超过就会踢人

    // 统计用途
    time_t m_lastprintTime; // 上次打印统计信息的时间【10s打印一次】
    int m_iDiscardSendPkgCount; // 丢弃发送消息包的次数

private:
    // http相关
    void ngx_http_read_request_handler(lpngx_connection_t pConn); //设置http数据来时的读回调函数
    void ngx_http_write_response_handler(lpngx_connection_t pConn); //设置http数据发出时的写回调函数
    // std::shared_ptr<HttpGetProcessor> m_httpGetProcessor;

};







}