#include "my_http_response.h"
#include "my_logger.h"
#include "my_macro.h"

namespace WYXB
{

void HttpResponse::appendToBuffer(std::string& outputBuf) const
{
    // HttpResponse封装的信息格式化输出
    char buf[32]; 
    // 为什么不把状态信息放入格式化字符串中，因为状态信息有长有短，不方便定义一个固定大小的内存存储
    snprintf(buf, sizeof buf, "%s %d ", httpVersion_.c_str(), statusCode_);
    
    outputBuf.append(buf);
    outputBuf.append(statusMessage_);
    outputBuf.append("\r\n");


    if (closeConnection_) // 思考一下这些地方是不是可以直接移入近headers_中
    {
        outputBuf.append("Connection: close\r\n");
    }
    else
    {
        // snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        // outputBuf.append(buf);
        outputBuf.append("Connection: Keep-Alive\r\n");
    }

    // // 添加Content-Type头
    // if(headers_.find("Content-Type") == headers_.end()) {
    //     outputBuf.append("Content-Type: text/plain; charset=utf-8\r\n");
    // }

    for (const auto& header : headers_)
    { // 为什么这里不用格式化字符串？因为key和value的长度不定
        outputBuf.append(header.first);
        outputBuf.append(": "); 
        outputBuf.append(header.second);
        outputBuf.append("\r\n");
    }
    // 必须添加Content-Length
    outputBuf.append("Content-Length: " + std::to_string(body_.size()) + "\r\n");

    outputBuf.append("\r\n");
    
    outputBuf.append(body_);

    // Logger::ngx_log_error_core(NGX_LOG_INFO, 0, "outputBuf is: %s", outputBuf.c_str());
}

void HttpResponse::setStatusLine(const std::string& version,
                                 HttpStatusCode statusCode,
                                 const std::string& statusMessage)
{
    httpVersion_ = version;
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
}

} 