#pragma once
#include "my_socket.h"
#include <functional>
#include <unordered_map>
#include <registHandler.h>

namespace WYXB
{

class CLogicSocket : public CSocket, public std::enable_shared_from_this<CLogicSocket>
{
friend RegistHandler;

public:
    CLogicSocket();
    virtual ~CLogicSocket();
    virtual bool Initialize();

private:
    // 状态机核心数据结构
    using HandlerFunc = std::function<bool(
        lpngx_connection_t, 
        LPSTRUC_MSG_HEADER, 
        char*, 
        uint16_t
    )>;

    std::unordered_map<uint16_t, HandlerFunc> m_handlerMap; // 消息码到处理函数的映射
    std::mutex m_handlerMutex; // 保证线程安全
    std::shared_ptr<RegistHandler> m_pRegistHandler;

public:
    // 新增动态注册方法
    void RegisterHandle(uint16_t msgCode, HandlerFunc handler) ;
    
    // 通用收发数据相关函数
    void SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader, uint16_t iMsgCode);

    // // 各种业务逻辑相关函数
    // bool HandleRegister(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);
    // bool HandleLogin(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);
    // bool HandlePing(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);

    virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t currtime); // 心跳包时间到，检测心跳包是否超时

public:
    virtual void threadRecvProFunc(char* pMsgBuf);


};



    
}