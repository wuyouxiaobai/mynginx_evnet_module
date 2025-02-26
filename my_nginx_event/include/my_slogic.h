#pragma once
#include "my_socket.h"

namespace WYXB
{

class CLogicSocket : public CSocket
{
public:
    CLogicSocket();
    virtual ~CLogicSocket();
    virtual bool Initialize();

public:

    // 通用收发数据相关函数
    void SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader, uint16_t iMsgCode);

    // 各种业务逻辑相关函数
    bool HandleRegister(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);
    bool HandleLogin(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);
    bool HandlePing(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);

    virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t currtime); // 心跳包时间到，检测心跳包是否超时

public:
    virtual void threadRecvProFunc(char* pMsgBuf);



};



    
}