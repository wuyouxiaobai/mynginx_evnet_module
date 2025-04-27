#include "my_http_request.h"
#include <cassert>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "my_socket.h"

namespace WYXB
{

void HttpRequest::setReceiveTime(std::chrono::system_clock::time_point t)
{
    receiveTime_ = t;
}

bool HttpRequest::setMethod(const char *start, const char *end)
{
    assert(method_ == kInvalid);
    std::string m(start, end); // [start, end)
    if (m == "GET")
    {
        method_ = kGet;
    }
    else if (m == "POST")
    {
        method_ = kPost;
    }
    else if (m == "PUT")
    {
        method_ = kPut;
    }
    else if (m == "DELETE")
    {
        method_ = kDelete;
    }
    else if (m == "OPTIONS")
    {
        method_ = kOptions;
    }
    else
    {
        method_ = kInvalid;
    }

    return method_ != kInvalid;
}

void HttpRequest::setPath(const char *start, const char *end)
{
    path_.assign(start, end);
}

void HttpRequest::setPathParameters(const std::string &key, const std::string &value)
{
    pathParameters_[key] = value;
}

std::string HttpRequest::getPathParameters(const std::string &key) const
{
    auto it = pathParameters_.find(key);
    if (it != pathParameters_.end())
    {
        return it->second;
    }
    return "";
}

std::string HttpRequest::getQueryParameters(const std::string &key) const
{
    auto it = queryParameters_.find(key);
    if (it != queryParameters_.end())
    {
        return it->second;
    }
    return "";
}

// 这是从问号后面分割参数
void HttpRequest::setQueryParameters(const char *start, const char *end)
{
    std::string argumentStr(start, end);
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;

    // 按 & 分割多个参数
    while ((pos = argumentStr.find('&', prev)) != std::string::npos)
    {
        std::string pair = argumentStr.substr(prev, pos - prev);
        std::string::size_type equalPos = pair.find('=');

        if (equalPos != std::string::npos)
        {
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            queryParameters_[key] = value;
        }

        prev = pos + 1;
    }

    // 处理最后一个参数
    std::string lastPair = argumentStr.substr(prev);
    std::string::size_type equalPos = lastPair.find('=');
    if (equalPos != std::string::npos)
    {
        std::string key = lastPair.substr(0, equalPos);
        std::string value = lastPair.substr(equalPos + 1);
        queryParameters_[key] = value;
    }
}

void HttpRequest::addHeader(std::string key, std::string value)
{
    headers_[key] = value;
}

std::string HttpRequest::getHeader(const std::string &field) const
{
    std::string result;
    auto it = headers_.find(field);
    if (it != headers_.end())
    {
        result = it->second;
    }
    return result;
}

void HttpRequest::swap(HttpRequest &that)
{
    std::swap(method_, that.method_);
    std::swap(path_, that.path_);
    std::swap(pathParameters_, that.pathParameters_);
    std::swap(queryParameters_, that.queryParameters_);
    std::swap(version_, that.version_);
    std::swap(headers_, that.headers_);
    std::swap(receiveTime_, that.receiveTime_);
    std::swap(content_, that.content_);
    std::swap(contentLength_, that.contentLength_);
    std::swap(boundary_, that.boundary_);
    std::swap(fileHeader_, that.fileHeader_);
    std::swap(formFields_, that.formFields_);
    std::swap(uploadedFiles_, that.uploadedFiles_);
    std::swap(connection_, that.connection_);
    
}

void HttpRequest::setConnection(const std::shared_ptr<ngx_connection_s>& conn)
{
    connection_ = conn;
}

std::string HttpRequest::getPeerIp() const
{
    auto conn = connection_.lock();
    if (!conn) {
        return "0.0.0.0"; // 连接已经无效
    }

    char ip[INET_ADDRSTRLEN] = {0}; // IPv4地址长度
    struct sockaddr_in* addr_in = (struct sockaddr_in*)&conn->s_sockaddr;
    inet_ntop(AF_INET, &(addr_in->sin_addr), ip, INET_ADDRSTRLEN);
    return std::string(ip);
}


}