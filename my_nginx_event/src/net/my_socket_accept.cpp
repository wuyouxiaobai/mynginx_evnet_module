#include "my_socket.h"
#include "my_macro.h"
#include "my_func.h"
#include <unistd.h>
#include <netinet/tcp.h>



namespace WYXB
{
// 建立连接专用函数，新连接进入时，本函数会被ngx_epoll_process_events()所调用
void CSocket::ngx_event_accept(lpngx_connection_t oldc)// 建立连接
{
    sockaddr mysockaddr; // 客户端socket地址
    socklen_t socklen;
    int err;
    int level;
    int sockfd;
    static int use_accept4=1; // 认为能使用accept4函数
    lpngx_connection_t newc; // 连接池中的新连接

    socklen = sizeof(mysockaddr);
    do
    {
        Logger::ngx_log_stderr(0,"m_worker_connections %d ......", m_worker_connections);
        if(use_accept4)
        {
            // 因为listen套接字是非阻塞，所以即使完成连接队列为空，accept也不会阻塞
            sockfd = accept4(oldc->fd, (struct sockaddr*)&mysockaddr, &socklen, SOCK_NONBLOCK); // 从内核获得一个用户连接
        }
        else
        {
            sockfd = accept(oldc->fd, (struct sockaddr*)&mysockaddr, &socklen); // 从内核获得一个用户连接
        }

        if(sockfd == -1)
        {
            err = errno;
            //对accept、send和recv而言，事件未发生时errno通常被设置成EAGAIN（意为“再来一次”）或者EWOULDBLOCK（意为“期待阻塞”） 
            if(err == EAGAIN || err == EWOULDBLOCK)
            {
                // 连接队列为空，继续等待
                return;
            }
            level = NGX_LOG_ALERT;
            if(err == ECONNABORTED)//ECONNRESET错误则发生在对方意外关闭套接字后【您的主机中的软件放弃了一个已建立的连接--由于超时或者其它失败而中止接连(用户插拔网线就可能有这个错误出现)】
            {
                level = NGX_LOG_ERR;
            }
            else if(err == EMFILE || err == ENFILE) //EMFILE 错误表示“进程已打开的文件描述符达到上限”。这意味着当前进程已经打开了太多的文件或套接字，超出了系统允许的限制。
                                                     //ENFILE 错误表示“系统已打开的文件描述符达到上限”。这意味着系统范围内的所有进程打开的文件描述符总和已经达到了限制。    
            {
                level = NGX_LOG_CRIT;
            }

            if(use_accept4 && err == ENOSYS) // 没有accept4函数，则尝试使用accept函数
            {
                use_accept4 = 0;
                continue;
            }

            if (err == ECONNABORTED)  //对方关闭套接字
            {
                //这个错误因为可以忽略，所以不用干啥
                //do nothing
            }
            
            if (err == EMFILE || err == ENFILE) 
            {
                //do nothing，这个官方做法是先把读事件从listen socket上移除，然后再弄个定时器，定时器到了则继续执行该函数，但是定时器到了有个标记，会把读事件增加到listen socket上去；
                //我这里目前先不处理吧【因为上边已经写这个日志了】；
            }            
            return;
        }

        //accept4/accept成功，创建新的连接对象
        if(m_onlineUserCount >= m_worker_connections) // 用户连接数达到上限，关闭当前用户socket
        {
            close(sockfd);
            return;
        }
        // 如果某些恶意用户连上后发送一条数据就断，然后不断连接，就会导致频繁调用ngx_get_connection在短时间产生大量连接
        if(m_connectionList.size() > m_worker_connections * 5)
        {
            // 如果按照设定只允许最大2048个连接，但连接池却有2048*5这么多连接，就表示短时间产生了大量连接/断开，因为设计的延迟回收机制，这里的连接还在垃圾池里并没有被回收
            if(m_freeConnectionList.size() < m_worker_connections)
            {
                //整个连接池已经很大了，但是空闲连接依旧很少，认为短时间产生了大量连接，故断开新用户连接
                //直到m_freeConnectionList足够大【连接池中连接被回收的足够多】
                close(sockfd);
                return;
            }
        }


        // 在连接建立后设置socket选项
        int recv_buf_size = 1 * 1024 * 1024;  // 2MB
        if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(recv_buf_size))==-1)
        {
            Logger::ngx_log_stderr(errno,"setsockopt 在连接建立后设置socket选项 失败.");
        }

        // // 启用TCP快速打开(TFO)
        // int qlen = 5; 
        // if(setsockopt(sockfd, SOL_TCP, TCP_FASTOPEN, &qlen, sizeof(qlen))==-1)
        // {
        //     Logger::ngx_log_stderr(errno,"setsockopt 启用TCP快速打开(TFO) 失败.");
        // }

        newc = ngx_get_connection(sockfd); // 从连接池中获得一个空闲连接
                                           //这是针对新连入用户的连接，和监听套接字 所对应的连接是两个不同的东西，不要搞混
        if(newc == NULL)
        {
            // 连接池中连接不够，关闭sockfd即可
            if(close(sockfd) == -1)
            {
                Logger::ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocekt::ngx_event_accept()中close(%d)失败!",sockfd);          
            }
            return;
        }
        
        // 成功拿到连接池中的连接
        memcpy(&newc->s_sockaddr, &mysockaddr, socklen); // 记录客户端的socket地址
        
        if(!use_accept4)
        {
            //如果不是用accept4()取得的socket，那么就要设置为非阻塞【因为用accept4()的已经被accept4()设置为非阻塞了】
            if(setnonblocking(sockfd) == false)
            {
                //设置非阻塞居然失败
                ngx_close_connection(newc); //关闭socket,这种可以立即回收这个连接，无需延迟，因为其上还没有数据收发，谈不到业务逻辑因此无需延迟；
                return; //直接返回
            }
        }

        newc->listening = oldc->listening; // 连接对象和监听对象关联，方便通过连接对象找监听对象【关联到监听端口】
        
        Logger::ngx_log_stderr(0,"ngx_event_accept bind ......");
        newc->rhandler = std::bind(&CSocket::ngx_http_read_request_handler, this, std::placeholders::_1); // 设置读事件处理函数
        newc->whandler = std::bind(&CSocket::ngx_http_write_response_handler, this, std::placeholders::_1); // 设置写事件处理函数

        // 客户端应该主动发送第一次数据，对读事件加入epoll监听，当客户端发送数据时，会触发读事件，读事件处理函数ngx_read_request_handler会被调用
        if(ngx_epoll_oper_event(sockfd, EPOLL_CTL_ADD, EPOLLIN, 0 ,newc.get()) == -1)
        {
            //增加事件失败，失败日志在ngx_epoll_add_event中写过了，因此这里不多写啥；
            ngx_close_connection(newc);//关闭socket,这种可以立即回收这个连接，无需延迟，因为其上还没有数据收发，谈不到业务逻辑因此无需延迟；
            return; //直接返回
        }
        Logger::ngx_log_stderr(0,"ngx_event_accept bind 2222......");
        if(m_ifkickTimeCount == 1)
        {
            AddToTimmerQueue(newc);
        }
        Logger::ngx_log_stderr(0,"ngx_event_accept bind 3333......");
    } while (1);
    
    return;


}





}
