#pragma once

#include <iostream>
#include <chrono> // 添加 chrono 用于时间戳
#include "my_http_request.h"

namespace WYXB
{
const size_t LARGE_FILE_THRESHOLD = 1 * 1024 * 1024; // 最大内存缓冲1MB
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
    {buffer_.reserve(2 * 1024 * 1024);}

    bool parseRequest(std::vector<uint8_t> buf, bool& isErr, std::chrono::system_clock::time_point receiveTime);
    bool gotAll() const 
    { return state_ == kGotAll;  }

    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummyData;
        request_.fileWriter_->closeFileStream();
        request_.swap(dummyData);
        buffer_.clear();
        parsed_pos_ = 0;
        body_bytes_received_ = 0;
    }

    const HttpRequest& request() const
    { return request_;}

    HttpRequest& request()
    { return request_;}

    // 文件重命名
    std::string urlDecode(const std::string &src) {
        std::string ret;
        size_t length = src.length();
        for (size_t i = 0; i < length; ++i) {
            char ch = src[i];
            if (ch == '%') {
                // 确保后面有两个字符可以解析
                if (i + 2 >= length) {
                    throw std::runtime_error("Invalid URL encoding.");
                }
                std::string hexStr = src.substr(i + 1, 2);
                int hexValue;
                std::istringstream iss(hexStr);
                iss >> std::hex >> hexValue;
                ret += static_cast<char>(hexValue);
                i += 2; // 跳过两个已经解析的字符
            } else if (ch == '+') {
                ret += ' ';
            } else {
                ret += ch;
            }
        }
        return ret;
    }

    std::string sanitizeFilename(const std::string& rawFilename) {
        // 1. URL 解码
        std::string decoded = urlDecode(rawFilename);

        // 2. 去除非法字符（例如只允许字母、数字、下划线和点）
        std::string safe;
        for (char c : decoded) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.')
                safe.push_back(c);
            else
                safe.push_back('_');
        }

        // 3. 分离文件扩展名（最后一个点后的部分）
        std::string baseName = safe;
        std::string extension;
        size_t pos = safe.rfind('.');
        if (pos != std::string::npos) {
            baseName = safe.substr(0, pos);    // 主文件名
            extension = safe.substr(pos);      // 包含点的扩展名
        }

        // 4. 限制主文件名长度（不包括扩展名）
        const size_t MAX_BASENAME_LENGTH = 10; // 例如：最大10个字符
        if (baseName.length() > MAX_BASENAME_LENGTH) {
            baseName = baseName.substr(0, MAX_BASENAME_LENGTH);
        }

        // 5. 重新拼接文件名
        return baseName + extension;
    }


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

    uint sequence{0}; // 期待解析的数据包的序号

public:

};

} // namespace http