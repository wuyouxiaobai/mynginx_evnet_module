#include "my_slogic.h"
#include "my_comm.h"
#include "arpa/inet.h"
#include "my_crc32.h"
#include "my_func.h"
#include "my_memory.h"
#include <cstring>
#include "my_global.h"
#include <fstream> 
#include "my_macro.h" 
#include <nlohmann/json.hpp>
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
    CSocket::Initialize();  //调用父类的同名函数
    InitRouter();
    InitMiddleware();
    InitSessionManager();
    // InitMysql() ;
    return true;
}





void CLogicSocket::registCallback(HttpRequest::Method method, const std::string &path, const Router::HandlerCallback &callback)
{
    m_Router->registerCallback(method, path, callback);
}


//处理接收消息队列中的消息，由线程池调用
void CLogicSocket::threadRecvProcFunc(STRUC_MSG_HEADER msghead, std::vector<uint8_t> pMsgBuf)
{
    Logger::ngx_log_stderr(0, "threadRecvProcFunc ing。。。。。。。。");


    // // 提取数据体
    // size_t headerSize = sizeof(STRUC_MSG_HEADER);
    // size_t dataSize = pMsgBuf.size() - headerSize;

    if (pMsgBuf.size() > 0) {
        bool isErr = false; // 判断是否解析失败
        lpngx_connection_t headptr =  msghead.pConn;
        if(headptr->iCurrsequence != msghead.iCurrsequence) // 连接已经断开
        {
            Logger::ngx_log_stderr(0, "连接已经断开。。。。。。。。。");
            return;
        }    

        Logger::ngx_log_stderr(0, "解析前1。。。。。。。。。");
        bool ok = headptr->context_->parseRequest(pMsgBuf, isErr, std::chrono::system_clock::now()); // 判断是否解析完成
        Logger::ngx_log_stderr(0, "解析前2。。。。。。。。。");
        if(!ok) // 未完成解析
        {

            if(!isErr) 
            {            
                Logger::ngx_log_stderr(0, "未完全解析");
                return; // 解析过程没有出错，直接返回
            }
            Logger::ngx_log_stderr(0, "msgSend(HTTP/1.1 400 Bad Request\r\n\r\n, headptr) 。。。。。。");
            msgSend("HTTP/1.1 400 Bad Request\r\n\r\n", headptr); // 返回错误响应
            return;

        }
        // 成功解析，处理数据体  
        Logger::ngx_log_stderr(0, "onRequest前1。。。。。。。。。");
        onRequest(headptr, headptr->context_->request());
        headptr->context_->reset();
        Logger::ngx_log_stderr(0, "成功解析，处理数据体");
    } else {
        Logger::ngx_log_stderr(0, "No data body present.");
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
    Logger::ngx_log_error_core(NGX_LOG_INFO, 0, "Connection: %s", connection.c_str());
    bool connection_close = (strcasecmp(connection.c_str(), "close") == 0);
    bool is_http11_keepalive = (strcasecmp(req.getVersion().c_str(), "HTTP/1.1") == 0 && strcasecmp(connection.c_str(), "keep-alive") == 0);
    bool is_http10_keepalive = (strcasecmp(req.getVersion().c_str(), "HTTP/1.0") == 0 && strcasecmp(connection.c_str(), "keep-alive") == 0);
    bool close = connection_close || !(is_http11_keepalive || is_http10_keepalive);

    HttpResponse response(close);
    conn->ishttpClose = close;


    // 根据请求报文信息来封装响应报文对象
    handleRequest(req, &response); 

    // 可以给response设置一个成员，判断是否请求的是文件，如果是文件设置为true，并且存在文件位置在这里send出去。
    // muduo::net::Buffer buf;
    std::string buf;
    response.appendToBuffer(buf);
    // 打印完整的响应内容用于调试
    // LOG_INFO << "Sending response:\n" << buf.toStringPiece().as_string();
    Logger::ngx_log_stderr(0, "msgSend %s.........", buf.c_str());
    msgSend(buf, conn);

}

// 执行请求对应的路由处理函数
void CLogicSocket::handleRequest(const HttpRequest &req, HttpResponse *resp)
{
    try
    {
        // 处理请求前的中间件
        HttpRequest mutableReq = req;
        // Logger::ngx_log_stderr(0, "middlewareChain_ 前 .........");
        middlewareChain_.processBefore(mutableReq);
        // Logger::ngx_log_stderr(0, "middlewareChain_ 后 .........");
        // 路由处理
        if (!m_Router->route(mutableReq, resp))
        {
            // LOG_INFO << "请求的啥，url：" << req.method() << " " << req.path();
            // LOG_INFO << "未找到路由，返回404";
            Logger::ngx_log_stderr(0, "Not Found .........");
            resp->setStatusCode(HttpResponse::k404NotFound);
            resp->setStatusMessage("Not Found");
            resp->setCloseConnection(true);
            resp->setVersion(req.getVersion());
        }

        // 处理响应后的中间件
        // Logger::ngx_log_stderr(0, "middlewareChain_ processAfter前 .........");
        middlewareChain_.processAfter(*resp);
        // Logger::ngx_log_stderr(0, "middlewareChain_ processAfter后 .........");
    }
    catch (const HttpResponse& res) 
    {
        // 处理中间件抛出的响应（如CORS预检请求）
        Logger::ngx_log_stderr(0, "处理中间件抛出的响应 .........");
        *resp = res;
    }
    catch (const std::exception& e) 
    {
        // 错误处理
        Logger::ngx_log_stderr(0, "k500InternalServerError .........");
        resp->setStatusCode(HttpResponse::k500InternalServerError);
        resp->setBody(e.what());
    }
}










/// 路由相关
// 初始化时注册
bool CLogicSocket::InitRouterRegist()
{
// 示例：注册动态路径心跳
    // m_Router->addRegexCallback(
    //     HttpRequest::Method::kGet, 
    //     "/heartbeat/\\d+",  // 匹配类似 /heartbeat/123
    //     [](const HttpRequest& req, HttpResponse* resp) {
    //         resp->setStatusCode(HttpResponse::HttpStatusCode::k200Ok);
    //         resp->setBody("Dynamic PONG");
    //     }
    // );



    m_Router->registerCallback(  // 改用精确匹配方法
        HttpRequest::Method::kGet,
        "/hello",  // 静态路径
        [](const HttpRequest& req, HttpResponse* resp) {
            resp->setStatusLine(req.getVersion(), HttpResponse::HttpStatusCode::k200Ok, "OK");
            // resp->addHeader("Keep-Alive", "timeout=500, max=1000");  
            resp->setContentType("text/plain; charset=utf-8"); // 显式设置编码
            resp->setBody("hello world"); // 直接返回中文文本
        }
    );

    // 新增HTML文件路由
    m_Router->registerCallback(
        HttpRequest::Method::kGet,
        "/getposthtml",
        [](const HttpRequest& req, HttpResponse* resp) {
            // 设置响应头
            resp->setStatusLine(req.getVersion(), HttpResponse::HttpStatusCode::k200Ok, "OK");
            resp->setContentType("text/html; charset=utf-8");
            // 读取HTML文件
            std::ifstream file("../html/posttest.html", std::ios::in | std::ios::binary);
            if (file) {
                std::string content((std::istreambuf_iterator<char>(file)), 
                                    std::istreambuf_iterator<char>());
                resp->setBody(content);
            } else {
                resp->setStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
                resp->setBody("HTML File Not Found");
            }
        }
    );

    m_Router->registerCallback(
        HttpRequest::Method::kPost,
        "/submit",
        [](const HttpRequest& req, HttpResponse* resp) {
            try {
                
                // 设置响应头
                resp->setStatusLine(req.getVersion(), HttpResponse::HttpStatusCode::k200Ok, "OK");
                resp->setContentType("text/html; charset=utf-8");
                // 解析表单数据
                std::vector<uint8_t> body = req.getBody();
                std::string prefix = "testData=";
                // 将字符串转换为字节序列
                std::vector<uint8_t> prefix_bytes(prefix.begin(), prefix.end());

                // 使用 std::search 算法查找字节序列
                auto it = std::search(
                    body.begin(), 
                    body.end(),
                    prefix_bytes.begin(),
                    prefix_bytes.end()
                );

                // 判断是否找到并计算位置
                size_t pos = (it != body.end()) ? std::distance(body.begin(), it) : std::string::npos;
                
                if (pos == std::string::npos) {
                    throw std::runtime_error("Invalid form data");
                }
    
                // URL解码并获取值
                auto start = body.begin() + pos + prefix.size();
                std::vector<uint8_t> encoded(start, body.end());
                std::string encoded_str(encoded.begin(), encoded.end());
                std::string decoded;
                for (size_t i = 0; i < encoded.size(); ++i) {
                    if (encoded_str[i] == '%' && i+2 < encoded_str.size()) {
                        int hex;
                        sscanf(encoded_str.substr(i+1,2).c_str(), "%x", &hex);
                        decoded += static_cast<char>(hex);
                        i += 2;
                    } else if (encoded_str[i] == '+') {
                        decoded += ' ';
                    } else {
                        decoded += encoded_str[i];
                    }
                }
    
                // 构造JSON响应
                nlohmann::json response = {
                    {"status", "success"},
                    {"message", "数据接收成功"},
                    {"input", decoded},
                    {"length", decoded.length()},
                    {"timestamp", static_cast<long>(std::time(nullptr))}
                };
    
                resp->setStatusCode(HttpResponse::k200Ok);
                resp->setContentType("application/json; charset=utf-8");
                resp->setBody(response.dump());
    
            } catch (const std::exception& e) {
                nlohmann::json error = {
                    {"status", "error"},
                    {"error_type", "invalid_request"},
                    {"message", e.what()}
                };
                resp->setStatusCode(HttpResponse::k400BadRequest);
                resp->setContentType("application/json; charset=utf-8");
                resp->setBody(error.dump());
            }
        }
    );


    m_Router->registerCallback(
        HttpRequest::Method::kGet,
        "/gethtml",
        [](const HttpRequest& req, HttpResponse* resp) {
            // 设置响应头
            resp->setStatusLine(req.getVersion(), HttpResponse::HttpStatusCode::k200Ok, "OK");
            resp->setContentType("text/html; charset=utf-8");
            // 读取HTML文件
            std::ifstream file("../html/test.html", std::ios::in | std::ios::binary);
            if (file) {
                std::string content((std::istreambuf_iterator<char>(file)), 
                                    std::istreambuf_iterator<char>());
                resp->setBody(content);
            } else {
                resp->setStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
                resp->setBody("HTML File Not Found");
            }
        }
    );

    // m_Router->registerCallback(
    //     HttpRequest::Method::kPost,
    //     "/uploadvideo",
    //     [](const HttpRequest& req, HttpResponse* resp) 
        // {
        //     // 1. 确保上传目录存在
        //     const std::string uploadDir = "../video/";
        //     if (!std::filesystem::exists(uploadDir)) {
        //         std::filesystem::create_directories(uploadDir);
        //     }
    
        //     // 2. 解析Content-Type获取boundary
        //     std::string contentType = req.getHeader("Content-Type");
        //     size_t boundaryPos = contentType.find("boundary=");
        //     if (boundaryPos == std::string::npos) {
        //         resp->setStatusCode(HttpResponse::k400BadRequest);
        //         resp->setBody("Invalid Content-Type");
        //         return;
        //     }
        //     std::string boundary = "--" + contentType.substr(boundaryPos + 9);
    
        //     // 3. 获取原始请求体
        //     std::vector<uint8_t> body = req.getBody();
    
        //     // 4. 解析multipart数据
        //     size_t fileStart = body.find(boundary);
        //     while (fileStart != std::string::npos) {
        //         size_t headersEnd = body.find("\r\n\r\n", fileStart);
        //         if (headersEnd == std::string::npos) break;
    
        //         // 解析文件头信息
        //         std::string headers = body.substr(
        //             fileStart + boundary.length() + 2, // 跳过boundary和换行
        //             headersEnd - (fileStart + boundary.length() + 2)
        //         );
    
        //         // 查找文件名
        //         size_t namePos = headers.find("filename=\"");
        //         if (namePos != std::string::npos) {
        //             size_t endQuote = headers.find("\"", namePos + 10);
        //             std::string filename = headers.substr(
        //                 namePos + 10,
        //                 endQuote - (namePos + 10)
        //             );
    
        //             // 提取文件内容
        //             size_t dataStart = headersEnd + 4;
        //             size_t dataEnd = body.find(boundary, dataStart);
        //             std::string fileContent = body.substr(
        //                 dataStart,
        //                 dataEnd - dataStart - 4 // 减去结尾的\r\n
        //             );
    
        //             // 保存文件
        //             std::ofstream out(uploadDir + filename, std::ios::binary);
        //             out.write(fileContent.data(), fileContent.size());
        //             resp->setStatusCode(HttpResponse::k200Ok);
        //             resp->setBody("Upload success");
        //             return;
        //         }
    
        //         fileStart = body.find(boundary, headersEnd);
        //     }
    
        //     // 未找到有效文件
        //     resp->setStatusCode(HttpResponse::k400BadRequest);
        //     resp->setBody("No video file received");
        // }
    // );
    
    m_Router->registerCallback(
        HttpRequest::Method::kGet,
        "/getvideos",
        [](const HttpRequest& req, HttpResponse* resp) {
            // 使用命名常量提升可维护性
            constexpr const char* videoDir = "../video/";
            constexpr const char* targetExt = ".mp4";
            
            std::vector<std::string> videos;
    
            try {
                // 前置目录检查避免异常
                if (!std::filesystem::exists(videoDir)) {
                    throw std::runtime_error("Directory not found");
                }
    
                // 优化文件遍历逻辑
                for (const auto& entry : std::filesystem::directory_iterator(videoDir)) {
                    if (entry.is_regular_file()) {
                        // 不区分大小写的扩展名比较
                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                        
                        if (ext == targetExt) {
                            videos.emplace_back(entry.path().filename().string());
                        }
                    }
                }
    
                // 使用nlohmann/json直接序列化
                resp->setContentType("application/json; charset=utf-8");
                resp->setBody(nlohmann::json(videos).dump());
                
                // 添加缓存控制头
                resp->addHeader("Cache-Control", "max-age=3600");
    
            } catch (const std::exception& e) {
                // 细化错误处理
                Logger::ngx_log_stderr(0, "Video list error: %s | Path: %s", 
                                     e.what(), videoDir);
                
                resp->setStatusCode(HttpResponse::k503ServiceUnavailable);
                resp->setContentType("application/problem+json");
                
                nlohmann::json error = {
                    {"type", "/errors/video-list"},
                    {"title", "Video Listing Failed"},
                    {"status", 503},
                    {"detail", "Failed to retrieve video list"}
                };
                resp->setBody(error.dump());
            }
        }
    );
    
    // m_Router->registerCallback(
    //     HttpRequest::Method::kGet,
    //     "/getvideo/(.+)", // 正则匹配文件名
    //     [](const HttpRequest& req, HttpResponse* resp) {
    //         const std::string videoPath = "../video/" + req.getPath().substr(10); // 提取文件名
            
    //         std::ifstream file(videoPath, std::ios::binary | std::ios::ate);
    //         if (!file) {
    //             resp->setStatusCode(HttpResponse::k404NotFound);
    //             return;
    //         }
    
    //         // 设置视频流响应头
    //         resp->setContentType("video/mp4");
    //         resp->addHeader("Accept-Ranges", "bytes");
            
    //         // 读取文件内容
    //         std::streamsize size = file.tellg();
    //         file.seekg(0, std::ios::beg);
    //         std::vector<char> buffer(size);
    //         if (file.read(buffer.data(), size)) {
    //             resp->setBody(std::string(buffer.data(), buffer.size()));
    //         }
    //     }
    // );

    return true;
}



/// 中间件相关
// 初始化时注册
void CLogicSocket::InitMiddleware() 
{

}



/// session相关
// 初始化时注册
void CLogicSocket::InitSessionManager()
{
//示例使用内存存储
    // 创建会话存储
    auto sessionStorage = std::make_unique<MemorySessionStorage>(); 
    // 创建会话管理器
    auto sessionManager = std::make_unique<SessionManager>(std::move(sessionStorage));
    // 设置会话管理器
    setSessionManager(std::move(sessionManager));
}






/// 数据库相关
// 初始化时
void CLogicSocket::InitMysql() 
{
    dbconn_ = ConnectionPool::getInstance()->getConnection();
}






















// // // 心跳包检测时间到，该去检测心跳包是否超时的事宜，本函数是子类函数，实现具体的判断动作
// void CLogicSocket::procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t cur_time){}
// {
//     CMemory memory = CMemory::getInstance();
//     if(tmpmsg->iCurrsequence == tmpmsg->pConn->iCurrsequence)//此连接没断
//     {
//         lpngx_connection_t p_Conn = tmpmsg->pConn;
//         if(m_ifTimeOutKick == 1)
//         {
//             zdClosesocketProc(p_Conn);
//         }
//         else if((cur_time - p_Conn->lastPingTime) > (m_iWaitTime*3+10))//超时踢的判断标准就是 每次检查的时间间隔*3，超过这个时间没发送心跳包，就踢【大家可以根据实际情况自由设定】
//         {
//             zdClosesocketProc(p_Conn);
//         }
//         memory.FreeMemory(tmpmsg);

//     }
//     else
//     {
//         memory.FreeMemory(tmpmsg);
//     }
//     return;
// }






// // 发送没有包体的数据包给客户端
// void CLogicSocket::SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader, uint16_t iMsgCode){}
// {
//     CMemory memory = CMemory::getInstance();

//     char* p_sendbuf = (char*)memory.AllocMemory(m_iLenMsgHeader+m_iLenPkgHeader, false);
//     char* p_tmpbuf = p_sendbuf;

//     memcpy(p_tmpbuf, pMsgHeader, m_iLenMsgHeader);
//     p_tmpbuf += m_iLenMsgHeader;

//     LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)p_tmpbuf; // 指向要发出去的包头
//     pPkgHeader->msgCode = htons(iMsgCode);
//     pPkgHeader->pkgLen = htonl(m_iLenPkgHeader);
//     pPkgHeader->crc32 = 0;
//     msgSend(p_sendbuf);
//     return;



// }
}