#include "my_socket.h"
#include "my_func.h"
#include "my_memory.h"
#include <arpa/inet.h>
#include <cstring>
#include "my_global.h"


namespace WYXB
{



// // 来数据的时候处理，当连接上有数据来的时候，本函数会被ngx_epoll_process_events()调用
void CSocket::ngx_read_request_handler(lpngx_connection_t pConn){} //设置数据来时的读回调函数
// {
//     bool isflood = false; // 是否是flood攻击
//     ngx_log_stderr(errno,"CSocekt::ngx_read_request_handler() before recvproc" );
//     // 收包
//     ssize_t reco = recvproc(pConn, pConn->precvbuf, pConn->irecvlen);
//     if(reco <= 0)
//     {
//         return;
//     }

//     //收到数据
//     if(pConn->curStat == _PKG_HD_INIT) // 建立连接时是_PKG_BD_INIT状态，因为ngx_get_connection()时已经设置好了curstat
//     {


//         if(reco == m_iLenPkgHeader) // 收到的数据正好只有一个包头
//         {
//             ngx_wait_request_handler_proc_p1(pConn, isflood);// 收到完整包头后的处理
//         }
//         else
//         {
//             // 收到包头不完整或者不只一个包头【包括包头+包体】
//             pConn->curStat = _PKG_HD_RECVING; // 继续接受包
//             pConn->precvbuf = pConn->precvbuf + reco; // 指针移动
//             pConn->irecvlen = pConn->irecvlen - reco; // 长度减少
        
//         }
//     }
//     else if(pConn->curStat == _PKG_HD_RECVING)
//     {
//         if(reco == pConn->irecvlen) // 收到完整包头
//         {
//             ngx_wait_request_handler_proc_p1(pConn, isflood); // 收到完整包体后的处理
//         }
//         else
//         {
//             // 收到包头不完整
//             pConn->precvbuf = pConn->precvbuf + reco; // 指针移动
//             pConn->irecvlen = pConn->irecvlen - reco; // 长度减少
//         }
    
//     }
//     else if(pConn->curStat == _PKG_BD_INIT)
//     {
//         // 接受包体
//         if(reco == pConn->irecvlen)
//         {
//             // 收到完整包体
//             if(m_floodAkEnable == 1)
//             {
//                 isflood = TestFlood(pConn); // 测试是否是flood攻击
//             }
//             ngx_wait_request_handler_proc_plast(pConn, isflood); // 收到完整包体后的处理
//         }
//         else
//         {
//             //收到的宽度小于要收的宽度
// 			pConn->curStat = _PKG_BD_RECVING;					
// 			pConn->precvbuf = pConn->precvbuf + reco;
// 			pConn->irecvlen = pConn->irecvlen - reco;
//         }
//     }
//     else if(pConn->curStat == _PKG_BD_RECVING)
//     {
//         // 接受包体
//         if(reco == pConn->irecvlen)
//         {
//             //包体收完整了
//             if(m_floodAkEnable == 1) 
//             {
//                 //Flood攻击检测是否开启
//                 isflood = TestFlood(pConn);
//             }
//             ngx_wait_request_handler_proc_plast(pConn,isflood);
//         }
//         else
//         {
//             // 收到的宽度小于要收的宽度
//             pConn->precvbuf = pConn->precvbuf + reco;
//             pConn->irecvlen = pConn->irecvlen - reco;
//         }
//     }
//     if(isflood)
//     {
//         // 处理flood攻击
//         zdClosesocketProc(pConn);
//     }

// }

// ssize_t CSocket::recvproc(lpngx_connection_t pConn, char *pBuf, ssize_t bufLen) //接受从客户端来的数据专用函数
// {
//     ssize_t n;
//     ngx_log_stderr(errno,"CSocekt::recvproc() before recv" );
//     n = recv(pConn->fd, pBuf, bufLen, 0);
//     ngx_log_stderr(errno,"CSocekt::recvproc() size %d！", n);
//     if(n == 0)
//     {
//         // 客户端关闭连接
//         ngx_log_stderr(errno,"CSocekt::recvproc()中客户端关闭连接！");
//         zdClosesocketProc(pConn);
//         return -1;
    
//     }

//     // 客户端没有断开连接
//     if(n < 0) 
//     {
//         if(errno == EAGAIN || errno == EWOULDBLOCK)
//         {
//             ngx_log_stderr(errno,"CSocekt::recvproc()中errno == EAGAIN || errno == EWOULDBLOCK成立，出乎我意料！");
//             return -1; //不当做错误处理，只是简单返回
//         }
//         if(errno == EINTR)
//         {
//             ngx_log_stderr(errno,"CSocekt::recvproc()中errno == EINTR成立，出乎我意料！");
//             return -1; //不当做错误处理，只是简单返回
//         }

//         // 认为出现错误，关闭连接
//         zdClosesocketProc(pConn);
//         return -1;
//     }


//     //此时认为收到数据
//     return n; //返回收到的数据长度
// }

// //包头收完整后的处理，我们称为包处理阶段1【p1】：写成函数，方便复用
// //注意参数isflood是个引用
// void CSocket::ngx_wait_request_handler_proc_p1(lpngx_connection_t pConn, bool &isflood) //处理收到完整包头后的处理
// {
//     CMemory memory = CMemory::getInstance();

//     LPCOMM_PKG_HEADER pPkgHeader;
//     pPkgHeader = (LPCOMM_PKG_HEADER)pConn->dataHeadInfo; // 指针指向包头

//     unsigned short e_pkgLen;
//     e_pkgLen = ntohs(pPkgHeader->pkgLen);

//     //恶意包或错误包
//     if(e_pkgLen < m_iLenPkgHeader) // 收到数据比包头都还小，认为出错
//     {
//         pConn->curStat = _PKG_HD_INIT;
//         pConn->precvbuf = pConn->dataHeadInfo;
//         pConn->irecvlen = m_iLenPkgHeader;
//     }
//     else if(e_pkgLen > (_PKG_MAX_LENGTH-1000)) // 长度过长，超过最大值，认为是恶意包
//     {
//         pConn->curStat = _PKG_HD_INIT;
//         pConn->precvbuf = pConn->dataHeadInfo;
//         pConn->irecvlen = m_iLenPkgHeader;
//     }   
//     else
//     {
//         //合法包头，继续处理
//         //分配内存开始接受
//         char* pTmpBuffer = (char*)memory.AllocMemory(m_iLenMsgHeader + e_pkgLen, false);//分配内存【长度是 消息头长度  + 包头长度 + 包体长度】，最后参数先给false，表示内存不需要memset; 
//         pConn->precvMemPointer = pTmpBuffer;

//         // a）先填写消息头内容
//         LPSTRUC_MSG_HEADER ptmpMsgHeader = (LPSTRUC_MSG_HEADER)pTmpBuffer;
//         ptmpMsgHeader->pConn = pConn;
//         ptmpMsgHeader->iCurrsequence = pConn->iCurrsequence; //收到包时连接池中连接序号记录到消息头中，将来备用
//         // b） 填写包头内容
//         pTmpBuffer += m_iLenMsgHeader; //跳过消息头，指向包头
//         memcpy(pTmpBuffer, pPkgHeader, m_iLenPkgHeader);// 将包头拷贝进来
//         if(e_pkgLen == m_iLenPkgHeader) // 这条数据只有包头，只处理包头即可
//         {
//             //接收包头就相当于完成接收
//             if(m_floodAkEnable == 1)
//             {
//                 isflood = TestFlood(pConn); // 测试是否是flood攻击
//             }
//             // 放入消息队列，等待后序业务逻辑线程处理
//             ngx_wait_request_handler_proc_plast(pConn, isflood);
//         }
//         else // 继续处理包体数据
//         {
//             pConn->curStat = _PKG_BD_INIT; // 切换状态
//             pConn->precvbuf = pTmpBuffer + m_iLenPkgHeader; // 指针指向包体
//             pConn->irecvlen = e_pkgLen - m_iLenPkgHeader; // 记录包体长度
//         }
//     }
// }

// // 将收到的数据放入消息队列，等待后序业务逻辑线程处理
// void CSocket::ngx_wait_request_handler_proc_plast(lpngx_connection_t pConn, bool &isflood) //处理收到完整包体后的处理
// {
//     if(isflood == false)
//     {
//         g_threadpool.inMsgRecvQueueAndSignal(pConn->precvMemPointer);

//     }
//     else{
//         // 处理flood攻击
//         CMemory memory = CMemory::getInstance();
//         memory.FreeMemory(pConn->precvMemPointer);
//     }

//     pConn->precvMemPointer = NULL; // 释放内存
//     pConn->curStat = _PKG_HD_INIT; //恢复收包状态，为下一次收包做准备
//     pConn->precvbuf = pConn->dataHeadInfo; // 初始化收包位置
//     pConn->irecvlen = m_iLenPkgHeader; // 初始化收包长度


// }

// // 发送数据专用函数，返回发送的字节数
// // // 返回=0，对方中断； 返回=-1，errno == EAGAIN，对方缓冲区满了；返回-2，errno ！= EAGAIN ！= EWOULDBLOCK ！= EINTR，也认为对端断开
// ssize_t CSocket::sendproc(lpngx_connection_t c, Buffer buff)// 将数据发送到客户端
// {
//     ssize_t n;
//     for(;;)
//     {
//         n = send(c->fd, buff.peek(), buff.readableBytes(), 0);
//         if(n > 0) // 发送成功
//         {
//             return n;
//         }
//         if(n == 0)
//         {
//             return 0; // 对方关闭连接
//         }
//         if(errno == EINTR) // 内核缓冲区满了
//         {
//             return -1;

//         }
//         if(errno == EINTR)
//         {
//             // 收到某个信号，不认为出错
//             // 仅打印日志
//             ngx_log_stderr(errno,"CSocekt::sendproc()中send()失败.");
//         }
//         else
//         {
//             // 出错，但是不断开socket，等待recv来统一处理断开，因为多线程时处理send和recv断开不容易
//             return -2;
//         }
//     }
// }


// // 设置数据发送时的写处理函数，当epoll通知我们时，我们在 int CSocekt::ngx_epoll_process_events(int timer)  中调用此函数
void CSocket::ngx_write_response_handler(lpngx_connection_t pConn){}
// {
//     CMemory memory = CMemory::getInstance();

//     ssize_t sendsize = sendproc(pConn, pConn->psendbuf, pConn->isendlen);

//     if(sendsize > 0 && sendsize != pConn->isendlen) // 只发送了部分数据
//     {
//         pConn->psendbuf = pConn->psendbuf + sendsize;
//         pConn->isendlen = pConn->isendlen - sendsize;
//         return;
//     }
//     else if(sendsize == -1)
//     {
//         //这不太可能，可以发送数据时通知我发送数据，我发送时你却通知我发送缓冲区满？
//         ngx_log_stderr(errno,"CSocekt::ngx_write_request_handler()时if(sendsize == -1)成立，这很怪异。"); //打印个日志，别的先不干啥
//         return;
//     }
//     if(sendsize > 0 && sendsize == pConn->isendlen) // 发送完毕
//     {
//         // 发送成功，从哪个epoll中干掉；
//         if(ngx_epoll_oper_event(pConn->fd, EPOLL_CTL_DEL, EPOLLOUT, 1, pConn) == -1) // 覆盖epoll中的写事件
//         {
//             //有这情况发生？这可比较麻烦，不过先do nothing
//             ngx_log_stderr(errno,"CSocekt::ngx_write_request_handler()中ngx_epoll_oper_event()失败。");
//         }
//     }

//     // 要么发送完毕，要么对端端口，开始收尾
//     if(sem_post(&m_semEventSendQueue) == -1)
//     {
//         ngx_log_stderr(0,"CSocekt::ngx_write_request_handler()中sem_post(&m_semEventSendQueue)失败.");
//     }

//     memory.FreeMemory(pConn->psendMemPointer); // 释放内存
//     pConn->psendMemPointer = NULL; // 重置指针
//     pConn->psendbuf = NULL; // 重置指针
    
// }
// //消息本身格式【消息头+包头+包体】 
// void CSocket::threadRecvProcFunc(char *pMsgBuf)
// {   
//     return;
// }



}