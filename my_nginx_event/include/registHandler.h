#pragma once
#include "my_slogic.h"
#include <memory>

namespace WYXB
{
class RegistHandler
{
private:
    std::shared_ptr<CLogicSocket> m_pLogicSocket;
public:
    RegistHandler(std::shared_ptr<CLogicSocket> pLogicSocket):  m_pLogicSocket(pLogicSocket){}
    ~RegistHandler() = default;

    // 各种业务逻辑相关函数
    bool HandleRegister(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);
    bool HandleLogin(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);
    bool HandlePing(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength);

    void Regist();
};






}