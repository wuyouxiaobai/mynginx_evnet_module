#pragma once

#include <iostream>
#include <chrono> // 添加 chrono 用于时间戳
#include "my_http/my_http_request.h"

namespace WYXB
{

class HttpContext 
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine, // 解析请求行
        kExpectHeaders, // 解析请求头
        kExpectBody, // 解析请求体
        kGotAll, // 解析完成
    };
    
    HttpContext()
    : state_(kExpectRequestLine)
    {}

    bool parseRequest(std::string buf, bool& isErr, std::chrono::system_clock::time_point receiveTime);
    bool gotAll() const 
    { return state_ == kGotAll;  }

    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummyData;
        request_.swap(dummyData);
    }

    const HttpRequest& request() const
    { return request_;}

    HttpRequest& request()
    { return request_;}

private:
    bool processRequestLine(const char* begin, const char* end);
private:
    HttpRequestParseState state_;
    HttpRequest           request_;

private:
    std::string buffer_;       // 持久化接收缓冲区
    size_t parsed_pos_ = 0;     // 已解析位置

public:


};

} // namespace http