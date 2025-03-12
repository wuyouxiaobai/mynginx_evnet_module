#include "router.h"
#include "my_memory.h"
#include <memory.h>
#include <netinet/in.h>
#include "my_logiccomm.h"
#include "my_crc32.h"
#include <functional>


namespace WYXB
{


void Router::registerHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler)
{
    RouteKey key{method, path};
    handlers_[key] = std::move(handler);
}

void Router::registerCallback(HttpRequest::Method method, const std::string &path, const HandlerCallback &callback)
{
    RouteKey key{method, path};
    callbacks_[key] = std::move(callback);
}

bool Router::route(const HttpRequest &req, HttpResponse *resp)
{
    RouteKey key{req.method(), req.path()};

    // 查找处理器
    auto handlerIt = handlers_.find(key);
    if (handlerIt != handlers_.end())
    {
        handlerIt->second->handle(req, resp);
        return true;
    }

    // 查找回调函数
    auto callbackIt = callbacks_.find(key);
    if (callbackIt != callbacks_.end())
    {
        callbackIt->second(req, resp);
        return true;
    }

    // 查找动态路由处理器
    for (const auto &[method, pathRegex, handler] : regexHandlers_)
    {
        std::smatch match;
        std::string pathStr(req.path());
        // 如果方法匹配并且动态路由匹配，则执行处理器
        if (method == req.method() && std::regex_match(pathStr, match, pathRegex))
        {
            // Extract path parameters and add them to the request
            HttpRequest newReq(req); // 因为这里需要用这一次所以是可以改的
            extractPathParameters(match, newReq);
            
            handler->handle(newReq, resp);
            return true;
        }
    }

    // 查找动态路由回调函数
    for (const auto &[method, pathRegex, callback] : regexCallbacks_)
    {
        std::smatch match;
        std::string pathStr(req.path());
        // 如果方法匹配并且动态路由匹配，则执行回调函数
        if (method == req.method() && std::regex_match(pathStr, match, pathRegex))
        {
                // Extract path parameters and add them to the request
            HttpRequest newReq(req); // 因为这里需要用这一次所以是可以改的
            extractPathParameters(match, newReq);

            callback(req, resp);
            return true;
        }
    }

    return false;
}

























// // 注册函数
// void RegistHandler::RegisterHandle(uint16_t msgCode, HandlerFunc handler) {
//     std::lock_guard<std::mutex> lock(m_handlerMutex);
//     m_handlerMap[msgCode] = handler;
// }


// void RegistHandler::Regist()
// {
//     // 获取并验证智能指针
//     if (auto pSocket = m_pLogicSocket.lock()) 
//     {
//         RegisterHandle(_CMD_PING, std::bind(&HandleRegister, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
//         RegisterHandle(_CMD_REGISTER, std::bind(&RegistHandler::RegisterCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
//         RegisterHandle(_CMD_LOGIN, std::bind(&RegistHandler::RegisterCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
//     }

}





















// // 根据GET这些调用对应回调函数（web）
// // 或者根据对应注册码调用对应回调函数（Tcp）
// static bool HandleRegister(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength)
// {
//     //(1)首先判断包体的合法性
//     if(pPkgBody == nullptr)
//     {
//         return false;
//     }
//     int iRecvLen = sizeof(STRUCT_REGISTER);
//     if(iRecvLen != iBodyLength) // 发送结构体大小不对，认为是恶意包
//     {
//         return false;
//     }

//     std::lock_guard<std::mutex> lock(pConn->logicPorcMutex);

//     //(3)取得了整个发送过来的数据
//     LPSTRUCT_REGISTER p_RecvInfo = (LPSTRUCT_REGISTER)pPkgBody;
//     p_RecvInfo->iType = ntohl(p_RecvInfo->iType);
//     p_RecvInfo->username[sizeof(p_RecvInfo->username)-1] = 0;
//     p_RecvInfo->password[sizeof(p_RecvInfo->password)-1] = 0;

//     //(4)这里可能要考虑 根据业务逻辑，进一步判断收到的数据的合法性，
//     //当前该玩家的状态是否适合收到这个数据等等【比如如果用户没登陆，它就不适合购买物品等等】
//     // --------后序处理用户注册逻辑--------
    
