#pragma once
#include <vector>
#include <map>
#include <string>
#include <unordered_map>
#include <chrono>
#include <fstream>
#include <sstream>
#include "my_logger.h"
#include <memory>

namespace WYXB
{
class FileWriter {
public:
    // 初始化文件流，传入保存的文件路径
    bool initFileStream(const std::string& filePath) {
        // 以二进制写入方式打开文件（如果文件存在则覆盖，也可以使用 std::ios::app 追加）
        fileStream_.open(filePath, std::ios::binary | std::ios::out);
        if (!fileStream_.is_open()) {
            // 记录错误信息，打开文件失败
            Logger::ngx_log_stderr(0, "initFileStream failed to open file: %s", filePath.c_str());
            return false;
        }
        Logger::ngx_log_stderr(0, "initFileStream success to open file: %s", filePath.c_str());
        return true;
    }

    // 将数据写入文件
    void writeToFile(const char* data, size_t size) {
        if (fileStream_.is_open()) {
            fileStream_.write(data, size);
            if (!fileStream_) {
                // 写入时出现错误
                Logger::ngx_log_stderr(0, "writeToFile encountered an error while writing data");
            }
            Logger::ngx_log_stderr(0, "writeToFile is writing data");
        } else {
            Logger::ngx_log_stderr(0, "writeToFile called but fileStream_ is not open");
        }
    }

    // 关闭文件流（完成上传或在错误处理时调用）
    void closeFileStream() {
        if (fileStream_.is_open()) {
            fileStream_.close();
        }
    }
    // 禁用拷贝构造函数和赋值运算符
    FileWriter(const FileWriter&) = delete;
    FileWriter& operator=(const FileWriter&) = delete;
    // 允许移动
    FileWriter(FileWriter&&) = default;
    FileWriter& operator=(FileWriter&&) = default;

    FileWriter() = default;
private:
    std::ofstream fileStream_;  // 大文件上传的视频流
};



class HttpRequest
{
    friend class HttpContext;
public:
    enum Method
    {
        kInvalid, kGet, kPost, kHead, kPut, kDelete, kOptions
    };
    
    HttpRequest()
        : method_(kInvalid)
        , version_("Unknown")
        , fileWriter_(std::make_shared<FileWriter>())
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

    void setBody(const std::vector<uint8_t>& body) { content_ = body; }
    void setBody(const char* start, const char* end) 
    { 
        if (end > start) {

            const uint8_t* u8_start = reinterpret_cast<const uint8_t*>(start);
            const uint8_t* u8_end = reinterpret_cast<const uint8_t*>(end);
            content_.assign(u8_start, u8_end);
        }
    }

    void appendBody(const char* data, size_t len) {
        content_.reserve(len);
        const uint8_t* u8_data = reinterpret_cast<const uint8_t*>(data);
        content_.insert(content_.end(), u8_data, u8_data + len);
    }
    
    std::vector<uint8_t> getBody() const
    { return content_; }

    void setContentLength(uint64_t length)
    { contentLength_ = length; }
    
    uint64_t contentLength() const
    { return contentLength_; }

    void swap(HttpRequest& that);

    void setBoundary(std::string boundary)
    {
        boundary_ = boundary;
    }

    std::string boundary()
    {
        return boundary_;
    }

    void savefileHeader(std::string fileHeader)
    {
        fileHeader_ = fileHeader;
    }

    std::string getfileHeader() const
    {
        return fileHeader_;
    }

    // 保存上传的文件部分
    void saveFilePart(const std::string& part_data)
    {
        // 生成唯一的文件名，例如使用当前时间和计数器组合
        std::string fileName = "upload_" + generateUniqueFileName() + ".dat";
        std::ofstream ofs(fileName, std::ios::binary);
        if (!ofs) {
            Logger::ngx_log_stderr(0, "无法打开文件保存: %s", fileName);
            return;
        }
        ofs.write(part_data.data(), part_data.size());
        ofs.close();
        uploadedFiles_.push_back(fileName);
        Logger::ngx_log_stderr(0, "保存文件: %s", fileName);

    }
    // 保存普通表单字段
    void saveFormField(const std::string& partHeader, const std::string& data)
    {
        std::string fieldName = parseFieldName(partHeader);
        if (fieldName.empty()) {
            Logger::ngx_log_stderr(0, "解析表单字段名失败");
            return;
        }
        formFields_[fieldName] = data;
        Logger::ngx_log_stderr(0, "保存表单字段: %s", fieldName);
    }

    // 获取所有保存的表单字段
    const std::unordered_map<std::string, std::string>& getFormFields() const
    {
        return formFields_;
    }

    // 获取上传的文件列表
    const std::vector<std::string>& getUploadedFiles() const
    {
        return uploadedFiles_;
    }



        



private:
    Method                                       method_; // 请求方法
    std::string                                  version_; // http版本
    std::string                                  path_; // 请求路径
    std::unordered_map<std::string, std::string> pathParameters_; // 路径参数
    std::unordered_map<std::string, std::string> queryParameters_; // 查询参数
    std::chrono::system_clock::time_point        receiveTime_; // 接收时间
    std::map<std::string, std::string>           headers_; // 请求头
    std::vector<uint8_t>                         content_; // 请求体
    uint64_t                                     contentLength_ { 0 }; // 请求体长度
    std::string                                  boundary_ ;  // 视频边界
    std::string                                  fileHeader_;   // 视频文件名称
    std::shared_ptr<FileWriter>                  fileWriter_;  // 大文件上传的视频流

    // 用于保存表单字段（非文件上传）数据
    std::unordered_map<std::string, std::string> formFields_;
    // 用于保存上传的文件名列表
    std::vector<std::string>                     uploadedFiles_;

private:
    // 从 partHeader 中解析出表单字段名，例如查找 name="xxx"
    std::string parseFieldName(const std::string& header)
    {
        std::string token = "name=\"";
        size_t pos = header.find(token);
        if (pos == std::string::npos) return "";
        pos += token.size();
        size_t end = header.find("\"", pos);
        if (end == std::string::npos) return "";
        return header.substr(pos, end - pos);
    }

    // 生成唯一文件名的辅助方法
    std::string generateUniqueFileName()
    {
        static int counter = 0;
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        std::ostringstream oss;
        oss << ms << "_" << counter++;
        return oss.str();
    }


};  


} 