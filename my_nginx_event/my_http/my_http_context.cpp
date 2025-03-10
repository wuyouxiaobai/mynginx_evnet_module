#include "my_http/my_http_context.h"
#include <algorithm>

namespace WYXB
{

// 将报文解析出来将关键信息封装到HttpRequest对象里面去
bool HttpContext::parseRequest(std::string buf, std::chrono::system_clock::time_point receiveTime)
{
    bool ok = true; // 解析每行请求格式是否正确
    bool hasMore = true;
    size_t pos = 0; // 用于记录当前解析的位置
    while (hasMore)
    {
        if (state_ == kExpectRequestLine)
        {
            size_t crlf = buf.find("\r\n", pos); // 查找CRLF
            if (crlf != std::string::npos)
            {
                ok = processRequestLine(buf.data() + pos, buf.data() + crlf);
                if (ok)
                {
                    request_.setReceiveTime(receiveTime);
                    pos = crlf + 2; // 移动到下一行
                    state_ = kExpectHeaders;
                }
                else
                {
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == kExpectHeaders)
        {
            size_t crlf = buf.find("\r\n", pos);
            if (crlf != std::string::npos)
            {
                const char *colon = std::find(buf.data() + pos, buf.data() + crlf, ':');
                if (colon < buf.data() + crlf)
                {
                    request_.addHeader(buf.data() + pos, colon, buf.data() + crlf);
                }
                else if (buf.data() + pos == buf.data() + crlf)
                { 
                    // 空行，结束Header
                    // 根据请求方法和Content-Length判断是否需要继续读取body
                    if (request_.method() == HttpRequest::kPost || 
                        request_.method() == HttpRequest::kPut)
                    {
                        std::string contentLength = request_.getHeader("Content-Length");
                        if (!contentLength.empty())
                        {
                            request_.setContentLength(std::stoi(contentLength));
                            if (request_.contentLength() > 0)
                            {
                                state_ = kExpectBody;
                            }
                            else
                            {
                                state_ = kGotAll;
                                hasMore = false;
                            }
                        }
                        else
                        {
                            // POST/PUT 请求没有 Content-Length，是HTTP语法错误
                            ok = false;
                            hasMore = false;
                        }
                    }
                    else
                    {
                        // GET/HEAD/DELETE 等方法直接完成（没有请求体）
                        state_ = kGotAll; 
                        hasMore = false;
                    }
                }
                else
                {
                    ok = false; // Header行格式错误
                    hasMore = false;
                }
                pos = crlf + 2; // 移动到下一行
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == kExpectBody)
        {
            // 检查缓冲区中是否有足够的数据
            if (buf.size() - pos < request_.contentLength())
            {
                hasMore = false; // 数据不完整，等待更多数据
                return true;
            }

            // 只读取 Content-Length 指定的长度
            std::string body(buf.data() + pos, buf.data() + pos + request_.contentLength());
            request_.setBody(body);

            // 准确移动读指针
            pos += request_.contentLength();

            state_ = kGotAll;
            hasMore = false;
        }
    }
    return ok; // ok为false代表报文语法解析错误
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