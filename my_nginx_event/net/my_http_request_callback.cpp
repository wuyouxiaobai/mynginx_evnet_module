#include "my_socket.h"
#include "my_func.h"
#include "my_memory.h"
#include <arpa/inet.h>
#include <cstring>
#include "my_global.h"
#include "my_http_context.h"

namespace WYXB
{




// http请求报文到来时的回调
void CSocket::ngx_http_read_request_handler(lpngx_connection_t pConn)
{
    auto logConnectionError = [pConn](int errcode, const char* msg) {
        char ip[INET6_ADDRSTRLEN] = "unknown"; // 扩展支持IPv6
        uint16_t port = 0;
    
        if (pConn->s_sockaddr.sa_family == AF_INET) {
            auto* addr4 = reinterpret_cast<sockaddr_in*>(&pConn->s_sockaddr);
            inet_ntop(AF_INET, &addr4->sin_addr, ip, INET_ADDRSTRLEN);
            port = ntohs(addr4->sin_port);
        } else if (pConn->s_sockaddr.sa_family == AF_INET6) {
            auto* addr6 = reinterpret_cast<sockaddr_in6*>(&pConn->s_sockaddr);
            inet_ntop(AF_INET6, &addr6->sin6_addr, ip, INET6_ADDRSTRLEN);
            port = ntohs(addr6->sin6_port);
        }
    
        ngx_log_stderr(errcode, "%s [%s]:%d (family:%d)", 
                      msg, ip, port, pConn->s_sockaddr.sa_family);
    };

    // 创建消息缓冲区（头部+最大数据空间）
    std::vector<uint8_t> buffer(sizeof(STRUC_MSG_HEADER) + 4096);

    // 在缓冲区头部构造消息头（使用placement new）
    auto* header = new (buffer.data()) STRUC_MSG_HEADER{
        .pConn = pConn,
        .iCurrsequence = pConn->iCurrsequence
    };

    // 接收网络数据
    const ssize_t n = recv(
        pConn->fd,
        reinterpret_cast<char*>(buffer.data()) + sizeof(STRUC_MSG_HEADER),
        4096,
        0
    );

    // 处理接收结果
    if (n > 0) {
        buffer.resize(sizeof(STRUC_MSG_HEADER) + n);
        g_threadpool.inMsgRecvQueueAndSignal(std::move(buffer));
        return;
    }

    // 处理错误情况
    const int lastError = errno;  // 立即保存errno值
    switch(n) {
    case 0:  // 客户端主动关闭连接
        logConnectionError(lastError, "Connection closed by client");
        zdClosesocketProc(pConn);
        break;
        
    case -1: // 处理系统调用错误
        switch(lastError) {
        case EAGAIN:
#if (EAGAIN != EWOULDBLOCK)
        case EWOULDBLOCK: // 合并 EAGAIN 和 EWOULDBLOCK
#endif
            logConnectionError(lastError, "Resource temporarily unavailable");
            break;
            
        case EINTR:
            logConnectionError(lastError, "Interrupted by signal");
            break;
            
        default:  // 不可恢复错误
            logConnectionError(lastError, "Fatal socket error");
            zdClosesocketProc(pConn);
        }
        break;
    }
}

// void HttpServer::onRequest(const muduo::net::TcpConnectionPtr &conn, const HttpRequest &req)
// {
//     const std::string &connection = req.getHeader("Connection");
//     bool close = ((connection == "close") ||
//                   (req.getVersion() == "HTTP/1.0" && connection != "Keep-Alive"));
//     HttpResponse response(close);
    
//     // 处理OPTIONS请求
//     if (req.method() == HttpRequest::kOptions)
//     {
//         response.setStatusLine(req.getVersion(), HttpResponse::k200Ok, "OK");
//         response.addHeader("Access-Control-Allow-Origin", "*");
//         response.addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
//         response.addHeader("Access-Control-Allow-Headers", "Content-Type");
//         response.addHeader("Access-Control-Max-Age", "86400");

//         muduo::net::Buffer buf;
//         response.appendToBuffer(&buf);
//         conn->send(&buf);
//         return;
//     }

//     // 根据请求报文信息来封装响应报文对象
//     httpCallback_(req, &response); // 执行onHttpCallback函数

//     // 可以给response设置一个成员，判断是否请求的是文件，如果是文件设置为true，并且存在文件位置在这里send出去。
//     muduo::net::Buffer buf;
//     response.appendToBuffer(&buf);

//     conn->send(&buf);

//     // 如果是短连接的话，返回响应报文后就断开连接
//     if (response.closeConnection())
//     {
//         conn->shutdown();
//     }
// }

// // 执行请求对应的路由处理函数
// void HttpServer::onHttpCallback(const HttpRequest &req, HttpResponse *resp)
// {
//     if (!router_.route(req, resp))
//     {
//         std::cout << "请求的啥，url：" << req.method() << " " << req.path() << std::endl;
//         std::cout << "未找到路由，返回404" << std::endl;
//         resp->setStatusCode(HttpResponse::k404NotFound);
//         resp->setStatusMessage("Not Found");
//         resp->setCloseConnection(true);
//     }
// }

// http响应未发送完时继续发送的回调
void CSocket::ngx_http_write_request_handler(lpngx_connection_t pConn)
{


todo...............












    // CMemory memory = CMemory::getInstance();

    // ssize_t sendsize = sendproc(pConn, pConn->psendbuf, pConn->isendlen);

    // if(sendsize > 0 && sendsize != pConn->isendlen) // 只发送了部分数据
    // {
    //     pConn->psendbuf = pConn->psendbuf + sendsize;
    //     pConn->isendlen = pConn->isendlen - sendsize;
    //     return;
    // }
    // else if(sendsize == -1)
    // {
    //     //这不太可能，可以发送数据时通知我发送数据，我发送时你却通知我发送缓冲区满？
    //     ngx_log_stderr(errno,"CSocekt::ngx_write_request_handler()时if(sendsize == -1)成立，这很怪异。"); //打印个日志，别的先不干啥
    //     return;
    // }
    // if(sendsize > 0 && sendsize == pConn->isendlen) // 发送完毕
    // {
    //     // 发送成功，从哪个epoll中干掉；
    //     if(ngx_epoll_oper_event(pConn->fd, EPOLL_CTL_DEL, EPOLLOUT, 1, pConn) == -1) // 覆盖epoll中的写事件
    //     {
    //         //有这情况发生？这可比较麻烦，不过先do nothing
    //         ngx_log_stderr(errno,"CSocekt::ngx_write_request_handler()中ngx_epoll_oper_event()失败。");
    //     }
    // }

    // // 要么发送完毕，要么对端端口，开始收尾
    // if(sem_post(&m_semEventSendQueue) == -1)
    // {
    //     ngx_log_stderr(0,"CSocekt::ngx_write_request_handler()中sem_post(&m_semEventSendQueue)失败.");
    // }

    // memory.FreeMemory(pConn->psendMemPointer); // 释放内存
    // pConn->psendMemPointer = NULL; // 重置指针
    // pConn->psendbuf = NULL; // 重置指针
}

}