//     //(5)给客户端返回数据时，一般也是返回一个结构，这个结构内容具体由客户端/服务器协商，这里我们就以给客户端也返回同样的 STRUCT_REGISTER 结构来举例    
//     //LPSTRUCT_REGISTER pFromPkgHeader =  (LPSTRUCT_REGISTER)(((char *)pMsgHeader)+m_iLenMsgHeader);	//指向收到的包的包头，其中数据后续可能要用到
//     LPCOMM_PKG_HEADER pPkgHeader;
//     CMemory memory = CMemory::getInstance();
//     CRC32 crc32 = CRC32::getInstance();
//     int iSendLen = sizeof(STRUCT_REGISTER);
//     // a)分配要发送出去包的内存
//     char* p_sendbuf = (char*)memory.AllocMemory(m_pLogicSocket->m_iLenMsgHeader+m_pLogicSocket->m_iLenPkgHeader+iSendLen, false);// 消息头+包头+包体
//     // b)填充消息头
//     memcpy(p_sendbuf, pMsgHeader, m_pLogicSocket->m_iLenMsgHeader);
//     // c)填充包头
//     pPkgHeader = (LPCOMM_PKG_HEADER)(p_sendbuf+m_pLogicSocket->m_iLenMsgHeader); // 指向包头
//     pPkgHeader->msgCode = _CMD_REGISTER; // 消息代码
//     pPkgHeader->msgCode = htons(pPkgHeader->msgCode);
//     pPkgHeader->pkgLen = htons(m_pLogicSocket->m_iLenPkgHeader+iSendLen); // 整个包的尺寸【包头+包体】
//     // d)填充包体
//     LPSTRUCT_REGISTER p_sendInfo = (LPSTRUCT_REGISTER)(p_sendbuf+m_pLogicSocket->m_iLenMsgHeader+m_pLogicSocket->m_iLenPkgHeader); // 指向包体
//     // e)包体内容全部确定好后，计算包体crc32
//     pPkgHeader->crc32 = crc32.Get_CRC((char*)p_sendInfo, iSendLen); // 计算整个包体的crc32
//     pPkgHeader->crc32 = htonl(pPkgHeader->crc32); // 网络字节序转换
//     //。。。。。这里根据需要，填充要发回给客户端的内容,int类型要使用htonl()转，short类型要使用htons()转；
    
//     // f)发送数据
//     m_pLogicSocket->msgSend(p_sendbuf);

//     return true;


// }

// bool RegistHandler::HandleLogin(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength)
// {
//     if(pPkgBody == nullptr)
//     {
//         return false;
//     }
//     int iRecvLen = sizeof(STRUCT_LOGIN);
//     if(iRecvLen != iBodyLength)
//     {
//         return false;
//     }
//     std::lock_guard<std::mutex> lock(pConn->logicPorcMutex);

//     LPSTRUCT_LOGIN p_RecvInfo = (LPSTRUCT_LOGIN)pPkgBody;
//     p_RecvInfo->username[sizeof(p_RecvInfo->username)-1]=0;
//     p_RecvInfo->password[sizeof(p_RecvInfo->password)-1]=0;
//     //--------后序处理用户登录逻辑--------

//     //给客户端返回数据
//     LPCOMM_PKG_HEADER pPkgHeader;
// 	CMemory  p_memory = CMemory::getInstance();
// 	CRC32   p_crc32 = CRC32::getInstance();

//     int iSendLen = sizeof(STRUCT_LOGIN);
//     char* p_sendbuf = (char*)p_memory.AllocMemory(m_pLogicSocket->m_iLenMsgHeader+m_pLogicSocket->m_iLenPkgHeader+iSendLen,false);  
//     memcpy(p_sendbuf,pMsgHeader,m_pLogicSocket->m_iLenMsgHeader);
//     pPkgHeader = (LPCOMM_PKG_HEADER)(p_sendbuf+m_pLogicSocket->m_iLenMsgHeader);
//     pPkgHeader->msgCode = _CMD_LOGIN;
//     pPkgHeader->msgCode = htons(pPkgHeader->msgCode);
//     pPkgHeader->pkgLen  = htons(m_pLogicSocket->m_iLenPkgHeader + iSendLen);   
//     LPSTRUCT_LOGIN p_sendInfo = (LPSTRUCT_LOGIN)(p_sendbuf+m_pLogicSocket->m_iLenMsgHeader+m_pLogicSocket->m_iLenPkgHeader);
//     pPkgHeader->crc32 = p_crc32.Get_CRC((char*)p_sendInfo, iSendLen);
//     pPkgHeader->crc32 = htonl(pPkgHeader->crc32);
//     //。。。。。这里根据需要，填充要发回给客户端的内容,int类型要使用htonl()转，short类型要使用htons()转；

//     m_pLogicSocket->msgSend(p_sendbuf);
//     return true;
// }

// bool RegistHandler::HandlePing(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength)
// {
//     if(iBodyLength != 0)
//     {
//         return false;
//     }

//     std::lock_guard<std::mutex> lock(pConn->logicPorcMutex);
//     pConn->lastPingTime = time(nullptr);

//     //服务器也发送 一个只有包头的数据包给客户端，作为返回的数据
//     m_pLogicSocket->SendNoBodyPkgToClient(pMsgHeader,_CMD_PING);

//     return true;
// }

