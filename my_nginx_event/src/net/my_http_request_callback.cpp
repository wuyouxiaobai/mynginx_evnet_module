#include "my_socket.h"
#include "my_func.h"
#include "my_memory.h"
#include <arpa/inet.h>
#include <cstring>
#include "my_global.h"
#include "my_http_context.h"
#include "my_server.h"

namespace WYXB {

// HTTP 请求到来时的回调处理函数
void CSocket::ngx_http_read_request_handler(lpngx_connection_t pConn) {
    auto logConnectionError = [pConn](int errcode, const char* msg) {
        char ip[INET6_ADDRSTRLEN] = "unknown";
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

        Logger::ngx_log_stderr(errcode, "%s [%s]:%d (family:%d)", msg, ip, port, pConn->s_sockaddr.sa_family);
    };

    std::vector<uint8_t> buffer(1024 * 1024);
    const ssize_t n = recv(pConn->fd, buffer.data(), buffer.size(), 0);

    if (n > 0) {
        buffer.resize(n);
        Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0, "recv msg %s", buffer.data());
        STRUC_MSG_HEADER msghead;
        msghead.iCurrsequence = pConn->iCurrsequence;
        msghead.pConn = pConn;
        msghead.sequence = pConn->sequence++;
        Server::instance().g_threadpool->inMsgRecvQueueAndSignal(msghead, buffer);
        return;
    }

    const int lastError = errno;
    switch (n) {
        case 0:
            logConnectionError(lastError, "Connection closed by client");
            zdClosesocketProc(pConn);
            break;
        case -1:
            switch (lastError) {
                case EAGAIN:
#if (EAGAIN != EWOULDBLOCK)
                case EWOULDBLOCK:
#endif
                    logConnectionError(lastError, "Resource temporarily unavailable");
                    break;
                case EINTR:
                    logConnectionError(lastError, "Interrupted by signal");
                    break;
                default:
                    logConnectionError(lastError, "Fatal socket error");
                    zdClosesocketProc(pConn);
                    break;
            }
            break;
    }
}

// HTTP 响应写入时的回调函数
void CSocket::ngx_http_write_response_handler(lpngx_connection_t pConn) {
    if (!pConn || pConn->fd == -1 || pConn->psendbuf.readableBytes() == 0 || pConn->iCurrsequence != pConn->sendCount) {
        Logger::ngx_log_stderr(0, "非法连接或空发送缓冲区");
        return;
    }

    while (true) {
        ssize_t sendsize = sendproc(pConn, pConn->psendbuf);
        Logger::ngx_log_error_core(NGX_LOG_INFO, 0, "ngx_http_write_response_handler 发送数据长度：%d, 发送数据: %s", sendsize, pConn->psendbuf.peek());

        if (sendsize > 0) {
            pConn->psendbuf.retrieve(sendsize);
            if (pConn->psendbuf.readableBytes() == 0) {
                Logger::ngx_log_stderr(errno, "发送成功");
                if (ngx_epoll_oper_event(pConn->fd, EPOLL_CTL_MOD, EPOLLOUT, 1, pConn.get()) == -1) {
                    Logger::ngx_log_stderr(errno, "epoll事件修改失败");
                }
                if (pConn->ishttpClose) {
                    Logger::ngx_log_stderr(0, "服务器主动断开连接 fd=%d", pConn->fd);
                    zdClosesocketProc(pConn);
                }
                break;
            }
        } else if (sendsize == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                ngx_epoll_oper_event(pConn->fd, EPOLL_CTL_MOD, EPOLLOUT, 0, pConn.get());
                Logger::ngx_log_stderr(0, "发送缓冲区满，等待再次发送 fd=%d", pConn->fd);
            } else if (errno == EPIPE || errno == ECONNRESET) {
                Logger::ngx_log_stderr(errno, "连接重置，关闭 fd=%d", pConn->fd);
                zdClosesocketProc(pConn);
            } else {
                Logger::ngx_log_stderr(0, "未知错误，关闭 fd=%d", pConn->fd);
                zdClosesocketProc(pConn);
            }
            break;
        } else {
            Logger::ngx_log_stderr(0, "未知错误，断开连接 fd=%d", pConn->fd);
            zdClosesocketProc(pConn);
            break;
        }
    }
}

// 实际执行发送操作
ssize_t CSocket::sendproc(lpngx_connection_t c, Buffer buff) {
    ssize_t n;
    for (;;) {
        Logger::ngx_log_stderr(0, "sendproc fd: %d", c->fd);

        n = send(c->fd, buff.peek(), buff.readableBytes(), MSG_NOSIGNAL);
        if (n > 0) return n;
        if (n == 0) return 0;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
        if (errno == EINTR) {
            Logger::ngx_log_stderr(errno, "sendproc中断");
            continue;
        }
        if (errno == EPIPE || errno == ECONNRESET) return -1;

        Logger::ngx_log_stderr(errno, "sendproc失败");
        return -2;
    }
}

} // namespace WYXB
