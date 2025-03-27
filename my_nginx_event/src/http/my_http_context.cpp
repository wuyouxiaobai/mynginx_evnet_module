#include "my_http_context.h"
#include <algorithm>
#include <cstring>
#include "my_logger.h"
#include "my_conf.h"
#include <filesystem>

namespace WYXB
{

// 将报文解析出来将关键信息封装到HttpRequest对象里面去
bool HttpContext::parseRequest(std::vector<uint8_t> buf, bool& isErr, std::chrono::system_clock::time_point receiveTime)
{
    if (!buf.empty()) {
        Logger::ngx_log_stderr(0, "parseRequest 中 buf ： %s", buf);
        buffer_.insert(buffer_.end(), buf.begin(), buf.end());
    }
    bool ok = true;
    isErr = false;
    Logger::ngx_log_stderr(0, "parseRequest...................");
    while (parsed_pos_ < buffer_.size()) {
        switch(state_) {
            case kExpectRequestLine: {
                Logger::ngx_log_stderr(0, "kExpectRequestLine...................");
                // 定义要查找的 CRLF（"\r\n"）模式
                const uint8_t crlf_pattern[] = {'\r', '\n'}; // 二进制形式的 "\r\n"
                // 计算查找范围
                auto start = buffer_.begin() + parsed_pos_;
                auto end = buffer_.end();
                // 使用 std::search 查找子序列
                auto found = std::search(start, end, 
                                        std::begin(crlf_pattern), std::end(crlf_pattern));
                // 计算位置或标记未找到
                size_t crlf = (found != end) ? (found - buffer_.begin()) : std::string::npos;
                if (crlf != std::string::npos) {
                    // 请求行长度超过1KB视为攻击尝试
                    if (crlf - parsed_pos_ > 1024) { 
                        isErr = true;
                        ok = false;
                        break;
                    }
                    
                    
                    ok = processRequestLine(    reinterpret_cast<const char*>(buffer_.data()) + parsed_pos_, 
                                                reinterpret_cast<const char*>(buffer_.data()) + crlf    ) ;
                    if (ok) {
                        request_.setReceiveTime(receiveTime);
                        parsed_pos_ = crlf + 2;
                        state_ = kExpectHeaders;
                    }
                    
                } else if (buffer_.size() - parsed_pos_ > 1024) {
                    isErr = true;
                    ok = false; // 异常长请求行防御
                }
                break;
            }
            
            case kExpectHeaders: {
                Logger::ngx_log_stderr(0, "kExpectHeaders...................");
                while(parsed_pos_ < buffer_.size()) {
                    // 定义要查找的 CRLF（"\r\n"）模式
                    const uint8_t crlf_pattern[] = {'\r', '\n'}; // 二进制形式的 "\r\n"
                    // 计算查找范围
                    auto start = buffer_.begin() + parsed_pos_;
                    auto end = buffer_.end();
                    // 使用 std::search 查找子序列
                    auto found = std::search(start, end, 
                                            std::begin(crlf_pattern), std::end(crlf_pattern));
                    // 计算位置或标记未找到
                    size_t crlf = (found != end) ? (found - buffer_.begin()) : std::string::npos;
                    if (crlf == std::string::npos) break;

                    // 空行检测
                    if (crlf == parsed_pos_) {
                        parsed_pos_ += 2;

                        if (request_.method() == HttpRequest::kPost || 
                            request_.method() == HttpRequest::kPut) {
                            auto contentLen = request_.getHeader("Content-Length");
                            if (contentLen.empty()) {
                                Logger::ngx_log_stderr(0, "contentLen.empty()");
                                isErr = true;
                                ok = false; // POST/PUT必须包含Content-Length[3](@ref)
                            } else if (!contentLen.empty()) {
                                Logger::ngx_log_stderr(0, "!contentLen.empty()");
                                try {

                                    request_.setContentLength(std::stoi(contentLen));
                                    Logger::ngx_log_stderr(0, "request_.contentLength() %d", request_.contentLength());
                                    state_ = (request_.contentLength() > 0) ? 
                                            kExpectBody : kGotAll;
                                } catch (...) {
                                    isErr = true;
                                    ok = false; // 非法Content-Length格式
                                }
                            }
                        } else {
                            Logger::ngx_log_stderr(0, "非post 或 put");
                            state_ = kGotAll;
                        }

                        break;
                    }

                    // 解析Header键值对
                    const char* colon = static_cast<const char*>(
                        memchr(buffer_.data() + parsed_pos_, ':', crlf - parsed_pos_));
                    if (colon) {
                        // Header解析
                        std::string key(reinterpret_cast<const char*>(buffer_.data()) + parsed_pos_, colon - (reinterpret_cast<const char*>(buffer_.data()) + parsed_pos_)); // 使用长度构造
                        std::string val(colon + 1, (reinterpret_cast<const char*>(buffer_.data()) + crlf) - (colon + 1)); // 使用迭代器构造
                        request_.addHeader(
                            std::move(key.erase(key.find_last_not_of(" \t") + 1)),
                            std::move(val.erase(0, val.find_first_not_of(" \t")))
                        );
                    } else {
                        isErr = true;
                        ok = false; // 非法Header格式
                    }
                    parsed_pos_ = crlf + 2;
                }
                // 检查 Content-Type 头
                std::string contentType = request_.getHeader("Content-Type");
                if (!contentType.empty())
                {
                    // 如果是 multipart/form-data，则需要提取 boundary
                    if (contentType.find("multipart/form-data") != std::string::npos)
                    {
                        // 查找 boundary 参数
                        size_t pos = contentType.find("boundary=");
                        if (pos != std::string::npos)
                        {
                            std::string boundary = contentType.substr(pos + 9);
                            // boundary 可能会带引号，去除引号
                            if (!boundary.empty() && boundary.front() == '\"')
                            {
                                boundary.erase(0, 1);
                                if (!boundary.empty() && boundary.back() == '\"')
                                    boundary.pop_back();
                            }
                            // 将 boundary 保存到 request_ 或相关对象中，供后续解析请求体时使用
                            request_.setBoundary(boundary); // 假设你有这样的方法
                        }
                        else
                        {
                            // 如果 multipart/form-data 却没有 boundary，则视为非法请求
                            isErr = true;
                            ok = false;
                        }
                    }
                }

                break;
            }
            
            case kExpectBody: {
                Logger::ngx_log_stderr(0, "kExpectBody...................");

                 // 先检查是否有 multipart 的 boundary
                std::string boundary = request_.boundary();
                if (boundary.empty()) {
                    Logger::ngx_log_stderr(0, "没有 boundary，则按普通 body 解析...................");
                    // 没有 boundary，则按普通 body 解析
                    size_t needed = request_.contentLength();
                    size_t remaining = buffer_.size() - parsed_pos_;
                    size_t bytes_to_copy = std::min(remaining, needed - body_bytes_received_);

                    request_.appendBody(reinterpret_cast<const char*>(buffer_.data()) + parsed_pos_, bytes_to_copy);

                    parsed_pos_ += bytes_to_copy;
                    body_bytes_received_ += bytes_to_copy;
                    if (body_bytes_received_ >= needed) {
                        state_ = kGotAll;
                    } else {
                        ok = false; // 需要更多数据
                    }
                } else {
                    Logger::ngx_log_stderr(0, "有 boundary，则按 multipart/form-data 解析上传内容..................");
                    // 有 boundary，则按 multipart/form-data 解析上传内容
                    size_t needed = request_.contentLength();
                    std::string boundary_delim = "--" + boundary;        // 正常 boundary
                    std::string boundary_end = "--" + boundary + "--";     // 结束 boundary
                    while (parsed_pos_ < buffer_.size()) {
                        // 头部解析
                        if(request_.getfileHeader().empty())
                        {
                            // 查找边界位置（替换原始 find 逻辑）
                            auto search = buffer_.begin() + parsed_pos_;
                            auto boundary_it = std::search(
                                search, buffer_.end(),
                                boundary_delim.begin(), boundary_delim.end()
                            );
                            size_t boundary_pos = (boundary_it != buffer_.end()) 
                                ? std::distance(buffer_.begin(), boundary_it) 
                                : std::string::npos;
                            if (boundary_pos == std::string::npos) {
                                // 没有找到完整的 Part，等待更多数据
                                ok = false;
                                break;
                            }

                            size_t part_start = boundary_pos + boundary_delim.size();
                            // 检查是否为结束 boundary
                            // 预定义结束边界标记（--）
                            constexpr std::array<uint8_t, 2> BOUNDARY_END_MARK = {'-', '-'};

                            // 安全比较逻辑
                            if (part_start + BOUNDARY_END_MARK.size() <= buffer_.size()) {
                                auto part_begin = buffer_.begin() + part_start;
                                auto part_end = part_begin + BOUNDARY_END_MARK.size();
                                
                                // 使用 std::equal 进行二进制比较
                                if (std::equal(part_begin, part_end, BOUNDARY_END_MARK.begin())) {
                                    state_ = kGotAll;  // 所有 Part 已解析完成
                                    break;
                                }
                            }

                            // 跳过 CRLF
                            // 预定义 CRLF 常量（放在类或全局作用域）
                            constexpr std::array<uint8_t, 2> CRLF = {'\r', '\n'}; // 二进制形式的 "\r\n"

                            // 安全检查与比较逻辑
                            if (part_start + CRLF.size() <= buffer_.size()) {
                                auto start = buffer_.begin() + part_start;
                                if (std::equal(start, start + CRLF.size(), CRLF.begin())) {
                                    part_start += CRLF.size(); // 安全跳过 CRLF
                                }
                            }

                            // 解析 Part 头部
                            // 预定义 CRLFCRLF 模式（\r\n\r\n）
                            constexpr std::array<uint8_t, 4> CRLF_CRLF = {'\r', '\n', '\r', '\n'};

                            // 计算查找范围（安全检查已省略，需确保 part_start <= buffer_.size()）
                            auto search_start = buffer_.begin() + part_start;
                            auto search_end = buffer_.end();

                            // 使用 std::search 查找子序列
                            auto found = std::search(search_start, search_end, 
                                                    CRLF_CRLF.begin(), CRLF_CRLF.end());

                            // 计算位置或标记未找到
                            size_t header_end = (found != search_end) 
                                ? std::distance(buffer_.begin(), found) 
                                : std::string::npos;
                            if (header_end == std::string::npos) {
                                // 不完整的 Part 头部，等待更多数据
                                ok = false;
                                break;
                            }

                            // 提取头部信息
                            std::string part_header(reinterpret_cast<const char*>(buffer_.data()) + part_start, header_end - part_start);
                            part_start = header_end + 4; // 跳过头部和空行
                            parsed_pos_ = part_start; // 更新解析位置
                            body_bytes_received_ += (part_header.size() + 4);
                            request_.savefileHeader(part_header);
                            if (request_.contentLength() > LARGE_FILE_THRESHOLD) {
                                // 初始化临时文件写入
                                MyConf* myconf = MyConf::getInstance();
                                const std::string uploadDir = myconf->GetString("Video_Path");
                                if (!std::filesystem::exists(uploadDir)) {
                                    std::filesystem::create_directories(uploadDir);
                                }
                                std::string path;
                                path.append(uploadDir);
                                path.append("/");
                                // 查找文件名
                                std::string filename;
                                size_t namePos = part_header.find("filename=\"");
                                if (namePos != std::string::npos) {
                                    size_t endQuote = part_header.find("\"", namePos + 10);
                                    filename = part_header.substr(
                                        namePos + 10,
                                        endQuote - (namePos + 10)
                                    );
                                    // 保存文件
                                    filename = sanitizeFilename(filename);
                                }else
                                {
                                    Logger::ngx_log_stderr(0, "find name failed...");
                                    filename = "Default";
                                }
                                path.append(filename);
                                request_.fileWriter_->initFileStream(path);
                            }
                            // ok = false;
                            // break;
                        }
                        else
                        {
                            // 解析 Part 数据

                            auto found = std::search(
                                buffer_.begin() + parsed_pos_, buffer_.end(),
                                boundary_end.begin(), boundary_end.end()
                            );
                            size_t part_end = found != buffer_.end() ? std::distance(buffer_.begin(), found) : std::string::npos;

                            Logger::ngx_log_stderr(0, "body_bytes_received_ : %d", body_bytes_received_);
                            Logger::ngx_log_stderr(0, "part_end : %d", part_end);
                            Logger::ngx_log_stderr(0, "boundary_end.size() : %d", boundary_end.size());
                            if (part_end == std::string::npos || (body_bytes_received_ + part_end - boundary_end.size()) < needed) {
                                // 没有找到完整的 Part 数据
                                part_end = std::min(needed - boundary_end.size() - body_bytes_received_, buffer_.size());
                                // 保存文件内容
                                if (request_.contentLength() > LARGE_FILE_THRESHOLD)
                                {
                                    request_.fileWriter_->writeToFile(reinterpret_cast<const char*>(buffer_.data()) + parsed_pos_, part_end - parsed_pos_);
                                    // request_.savefileHeader(part_header);
                                }else
                                {
                                    request_.appendBody(reinterpret_cast<const char*>(buffer_.data()) + parsed_pos_, part_end - parsed_pos_); 
                                    // request_.savefileHeader(part_header);
                                }
                                // if(parsed_pos_ > 4 * 1024)
                                // {
                                    body_bytes_received_ += (part_end - parsed_pos_);
                                    parsed_pos_ = part_end;
                                    std::vector<uint8_t>(buffer_.begin() + parsed_pos_, buffer_.end()).swap(buffer_);
                                    parsed_pos_ = 0;
                                // }
                                if (part_end == std::string::npos || (body_bytes_received_ + part_end - boundary_end.size()) < needed)
                                {
                                    ok = false;
                                    break;
                                }
                            }

                            size_t part_size = part_end - parsed_pos_;
                            Logger::ngx_log_stderr(0, "needed : %d", needed);
                            Logger::ngx_log_stderr(0, "part_start : %d", parsed_pos_);
                            Logger::ngx_log_stderr(0, "part_end : %d", part_end);
                            Logger::ngx_log_stderr(0, "buffer size : %d", buffer_.size());
                            // std::string part_data(reinterpret_cast<const char*>(buffer_.data()) + part_start, part_size);

                            // 根据 Part 头部信息，处理数据

                            if (request_.contentLength() > LARGE_FILE_THRESHOLD)
                            {                                   
                                request_.fileWriter_->writeToFile(reinterpret_cast<const char*>(buffer_.data()) + parsed_pos_, part_size);
                                // request_.savefileHeader(part_header);
                            }else
                            {
                                request_.appendBody(reinterpret_cast<const char*>(buffer_.data()) + parsed_pos_, part_size); 
                                // request_.savefileHeader(part_header);
                            }
                            // body_bytes_received_ += (part_end - parsed_pos_);
                            // 移动解析位置到 Part 末尾
                            parsed_pos_ = part_end + boundary_end.size();
                            std::vector<uint8_t>(buffer_.begin() + parsed_pos_, buffer_.end()).swap(buffer_);
                            parsed_pos_ = 0;
                            // ok = true;
                            state_ = kGotAll;
                            break;
                        }
       
                    }
                }
                break;
            }
            
            case kGotAll:
                // 处理完成后滑动窗口
                // if (parsed_pos_ > 4096) {
                //     buffer_.erase(0, parsed_pos_);
                //     parsed_pos_ = 0;
                // } else if (parsed_pos_ > 0) {
                //     buffer_.erase(0, parsed_pos_);
                //     parsed_pos_ = 0;
                // }
                return true;
        }
        
        if (!ok || (state_ != kGotAll && parsed_pos_ >= buffer_.size())) 
            break;
    }

    // 如果解析失败，重置状态
    if (isErr && !ok) {
        reset();
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