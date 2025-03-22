#pragma once
#include "my_socket.h"
#include <functional>
#include <unordered_map>
#include "router.h"
#include "middlewareChain.h"
#include "sessionManager.h"
#include "connectionPool.h"

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
    virtual void threadRecvProcFunc(STRUC_MSG_HEADER msghead, std::vector<uint8_t> pMsgBuf);



/// 路由相关
private:

    std::shared_ptr<Router> m_Router;
    void InitRouter() {
        m_Router = std::make_shared<Router>();
        InitRouterRegist();
    }
    // 初始化时注册
    bool InitRouterRegist();
public:
    // 动态注册
    void registCallback(HttpRequest::Method method, const std::string &path, const Router::HandlerCallback &callback);
private:
    void onRequest(lpngx_connection_t conn, const HttpRequest &req);
    void handleRequest(const HttpRequest &req, HttpResponse *resp);
private:
    // // 通用收发数据相关函数
    // void SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader, uint16_t iMsgCode);
    // // 心跳包时间到，检测心跳包是否超时
    // virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t currtime);

private:


/// 中间件
private:
    MiddlewareChain middlewareChain_;

    // 初始化时注册
    void InitMiddleware() ;
public:
    //添加中间件
    void addMiddleware(std::shared_ptr<Middleware> middleware) {
        middlewareChain_.addMiddleware(middleware);
    }




/// 会话相关
public:
    void setSessionManager(std::unique_ptr<SessionManager> sessionManager) {
        sessionManager_ = std::move(sessionManager);
    }

    SessionManager* getSessionManager() { return sessionManager_.get(); }

private:
    std::unique_ptr<SessionManager> sessionManager_;

    // 初始化时注册
    void InitSessionManager() ;


/// 数据库相关
    // 初始化时
    void InitMysql() ;

    std::shared_ptr<MySQLConn> dbconn_;


};



    
}