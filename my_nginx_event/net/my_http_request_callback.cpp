// #include "my_socket.h"
// #include "my_func.h"
// #include "my_memory.h"
// #include <arpa/inet.h>
// #include <cstring>
// #include "my_global.h"
// #include "my_http_context.h"

// namespace WYXB
// {





// // http请求报文到来时的回调
// void CSocket::ngx_http_read_request_handler(lpngx_connection_t pConn)
// {
// //接收数据
//     bool isflood = false; // 是否是flood攻击
//     ngx_log_stderr(errno,"ngx_http_read_request_handler before recvproc" );

//     // char pMsgBuf[2024];
//     // 收包
//     ssize_t reco = recvproc(pConn, pConn->precvbuf, 2024);
//     if(reco <= 0)
//     {
//         return;
//     }



// // 解析http请求
//     try
//     {
//         // HttpContext对象用于解析出buf中的请求报文，并把报文的关键信息封装到HttpRequest对象中
//         std::chrono::system_clock::time_point receiveTime = std::chrono::system_clock::now();
//         std::shared_ptr<HttpContext> context = pConn->getContext();
//         if (!context->parseRequest(pConn->precvbuf, receiveTime)) // 解析一个http请求
//         {
//             // 如果解析http报文过程中出错
//             conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
//             conn->shutdown();
//         }

//         // 如果buf缓冲区中解析出一个完整的数据包才封装响应报文
//         if (context->gotAll())
//         {
//             onRequest(conn, context->request());
//             context->reset();
//         }
//     }
//     catch (const std::exception &e)
//     {
//         // 捕获异常，返回错误信息
//         LOG_ERROR << "Exception in onMessage: " << e.what();
//         conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
//         conn->shutdown();
//     }   
































//     //收到数据
//     auto params = m_httpGetProcessor->parseRequestLine(pConn->precvbuf);
//     std::string tmpstr = m_httpGetProcessor->buildHtmlResponse(params);


//     //合法包头，继续处理
//     //分配内存开始接受
//     CMemory memory = CMemory::getInstance();
//     char* pTmpBuffer = (char*)memory.AllocMemory(m_iLenMsgHeader + tmpstr.size() + 1, false);

//     // a）先填写消息头内容
//     LPSTRUC_MSG_HEADER ptmpMsgHeader = (LPSTRUC_MSG_HEADER)pTmpBuffer;
//     pConn->ishttp = true;
//     ptmpMsgHeader->pConn = pConn;
//     ptmpMsgHeader->iCurrsequence = pConn->iCurrsequence; //收到包时连接池中连接序号记录到消息头中，将来备用
//     // b） 填写包头内容
//     pTmpBuffer += m_iLenMsgHeader; //跳过消息头
//     memcpy(pTmpBuffer, tmpstr.c_str(), tmpstr.size());// 将数据拷贝进来

//     // 加入发送队列
//     g_threadpool.inMsgRecvQueueAndSignal(pTmpBuffer);

// }
    
    

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

// // http响应未发送完时继续发送的回调
// void CSocket::ngx_http_write_request_handler(lpngx_connection_t pConn)
// {








//     // CMemory memory = CMemory::getInstance();

//     // ssize_t sendsize = sendproc(pConn, pConn->psendbuf, pConn->isendlen);

//     // if(sendsize > 0 && sendsize != pConn->isendlen) // 只发送了部分数据
//     // {
//     //     pConn->psendbuf = pConn->psendbuf + sendsize;
//     //     pConn->isendlen = pConn->isendlen - sendsize;
//     //     return;
//     // }
//     // else if(sendsize == -1)
//     // {
//     //     //这不太可能，可以发送数据时通知我发送数据，我发送时你却通知我发送缓冲区满？
//     //     ngx_log_stderr(errno,"CSocekt::ngx_write_request_handler()时if(sendsize == -1)成立，这很怪异。"); //打印个日志，别的先不干啥
//     //     return;
//     // }
//     // if(sendsize > 0 && sendsize == pConn->isendlen) // 发送完毕
//     // {
//     //     // 发送成功，从哪个epoll中干掉；
//     //     if(ngx_epoll_oper_event(pConn->fd, EPOLL_CTL_DEL, EPOLLOUT, 1, pConn) == -1) // 覆盖epoll中的写事件
//     //     {
//     //         //有这情况发生？这可比较麻烦，不过先do nothing
//     //         ngx_log_stderr(errno,"CSocekt::ngx_write_request_handler()中ngx_epoll_oper_event()失败。");
//     //     }
//     // }

//     // // 要么发送完毕，要么对端端口，开始收尾
//     // if(sem_post(&m_semEventSendQueue) == -1)
//     // {
//     //     ngx_log_stderr(0,"CSocekt::ngx_write_request_handler()中sem_post(&m_semEventSendQueue)失败.");
//     // }

//     // memory.FreeMemory(pConn->psendMemPointer); // 释放内存
//     // pConn->psendMemPointer = NULL; // 重置指针
//     // pConn->psendbuf = NULL; // 重置指针
// }


// }



