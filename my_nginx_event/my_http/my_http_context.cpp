#include "my_http/my_http_context.h"
#include <algorithm>
#include <cstring>

namespace WYXB
{

// 将报文解析出来将关键信息封装到HttpRequest对象里面去
bool HttpContext::parseRequest(std::string buf, std::chrono::system_clock::time_point receiveTime)
{
    buffer_.append(buf);
    bool ok = true;

    while (parsed_pos_ < buffer_.size()) {
        switch(state_) {
            case kExpectRequestLine: {
                size_t crlf = buffer_.find("\r\n", parsed_pos_);
                if (crlf != std::string::npos) {
                    // 请求行长度超过1KB视为攻击尝试
                    if (crlf - parsed_pos_ > 1024) { 
                        ok = false;
                        break;
                    }
                    
                    
                    ok = processRequestLine(buffer_.data()+parsed_pos_, 
                                            buffer_.data()+crlf);
                    if (ok) {
                        request_.setReceiveTime(receiveTime);
                        parsed_pos_ = crlf + 2;
                        state_ = kExpectHeaders;
                    }
                    
                } else if (buffer_.size() - parsed_pos_ > 1024) {
                    ok = false; // 异常长请求行防御
                }
                break;
            }
            
            case kExpectHeaders: {
                while(parsed_pos_ < buffer_.size()) {
                    size_t crlf = buffer_.find("\r\n", parsed_pos_);
                    if (crlf == std::string::npos) break;

                    // 空行检测
                    if (crlf == parsed_pos_) {
                        parsed_pos_ += 2;

                        if (request_.method() == HttpRequest::kPost || 
                            request_.method() == HttpRequest::kPut) {
                            auto contentLen = request_.getHeader("Content-Length");
                            if (contentLen.empty()) {
                                ok = false; // POST/PUT必须包含Content-Length[3](@ref)
                            } else if (!contentLen.empty()) {
                                try {
                                    request_.setContentLength(std::stoi(contentLen));
                                    state_ = (request_.contentLength() > 0) ? 
                                            kExpectBody : kGotAll;
                                } catch (...) {
                                    ok = false; // 非法Content-Length格式
                                }
                            }
                        } else {
                            state_ = kGotAll;
                        }

                        break;
                    }

                    // 解析Header键值对
                    const char* colon = static_cast<const char*>(
                        memchr(buffer_.data()+parsed_pos_, ':', crlf - parsed_pos_));
                    if (colon) {
                        //Header解析
                            std::string key(buffer_.data()+parsed_pos_, colon);
                            std::string val(colon+1, buffer_.data()+crlf);
                            request_.addHeader(
                                std::move(key.erase(key.find_last_not_of(" \t")+1)),
                                std::move(val.erase(0, val.find_first_not_of(" \t")))
                            );
                        
                    } else {
                        ok = false; // 非法Header格式
                    }
                    parsed_pos_ = crlf + 2;
                }
                break;
            }
            
            case kExpectBody: {
                size_t remaining = buffer_.size() - parsed_pos_;
                if (remaining >= request_.contentLength()) {
                    request_.setBody(buffer_.substr(
                        parsed_pos_, request_.contentLength()));
                    parsed_pos_ += request_.contentLength();
                    state_ = kGotAll;
                }
                break;
            }
            
            case kGotAll:
                return true;
        }
        
        if (!ok || (state_ != kGotAll && parsed_pos_ >= buffer_.size())) 
            break;
    }

    // 滑动窗口优化（内存安全防御）
    if (parsed_pos_ > 4096) {
        buffer_ = buffer_.substr(parsed_pos_);
        parsed_pos_ = 0;
    } else if (parsed_pos_ > 0) {
        buffer_.erase(0, parsed_pos_);
        parsed_pos_ = 0;
    }
    return ok;
}

// 解析请求行
bool HttpContext::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    if (space != end && request_.setMethod(start, space))
    {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end)
        {
            const char *argumentStart = std::find(start, space, '?');
            if (argumentStart != space) // 请求带参数
            {
                request_.setPath(start, argumentStart); // 注意这些返回值边界
                request_.setQueryParameters(argumentStart + 1, space);
            }
            else // 请求不带参数
            {
                request_.setPath(start, space);
            }

            start = space + 1;
            succeed = ((end - start == 8) && std::equal(start, end - 1, "HTTP/1."));
            if (succeed)
            {
                if (*(end - 1) == '1')
                {
                    request_.setVersion("HTTP/1.1");
                }
                else if (*(end - 1) == '0')
                {
                    request_.setVersion("HTTP/1.0");
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

} // namespace http