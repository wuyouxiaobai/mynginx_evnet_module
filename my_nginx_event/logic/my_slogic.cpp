#include "my_logiccomm.h"
#include "my_slogic.h"
#include "my_comm.h"
#include "arpa/inet.h"
#include "my_crc32.h"
#include "my_func.h"
#include "my_memory.h"
#include <cstring>

namespace WYXB
{
//定义成员函数指针
typedef bool (CLogicSocket::*handler)(  lpngx_connection_t pConn, //连接池中指针
                                        LPSTRUC_MSG_HEADER pMsgHeader, // 消息头
                                        char* pPkgBody, // 包体
                                        uint16_t iBodyLength // 包体长度
                                    );    

// 用来保存成员函数指针的数组
static const handler statusHandler[] = 
{
    //数组前5个元素，保留，以备将来增加一些基本服务器功能
    &CLogicSocket::HandlePing,//【0】：心跳包的实现
    nullptr,nullptr,nullptr,nullptr,
    &CLogicSocket::HandleRegister,//【5】：实现具体的注册功能
    &CLogicSocket::HandleLogin//【6】：实现具体的登录功能

    //......其他待扩展，比如实现攻击功能，实现加血功能等等；
};

#define AUTH_TOTAL_COMMONS (sizeof(statusHandler) / sizeof(handler))//整个命令有多少个，编译时即可知道

CLogicSocket::CLogicSocket() = default;
CLogicSocket::~CLogicSocket() = default;

//初始化函数【fork()子进程之前干这个事】
//成功返回true，失败返回false
bool CLogicSocket::Initialize()
{
    //做一些和本类相关的初始化工作
    //....日后根据需要扩展        
    bool bParentInit = CSocket::Initialize();  //调用父类的同名函数
    return bParentInit;
}



//处理收到的数据包，由线程池来调用本函数，本函数是一个单独的线程；
void CLogicSocket::threadRecvProFunc(char* pMsgBuf)
{
    LPSTRUC_MSG_HEADER pMsgHeader = (LPSTRUC_MSG_HEADER)(pMsgBuf); // 消息头
    LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)(pMsgBuf + m_iLenMsgHeader); // 包头
    void* pPkgBody; // 指向包体的指针
    uint16_t pkglen = ntohs(pPkgHeader->pkgLen); // 客户端指明的包宽度

    if(m_iLenPkgHeader == pkglen)
    {
        // 没有包体，只有包头
        if(pPkgHeader->crc32 != 0)//只有包头的crc值给0
        {
            return;//crc错，直接丢弃
        }
        pPkgBody = NULL;
    }
    else
    {
        // 有包体
        pPkgHeader->crc32 = ntohl(pPkgHeader->crc32);//针对4字节的数据，网络序转主机序
        pPkgBody = (void*)(pMsgBuf+m_iLenMsgHeader+m_iLenPkgHeader);

        int calccrc = CRC32::getInstance().Get_CRC((char*)pPkgBody, pkglen-m_iLenPkgHeader);//计算纯包体的crc值
        if(calccrc != pPkgHeader->crc32) //服务器端根据包体计算crc值，和客户端传递过来的包头中的crc32信息比较
        {
            ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中CRC错误[服务器:%d/客户端:%d]，丢弃数据!",calccrc,pPkgHeader->crc32);    //正式代码中可以干掉这个信息
			return; //crc错，直接丢弃
        }
        else
        {
            //ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中CRC正确[服务器:%d/客户端:%d]，不错!",calccrc,pPkgHeader->crc32);
        }
    }

    uint16_t imsgCode = ntohs(pPkgHeader->msgCode);
    lpngx_connection_t p_Conn = pMsgHeader->pConn;

    if(p_Conn->iCurrsequence != pMsgHeader->iCurrsequence)
    {
        return;//丢弃不理这种包了
    } 

    if(imsgCode >= AUTH_TOTAL_COMMONS) // 如果收到的消息类型超出了我们预设的范围
    {
        ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码不对!",imsgCode); //这种有恶意倾向或者错误倾向的包，希望打印出来看看是谁干的
        return; //丢弃不理这种包【恶意包或者错误包】
    }

    if(statusHandler[imsgCode] == nullptr)//这种用imsgCode的方式可以使查找要执行的成员函数效率特别高
    {
        ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码找不到对应的处理函数!",imsgCode); //这种有恶意倾向或者错误倾向的包，希望打印出来看看是谁干的
        return;  //没有相关的处理函数
    }

