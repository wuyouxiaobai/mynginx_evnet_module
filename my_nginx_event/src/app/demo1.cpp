#include "my_server.h"

#include "middleware/Rate_Limiting_Algorithm/RateLimiterMiddleware.h"
/// 路由相关
// 初始化时注册
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


int main(int argc, char*const *argv)
{
    
    // 示例：注册动态路径心跳
    // server.get(  
    //     "/heartbeat/\\d+",  // 匹配类似 /heartbeat/123
    //     [](const WYXB::HttpRequest& req, WYXB::HttpResponse* resp) {
    //         resp->setStatusCode(WYXB::HttpResponse::HttpStatusCode::k200Ok);
    //         resp->setBody("Dynamic PONG");
    //     }
    // );


    WYXB::Server::instance().get( 
        "/hello", 
        [](const WYXB::HttpRequest& req, WYXB::HttpResponse* resp) {
            resp->setStatusLine(req.getVersion(), WYXB::HttpResponse::HttpStatusCode::k200Ok, "OK");
            // resp->addHeader("Keep-Alive", "timeout=500, max=1000");  
            resp->setContentType("text/plain; charset=utf-8"); // 显式设置编码
            std::string respstr = "hello world";
            resp->setBody(respstr); // 直接返回中文文本
            resp->setContentLength(respstr.size());
        }
    );

    // 新增HTML文件路由
    WYXB::Server::instance().get( 
        "/getposthtml",
        [](const WYXB::HttpRequest& req, WYXB::HttpResponse* resp) {
            // 设置响应头
            resp->setStatusLine(req.getVersion(), WYXB::HttpResponse::HttpStatusCode::k200Ok, "OK");
            resp->setContentType("text/html; charset=utf-8");
            // 读取HTML文件
            std::ifstream file("../html/posttest.html", std::ios::in | std::ios::binary);
            if (file) {
                std::string content((std::istreambuf_iterator<char>(file)), 
                                    std::istreambuf_iterator<char>());
                resp->setBody(content);
                resp->setContentLength(content.size());
            } else {
                resp->setStatusCode(WYXB::HttpResponse::HttpStatusCode::k404NotFound);
                resp->setBody("HTML File Not Found");
            }
        }
    );

    WYXB::Server::instance().post(
        "/submit",
        [](const WYXB::HttpRequest& req, WYXB::HttpResponse* resp) {
            try {
                
                // 设置响应头
                resp->setStatusLine(req.getVersion(), WYXB::HttpResponse::HttpStatusCode::k200Ok, "OK");
                resp->setContentType("text/html; charset=utf-8");
                // 解析表单数据
                std::vector<uint8_t> body = req.getBody();
                std::string prefix = "testData=";
                // 将字符串转换为字节序列
                std::vector<uint8_t> prefix_bytes(prefix.begin(), prefix.end());

                // 使用 std::search 算法查找字节序列
                auto it = std::search(
                    body.begin(), 
                    body.end(),
                    prefix_bytes.begin(),
                    prefix_bytes.end()
                );

                // 判断是否找到并计算位置
                size_t pos = (it != body.end()) ? std::distance(body.begin(), it) : std::string::npos;
                
                if (pos == std::string::npos) {
                    throw std::runtime_error("Invalid form data");
                }
    
                // URL解码并获取值
                auto start = body.begin() + pos + prefix.size();
                std::vector<uint8_t> encoded(start, body.end());
                std::string encoded_str(encoded.begin(), encoded.end());
                std::string decoded;
                for (size_t i = 0; i < encoded.size(); ++i) {
                    if (encoded_str[i] == '%' && i+2 < encoded_str.size()) {
                        int hex;
                        sscanf(encoded_str.substr(i+1,2).c_str(), "%x", &hex);
                        decoded += static_cast<char>(hex);
                        i += 2;
                    } else if (encoded_str[i] == '+') {
                        decoded += ' ';
                    } else {
                        decoded += encoded_str[i];
                    }
                }
    
                // 构造JSON响应
                nlohmann::json response = {
                    {"status", "success"},
                    {"message", "数据接收成功"},
                    {"input", decoded},
                    {"length", decoded.length()},
                    {"timestamp", static_cast<long>(std::time(nullptr))}
                };
    
                resp->setStatusCode(WYXB::HttpResponse::k200Ok);
                resp->setContentType("application/json; charset=utf-8");
                resp->setBody(response.dump());
                resp->setContentLength(response.dump().size());
    
            } catch (const std::exception& e) {
                nlohmann::json error = {
                    {"status", "error"},
                    {"error_type", "invalid_request"},
                    {"message", e.what()}
                };
                resp->setStatusCode(WYXB::HttpResponse::k400BadRequest);
                resp->setContentType("application/json; charset=utf-8");
                resp->setBody(error.dump());
                resp->setContentLength(error.dump().size());
            }
        }
    );


    WYXB::Server::instance().get(
        "/gethtml",
        [](const WYXB::HttpRequest& req, WYXB::HttpResponse* resp) {
            // 设置响应头
            resp->setStatusLine(req.getVersion(), WYXB::HttpResponse::HttpStatusCode::k200Ok, "OK");
            resp->setContentType("text/html; charset=utf-8");
            // 读取HTML文件
            std::ifstream file("../html/test.html", std::ios::in | std::ios::binary);
            if (file) {
                std::string content((std::istreambuf_iterator<char>(file)), 
                                    std::istreambuf_iterator<char>());
                resp->setBody(content);
                resp->setContentLength(content.size());
            } else {
                resp->setStatusCode(WYXB::HttpResponse::HttpStatusCode::k404NotFound);
                resp->setBody("HTML File Not Found");

            }
        }
    );

    WYXB::Server::instance().post(
        "/uploadvideo",
        [](const WYXB::HttpRequest& req, WYXB::HttpResponse* resp) 
        {
            // 1. 确保上传目录存在
            resp->setStatusLine(req.getVersion(), WYXB::HttpResponse::HttpStatusCode::k200Ok, "OK");
            
            WYXB::Logger::ngx_log_stderr(0 ,"uploadvideo test");
            const std::string uploadDir = "../video/";
            if (!std::filesystem::exists(uploadDir)) {
                std::filesystem::create_directories(uploadDir);
            }
    
            // 2. 解析Content-Type获取boundary
            std::string fileHeader = req.getfileHeader();
            if (fileHeader.empty()) {
                resp->setStatusCode(WYXB::HttpResponse::k400BadRequest);
                resp->setBody("Invalid Content-Type");
                return;
            }
    
            // 3. 获取原始请求体
            std::vector<uint8_t> body = req.getBody();

    
            // 查找文件名
            size_t namePos = fileHeader.find("filename=\"");
            if (namePos != std::string::npos) {
                size_t endQuote = fileHeader.find("\"", namePos + 10);
                std::string filename = fileHeader.substr(
                    namePos + 10,
                    endQuote - (namePos + 10)
                );


                if(req.contentLength() <= 1 * 1024 * 1024)
                {
                    // 保存文件
                    filename = sanitizeFilename(filename);
                    std::ofstream out(uploadDir + filename, std::ios::binary);
                    out.write(reinterpret_cast<const char*>(body.data()), body.size() * sizeof(body[0]));
                }
                resp->setStatusCode(WYXB::HttpResponse::k200Ok);

                nlohmann::json root;
                root["status"] = "success";
                root["message"] = "Upload success";
                
                // 遍历目录获取文件列表
                std::vector<std::string> newList;
                for (const auto& entry : std::filesystem::directory_iterator(uploadDir)) {
                    newList.push_back(entry.path().filename().string());
                }
                
                // 直接将 vector 赋值给 JSON 数组（自动转换）
                root["files"] = newList;  // 或使用 root["files"] = nlohmann::json::array(newList);
                
                // 设置响应内容（自动序列化）
                resp->setContentType("application/json");
                resp->setBody(root.dump());  // 默认无缩进，等同于 Json::FastWriter
                resp->setContentLength(root.dump().size());
                return;
            }


    
            // 未找到有效文件
            resp->setStatusCode(WYXB::HttpResponse::k400BadRequest);
            resp->setBody("No video file received");
        }
    );
    
    WYXB::Server::instance().get(
        "/getvideos",
        [](const WYXB::HttpRequest& req, WYXB::HttpResponse* resp) {
            const std::string videoDir = "../video/";
            nlohmann::json fileList = nlohmann::json::array();
            
            for (const auto& entry : std::filesystem::directory_iterator(videoDir)) {
                fileList.push_back(entry.path().filename().string());
            }
            resp->setStatusLine(req.getVersion(), WYXB::HttpResponse::HttpStatusCode::k200Ok, "OK");
            resp->setContentType("application/json");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->setBody(fileList.dump());
            resp->setContentLength(fileList.dump().size());
        }
    );


    WYXB::Server::instance().get(
        "/video/:filename",
        [](const WYXB::HttpRequest& req, WYXB::HttpResponse* resp) {
            // 获取 '/video/' 后面的文件名
            std::string path = req.path().substr(7);
            std::string videoPath = "../video/" + path;
            
            // 文件不存在处理
            if (!std::filesystem::exists(videoPath)) {
                WYXB::Logger::ngx_log_stderr(0, "Video not found");
                resp->setStatusCode(WYXB::HttpResponse::k404NotFound);
                resp->setBody("Video not found");
                return;
            }
            
            // 获取文件总大小
            std::uintmax_t fileSize = std::filesystem::file_size(videoPath);
            
            // 打开文件流
            std::ifstream videoFile(videoPath, std::ios::binary);
            if (!videoFile) {
                resp->setStatusCode(WYXB::HttpResponse::k500InternalServerError);
                resp->setBody("Failed to open video file");
                return;
            }
            
            // 检查是否存在 Range 请求头
            std::string rangeHeader = req.getHeader("Range");
            if (!rangeHeader.empty()) {
                size_t pos = rangeHeader.find('=');
                if (pos != std::string::npos) {
                    std::string rangeSpec = rangeHeader.substr(pos + 1);
                    size_t dashPos = rangeSpec.find('-');
                    if (dashPos != std::string::npos) {
                        // 解析起始字节
                        long long start = std::stoll(rangeSpec.substr(0, dashPos));
                        long long end = fileSize - 1;  // 默认到文件末尾
                        if (dashPos + 1 < rangeSpec.size() && !rangeSpec.substr(dashPos + 1).empty()) {
                            end = std::stoll(rangeSpec.substr(dashPos + 1));
                        }
                        if (start > end || end >= fileSize) {
                            resp->setStatusCode(WYXB::HttpResponse::k416RangeNotSatisfiable);
                            resp->setBody("Requested Range Not Satisfiable");
                            return;
                        }
                        
                        // 计算需要传输的字节数
                        std::size_t contentLength = static_cast<std::size_t>(end - start + 1);
                        videoFile.seekg(start);
                        
                        // 设置响应头：206状态，Content-Range 以及 chunked 编码（不指定 Content-Length）
                        resp->setStatusLine(req.getVersion(), WYXB::HttpResponse::k206PartialContent, "Partial Content");
                        resp->addHeader("Content-Range", "bytes " + std::to_string(start) + "-" + std::to_string(end) + "/" + std::to_string(fileSize));
                        resp->addHeader("Accept-Ranges", "bytes");
                        resp->addHeader("Transfer-Encoding", "chunked");
                        resp->setContentType("video/mp4");
                        
                        // 分块读取并构造 chunked 编码数据
                        const size_t CHUNK_SIZE = 8 * 1024; // 8kb
                        std::string chunkedBody;
                        size_t bytesRemaining = contentLength;
                        // while (bytesRemaining > 0) {
                            size_t toRead = std::min(CHUNK_SIZE, bytesRemaining);
                            std::vector<char> buffer(toRead);
                            videoFile.read(buffer.data(), toRead);
                            std::streamsize bytesRead = videoFile.gcount();
                            if (bytesRead <= 0) return;
                            // 构造当前 chunk，格式为：<chunk size in hex>\r\n<data>\r\n
                            std::ostringstream oss;
                            oss << std::hex << bytesRead << "\r\n";
                            chunkedBody.append(oss.str());
                            chunkedBody.append(buffer.data(), bytesRead);
                            chunkedBody.append("\r\n");
                            bytesRemaining -= bytesRead;
                        // }
                        // 添加终止 chunk
                        chunkedBody.append("0\r\n\r\n");
                        resp->setBody(chunkedBody);
                        return;
                    }
                }
            }
            
            // 如果没有 Range 请求，返回整个文件（使用 chunked 传输）
            resp->setStatusLine(req.getVersion(), WYXB::HttpResponse::k200Ok, "OK");
            resp->setContentType("video/mp4");
            resp->addHeader("Transfer-Encoding", "chunked");
            const size_t CHUNK_SIZE = 8 * 1024;
            std::string chunkedBody;
            while (videoFile) {
                std::vector<char> buffer(CHUNK_SIZE);
                videoFile.read(buffer.data(), CHUNK_SIZE);
                std::streamsize bytesRead = videoFile.gcount();
                if (bytesRead <= 0)
                    break;
                std::ostringstream oss;
                oss << std::hex << bytesRead << "\r\n";
                chunkedBody.append(oss.str());
                chunkedBody.append(buffer.data(), bytesRead);
                chunkedBody.append("\r\n");
            }
            chunkedBody.append("0\r\n\r\n");
            resp->setBody(chunkedBody);
        }
    );
    
    
    
    
    // m_Router->registerCallback(
    //     HttpRequest::Method::kGet,
    //     "/getvideo/(.+)", // 正则匹配文件名
    //     [](const HttpRequest& req, HttpResponse* resp) {
    //         const std::string videoPath = "../video/" + req.getPath().substr(10); // 提取文件名
            
    //         std::ifstream file(videoPath, std::ios::binary | std::ios::ate);
    //         if (!file) {
    //             resp->setStatusCode(HttpResponse::k404NotFound);
    //             return;
    //         }
    
    //         // 设置视频流响应头
    //         resp->setContentType("video/mp4");
    //         resp->addHeader("Accept-Ranges", "bytes");
            
    //         // 读取文件内容
    //         std::streamsize size = file.tellg();
    //         file.seekg(0, std::ios::beg);
    //         std::vector<char> buffer(size);
    //         if (file.read(buffer.data(), size)) {
    //             resp->setBody(std::string(buffer.data(), buffer.size()));
    //         }
    //     }
    // );
    // 添加限流中间件（全局100QPS，突发不超过200）
    WYXB::Server::instance().addMiddleware(std::make_shared<WYXB::RateLimiterMiddleware>(200, 100.0));


    //示例使用内存存储
    // 创建会话存储
    auto sessionStorage = std::make_unique<WYXB::MemorySessionStorage>(); 
    // 创建会话管理器
    auto sessionManager = std::make_unique<WYXB::SessionManager>(std::move(sessionStorage));
    // 设置会话管理器
    WYXB::Server::instance().setSessionManager(std::move(sessionManager));


    // 获得mysql连接
    auto conn = WYXB::Server::instance().getConn();

    int exitcode = WYXB::Server::instance().run(argc, argv);
    return exitcode;
}

