#include "my_logiccomm.h"
#include "my_slogic.h"
#include "my_comm.h"
#include "arpa/inet.h"
#include "my_crc32.h"
#include "my_func.h"
#include "my_memory.h"
#include <cstring>
#include "my_global.h"

namespace WYXB
{
// //定义成员函数指针
// typedef bool (CLogicSocket::*handler)(  lpngx_connection_t pConn, //连接池中指针
//                                         LPSTRUC_MSG_HEADER pMsgHeader, // 消息头
//                                         char* pPkgBody, // 包体
//                                         uint16_t iBodyLength // 包体长度
//                                     );    

// // 用来保存成员函数指针的数组
// static const handler statusHandler[] = 
// {
//     //数组前5个元素，保留，以备将来增加一些基本服务器功能
//     &CLogicSocket::HandlePing,//【0】：心跳包的实现
//     nullptr,nullptr,nullptr,nullptr,
//     &CLogicSocket::HandleRegister,//【5】：实现具体的注册功能
//     &CLogicSocket::HandleLogin//【6】：实现具体的登录功能

//     //......其他待扩展，比如实现攻击功能，实现加血功能等等；
// };

// #define AUTH_TOTAL_COMMONS (sizeof(statusHandler) / sizeof(handler))//整个命令有多少个，编译时即可知道

// CLogicSocket::CLogicSocket()
// {
//     m_pRegistHandler = std::make_shared<RegistHandler>(RegistHandler(std::make_shared<CLogicSocket>(this)));
// }
// CLogicSocket::~CLogicSocket() = default;

//初始化函数【fork()子进程之前干这个事】
//成功返回true，失败返回false
bool CLogicSocket::Initialize()
{
    //做一些和本类相关的初始化工作
    //....日后根据需要扩展        
    bool bParentInit = CSocket::Initialize();  //调用父类的同名函数
    InitRouter();
    return bParentInit;
}

// 根据需求注册对应业务函数
bool CLogicSocket::regist()
{
// 示例：注册动态路径心跳
    m_Router->addRegexCallback(
        HttpRequest::Method::kGet, 
        "/heartbeat/\\d+",  // 匹配类似 /heartbeat/123
        [](const HttpRequest& req, HttpResponse* resp) {
            resp->setStatusCode(HttpResponse::HttpStatusCode::k200Ok);
            resp->setBody("Dynamic PONG");
        }
    );
}


//处理接收消息队列中的消息，由线程池调用
void CLogicSocket::threadRecvProFunc(std::vector<uint8_t>&& pMsgBuf)
{
    // 检查缓冲区大小是否至少包含头部
    if (pMsgBuf.size() < sizeof(STRUC_MSG_HEADER)) {
        throw std::runtime_error("Buffer size is smaller than the header size.");
    }

    // 提取头部
    STRUC_MSG_HEADER header;
    std::memcpy(&header, pMsgBuf.data(), sizeof(STRUC_MSG_HEADER));

    // 提取数据体
    size_t headerSize = sizeof(STRUC_MSG_HEADER);
    size_t dataSize = pMsgBuf.size() - headerSize;

    if (dataSize > 0) {
        std::string dataBody(pMsgBuf.begin() + headerSize, pMsgBuf.end());
        bool isErr = false; // 判断是否解析失败
        lpngx_connection_t headptr =  header.pConn.lock();
        if(!headptr) // 连接已经断开
        {
            return;
        }
        bool ok = headptr->context_->parseRequest(dataBody, isErr, std::chrono::system_clock::now()); // 判断是否解析完成
        if(!ok) // 未完成解析
        {
            if(!isErr) return; // 解析过程没有出错，直接返回

            msgSend("HTTP/1.1 400 Bad Request\r\n\r\n"); // 返回错误响应
            return;

        }
        // 成功解析，处理数据体  
        onRequest(headptr, headptr->context_->request());
        headptr->context_->reset();

    } else {
        std::cout << "No data body present." << std::endl;
    }








    // LPSTRUC_MSG_HEADER pMsgHeader = (LPSTRUC_MSG_HEADER)(pMsgBuf); // 消息头
    // void* pPkgBody; // 指向包体的指针
    // uint16_t pkglen = ntohs(pPkgHeader->pkgLen); // 客户端指明的包宽度

    // if(m_iLenPkgHeader == pkglen)
    // {
    //     // 没有包体，只有包头
    //     if(pPkgHeader->crc32 != 0)//只有包头的crc值给0
    //     {
    //         return;//crc错，直接丢弃
    //     }
    //     pPkgBody = NULL;
    // }
    // else
    // {
    //     // 有包体
    //     pPkgHeader->crc32 = ntohl(pPkgHeader->crc32);//针对4字节的数据，网络序转主机序
    //     pPkgBody = (void*)(pMsgBuf+m_iLenMsgHeader+m_iLenPkgHeader);

    //     int calccrc = CRC32::getInstance().Get_CRC((char*)pPkgBody, pkglen-m_iLenPkgHeader);//计算纯包体的crc值
    //     if(calccrc != pPkgHeader->crc32) //服务器端根据包体计算crc值，和客户端传递过来的包头中的crc32信息比较
    //     {
    //         ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中CRC错误[服务器:%d/客户端:%d]，丢弃数据!",calccrc,pPkgHeader->crc32);    //正式代码中可以干掉这个信息
	// 		return; //crc错，直接丢弃
    //     }
    //     else
    //     {
    //         //ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中CRC正确[服务器:%d/客户端:%d]，不错!",calccrc,pPkgHeader->crc32);
    //     }
    // }

    // uint16_t imsgCode = ntohs(pPkgHeader->msgCode);
    // lpngx_connection_t p_Conn = pMsgHeader->pConn;

    // if(p_Conn->iCurrsequence != pMsgHeader->iCurrsequence)
    // {
    //     return;//丢弃不理这种包了
    // } 

    // // if(imsgCode >= AUTH_TOTAL_COMMONS) // 如果收到的消息类型超出了我们预设的范围
    // // {
    // //     ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码不对!",imsgCode); //这种有恶意倾向或者错误倾向的包，希望打印出来看看是谁干的
    // //     return; //丢弃不理这种包【恶意包或者错误包】
    // // }

    // std::function<bool(lpngx_connection_t, LPSTRUC_MSG_HEADER, char*, uint16_t)> handler;
    // {
    //    std::lock_guard<std::mutex> lock(m_handlerMutex);
    //    auto it = m_handlerMap.find(imsgCode);
    //    if(it == m_handlerMap.end()) {
    //        ngx_log_stderr(0, "未注册的消息码: %d", imsgCode);
    //        return;
    //    }
    //    handler = it->second;
    // }

    // // if(statusHandler[imsgCode] == nullptr)//这种用imsgCode的方式可以使查找要执行的成员函数效率特别高
    // // {
    // //     ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码找不到对应的处理函数!",imsgCode); //这种有恶意倾向或者错误倾向的包，希望打印出来看看是谁干的
    // //     return;  //没有相关的处理函数
    // // }

    // // (this->*statusHandler[imsgCode])(p_Conn, pMsgHeader, (char*)pPkgBody, pkglen-m_iLenMsgHeader); //执行对应回调函数

    // handler(p_Conn, pMsgHeader, (char*)pPkgBody, pkglen-m_iLenMsgHeader);
    // return;
}



void CLogicSocket::onRequest(lpngx_connection_t conn, const HttpRequest &req)
{
    const std::string &connection = req.getHeader("Connection");
    bool close = ((connection == "close") ||
                  (req.getVersion() == "HTTP/1.0" && connection != "Keep-Alive"));
    HttpResponse response(close);

    // 根据请求报文信息来封装响应报文对象
    handleRequest(req, &response); 

    // 可以给response设置一个成员，判断是否请求的是文件，如果是文件设置为true，并且存在文件位置在这里send出去。
    // muduo::net::Buffer buf;
    std::string buf;
    response.appendToBuffer(buf);
    // 打印完整的响应内容用于调试
    // LOG_INFO << "Sending response:\n" << buf.toStringPiece().as_string();

    msgSend(buf);
    // 如果是短连接的话，返回响应报文后就断开连接
    if (response.closeConnection())
    {
        zdClosesocketProc(conn);
    }
}

// 执行请求对应的路由处理函数
void CLogicSocket::handleRequest(const HttpRequest &req, HttpResponse *resp)
{
    try
    {
        // 处理请求前的中间件
        HttpRequest mutableReq = req;
        // middlewareChain_.processBefore(mutableReq);

        // 路由处理
        if (!m_Router->route(mutableReq, resp))
        {
            // LOG_INFO << "请求的啥，url：" << req.method() << " " << req.path();
            // LOG_INFO << "未找到路由，返回404";
            resp->setStatusCode(HttpResponse::k404NotFound);
            resp->setStatusMessage("Not Found");
            resp->setCloseConnection(true);
        }

        // 处理响应后的中间件
        // middlewareChain_.processAfter(*resp);
    }
    catch (const HttpResponse& res) 
    {
        // 处理中间件抛出的响应（如CORS预检请求）
        *resp = res;
    }
    catch (const std::exception& e) 
    {
        // 错误处理
        resp->setStatusCode(HttpResponse::k500InternalServerError);
        resp->setBody(e.what());
    }
}



























// 心跳包检测时间到，该去检测心跳包是否超时的事宜，本函数是子类函数，实现具体的判断动作
void CLogicSocket::procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t cur_time)
{
    CMemory memory = CMemory::getInstance();
    if(tmpmsg->iCurrsequence == tmpmsg->pConn->iCurrsequence)//此连接没断
    {
        lpngx_connection_t p_Conn = tmpmsg->pConn;
        if(m_ifTimeOutKick == 1)
        {
            zdClosesocketProc(p_Conn);
        }
        else if((cur_time - p_Conn->lastPingTime) > (m_iWaitTime*3+10))//超时踢的判断标准就是 每次检查的时间间隔*3，超过这个时间没发送心跳包，就踢【大家可以根据实际情况自由设定】
        {
            zdClosesocketProc(p_Conn);
        }
        memory.FreeMemory(tmpmsg);

    }
    else
    {
        memory.FreeMemory(tmpmsg);
    }
    return;
}






// 发送没有包体的数据包给客户端
void CLogicSocket::SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader, uint16_t iMsgCode)
{
    CMemory memory = CMemory::getInstance();

    char* p_sendbuf = (char*)memory.AllocMemory(m_iLenMsgHeader+m_iLenPkgHeader, false);
    char* p_tmpbuf = p_sendbuf;

    memcpy(p_tmpbuf, pMsgHeader, m_iLenMsgHeader);
    p_tmpbuf += m_iLenMsgHeader;

    LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)p_tmpbuf; // 指向要发出去的包头
    pPkgHeader->msgCode = htons(iMsgCode);
    pPkgHeader->pkgLen = htonl(m_iLenPkgHeader);
    pPkgHeader->crc32 = 0;
    msgSend(p_sendbuf);
    return;



}