    (this->*statusHandler[imsgCode])(p_Conn, pMsgHeader, (char*)pPkgBody, pkglen-m_iLenMsgHeader); //执行对应回调函数
    return;
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


// 处理业务的逻辑
bool CLogicSocket::HandleRegister(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength)
{
    //(1)首先判断包体的合法性
    if(pPkgBody == nullptr)
    {
        return false;
    }
    int iRecvLen = sizeof(STRUCT_REGISTER);
    if(iRecvLen != iBodyLength) // 发送结构体大小不对，认为是恶意包
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(pConn->logicPorcMutex);

    //(3)取得了整个发送过来的数据
    LPSTRUCT_REGISTER p_RecvInfo = (LPSTRUCT_REGISTER)pPkgBody;
    p_RecvInfo->iType = ntohl(p_RecvInfo->iType);
    p_RecvInfo->username[sizeof(p_RecvInfo->username)-1] = 0;
    p_RecvInfo->password[sizeof(p_RecvInfo->password)-1] = 0;

    //(4)这里可能要考虑 根据业务逻辑，进一步判断收到的数据的合法性，
    //当前该玩家的状态是否适合收到这个数据等等【比如如果用户没登陆，它就不适合购买物品等等】
    // --------后序处理用户注册逻辑--------
    
    //(5)给客户端返回数据时，一般也是返回一个结构，这个结构内容具体由客户端/服务器协商，这里我们就以给客户端也返回同样的 STRUCT_REGISTER 结构来举例    
    //LPSTRUCT_REGISTER pFromPkgHeader =  (LPSTRUCT_REGISTER)(((char *)pMsgHeader)+m_iLenMsgHeader);	//指向收到的包的包头，其中数据后续可能要用到
    LPCOMM_PKG_HEADER pPkgHeader;
    CMemory memory = CMemory::getInstance();
    CRC32 crc32 = CRC32::getInstance();
    int iSendLen = sizeof(STRUCT_REGISTER);
    // a)分配要发送出去包的内存
    char* p_sendbuf = (char*)memory.AllocMemory(m_iLenMsgHeader+m_iLenPkgHeader+iSendLen, false);// 消息头+包头+包体
    // b)填充消息头
    memcpy(p_sendbuf, pMsgHeader, m_iLenMsgHeader);
    // c)填充包头
    pPkgHeader = (LPCOMM_PKG_HEADER)(p_sendbuf+m_iLenMsgHeader); // 指向包头
    pPkgHeader->msgCode = _CMD_REGISTER; // 消息代码
    pPkgHeader->msgCode = htons(pPkgHeader->msgCode);
    pPkgHeader->pkgLen = htons(m_iLenPkgHeader+iSendLen); // 整个包的尺寸【包头+包体】
    // d)填充包体
    LPSTRUCT_REGISTER p_sendInfo = (LPSTRUCT_REGISTER)(p_sendbuf+m_iLenMsgHeader+m_iLenPkgHeader); // 指向包体
    // e)包体内容全部确定好后，计算包体crc32
    pPkgHeader->crc32 = crc32.Get_CRC((char*)p_sendInfo, iSendLen); // 计算整个包体的crc32
    pPkgHeader->crc32 = htonl(pPkgHeader->crc32); // 网络字节序转换
    //。。。。。这里根据需要，填充要发回给客户端的内容,int类型要使用htonl()转，short类型要使用htons()转；
    
    // f)发送数据
    msgSend(p_sendbuf);

    return true;


}

bool CLogicSocket::HandleLogin(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength)
{
    if(pPkgBody == nullptr)
    {
        return false;
    }
    int iRecvLen = sizeof(STRUCT_LOGIN);
    if(iRecvLen != iBodyLength)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(pConn->logicPorcMutex);

    LPSTRUCT_LOGIN p_RecvInfo = (LPSTRUCT_LOGIN)pPkgBody;
    p_RecvInfo->username[sizeof(p_RecvInfo->username)-1]=0;
    p_RecvInfo->password[sizeof(p_RecvInfo->password)-1]=0;
    //--------后序处理用户登录逻辑--------

    //给客户端返回数据
    LPCOMM_PKG_HEADER pPkgHeader;
	CMemory  p_memory = CMemory::getInstance();
	CRC32   p_crc32 = CRC32::getInstance();

    int iSendLen = sizeof(STRUCT_LOGIN);
    char* p_sendbuf = (char*)p_memory.AllocMemory(m_iLenMsgHeader+m_iLenPkgHeader+iSendLen,false);  
    memcpy(p_sendbuf,pMsgHeader,m_iLenMsgHeader);
    pPkgHeader = (LPCOMM_PKG_HEADER)(p_sendbuf+m_iLenMsgHeader);
    pPkgHeader->msgCode = _CMD_LOGIN;
    pPkgHeader->msgCode = htons(pPkgHeader->msgCode);
    pPkgHeader->pkgLen  = htons(m_iLenPkgHeader + iSendLen);   
    LPSTRUCT_LOGIN p_sendInfo = (LPSTRUCT_LOGIN)(p_sendbuf+m_iLenMsgHeader+m_iLenPkgHeader);
    pPkgHeader->crc32 = p_crc32.Get_CRC((char*)p_sendInfo, iSendLen);
    pPkgHeader->crc32 = htonl(pPkgHeader->crc32);
    //。。。。。这里根据需要，填充要发回给客户端的内容,int类型要使用htonl()转，short类型要使用htons()转；

    msgSend(p_sendbuf);
    return true;
}

bool CLogicSocket::HandlePing(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgBody, uint16_t iBodyLength)
{
    if(iBodyLength != 0)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(pConn->logicPorcMutex);
    pConn->lastPingTime = time(nullptr);

    //服务器也发送 一个只有包头的数据包给客户端，作为返回的数据
    SendNoBodyPkgToClient(pMsgHeader,_CMD_PING);

    return true;
}


}