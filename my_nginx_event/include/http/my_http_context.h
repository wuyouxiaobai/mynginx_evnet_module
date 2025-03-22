#pragma once

#include <iostream>
#include <chrono> // 添加 chrono 用于时间戳
#include "my_http_request.h"

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
    {buffer_.reserve(4 * 1024 * 1024);}

    bool parseRequest(std::vector<uint8_t> buf, bool& isErr, std::chrono::system_clock::time_point receiveTime);
    bool gotAll() const 
    { return state_ == kGotAll;  }

    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummyData;
        request_.swap(dummyData);
        buffer_.clear();
        parsed_pos_ = 0;
        body_bytes_received_ = 0;
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
    std::vector<uint8_t> buffer_;       // 持久化接收缓冲区
    size_t parsed_pos_ = 0;     // 已解析位置
    size_t body_bytes_received_ = 0; //记录已接收的Body字节数

public:

};

} // namespace http