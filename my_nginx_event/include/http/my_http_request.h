#pragma once
#include <vector>
#include <map>
#include <string>
#include <unordered_map>
#include <chrono> // 添加 chrono 用于时间戳

namespace WYXB
{


class HttpRequest
{
    friend class HttpContext;
    struct FilePart {
        std::string filename;
        std::string content_type;
        std::string data;
    };
public:
    enum Method
    {
        kInvalid, kGet, kPost, kHead, kPut, kDelete, kOptions
    };
    
    HttpRequest()
        : method_(kInvalid)
        , version_("Unknown")
    {
    }
    
    void setReceiveTime(std::chrono::system_clock::time_point t);
    std::chrono::system_clock::time_point receiveTime() const { return receiveTime_; }
    
    bool setMethod(const char* start, const char* end);
    Method method() const { return method_; }

    void setPath(const char* start, const char* end);
    std::string path() const { return path_; }

    void setPathParameters(const std::string &key, const std::string &value);
    std::string getPathParameters(const std::string &key) const;

    void setQueryParameters(const char* start, const char* end);
    std::string getQueryParameters(const std::string &key) const;
    
    void setVersion(std::string v)
    {
        version_ = v;
    }

    std::string getVersion() const
    {
        return version_;
    }
    
    void addHeader(std::string key, std::string value);
    std::string getHeader(const std::string& field) const;

    const std::map<std::string, std::string>& headers() const
    { return headers_; }

    void setBody(const std::string& body) { content_ = body; }
    void setBody(const char* start, const char* end) 
    { 
        if (end >= start) 
        {
            content_.assign(start, end - start); 
        }
    }
    
    std::string getBody() const
    { return content_; }

    void setContentLength(uint64_t length)
    { contentLength_ = length; }
    
    uint64_t contentLength() const
    { return contentLength_; }

    void swap(HttpRequest& that);

private:
    Method                                       method_; // 请求方法
    std::string                                  version_; // http版本
    std::string                                  path_; // 请求路径
    std::unordered_map<std::string, std::string> pathParameters_; // 路径参数
    std::unordered_map<std::string, std::string> queryParameters_; // 查询参数
    std::chrono::system_clock::time_point        receiveTime_; // 接收时间
    std::map<std::string, std::string>           headers_; // 请求头
    std::string                                  content_; // 请求体
    uint64_t                                     contentLength_ { 0 }; // 请求体长度
    std::vector<FilePart> files;
    std::string boundary;

public:  
void setMultipartBoundary(const std::string& ct) {
    size_t pos = ct.find("boundary=");
    if (pos != std::string::npos) {
        boundary = ct.substr(pos + 9);
        if (boundary.front() == '"') 
            boundary = boundary.substr(1, boundary.size()-2);
    }
}

bool isMultipart() const {
    auto it = headers_.find("content-type");
    return it != headers_.end() && 
           it->second.find("multipart/form-data") != std::string::npos;
}

};  


} 