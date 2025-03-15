#pragma once
#include "my_socket.h"
#include <functional>
#include <unordered_map>
#include "router.h"



namespace WYXB
{
class Router;

class CLogicSocket : public CSocket
{


public:
    CLogicSocket() = default;
    virtual ~CLogicSocket() = default;
    virtual bool Initialize();

public:
    // 解析http请求的函数
    virtual void threadRecvProcFunc(std::vector<uint8_t>&& pMsgBuf);

private:

    std::shared_ptr<Router> m_Router;
    void InitRouter() {
        m_Router = std::make_shared<Router>();
        regist();
    }

    // 注册
    bool regist();

private:
    void onRequest(lpngx_connection_t conn, const HttpRequest &req);
    void handleRequest(const HttpRequest &req, HttpResponse *resp);

private:
    // 通用收发数据相关函数
    void SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader, uint16_t iMsgCode);
    // 心跳包时间到，检测心跳包是否超时
    virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t currtime);

private:
    // // 各种业务逻辑相关函数
    // bool HandleRegister(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);
    // bool HandleLogin(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);
    // bool HandlePing(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);

};



    
}