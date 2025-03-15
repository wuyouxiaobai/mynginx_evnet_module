// #include "my_http.h"
// #include "my_socket.h"


// namespace WYXB
// {
// // 核心解析方法
// std::unordered_map<std::string, std::string> HttpGetProcessor::parseRequestLine(const std::string& request) {
//     std::unordered_map<std::string, std::string> result;

//     // 提取请求行（首行内容）
//     size_t lineEnd = request.find("\r\n");
//     std::string requestLine = request.substr(0, lineEnd);

//     // 分割方法、路径和协议
//     size_t firstSpace = requestLine.find(' ');
//     size_t secondSpace = requestLine.find(' ', firstSpace + 1);
//     std::string pathPart = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);

//     // 提取路径和查询参数
//     size_t queryStart = pathPart.find('?');
//     result["path"] = (queryStart != std::string::npos) ? 
//         pathPart.substr(0, queryStart) : pathPart;

//     // 解析查询参数
//     if (queryStart != std::string::npos) {
//         std::string query = pathPart.substr(queryStart + 1);
//         std::stringstream ss(query);
//         std::string pair;
        
//         while (std::getline(ss, pair, '&')) {
//             size_t eqPos = pair.find('=');
//             if (eqPos != std::string::npos) {
//                 std::string key = urlDecode(pair.substr(0, eqPos));
//                 std::string val = urlDecode(pair.substr(eqPos + 1));
//                 result[key] = val;
//             }
//         }
//     }
//     return result;
// }

// // URL解码辅助函数
// std::string HttpGetProcessor::urlDecode(const std::string& str) {
//     std::string decoded;
//     for (size_t i = 0; i < str.length(); ++i) {
//         if (str[i] == '%' && i + 2 < str.length()) {
//             int hexVal;
//             sscanf(str.substr(i + 1, 2).c_str(), "%x", &hexVal);
//             decoded += static_cast<char>(hexVal);
//             i += 2;
//         } else if (str[i] == '+') {
//             decoded += ' ';
//         } else {
//             decoded += str[i];
//         }
//     }
//     return decoded;
// }

// // 响应生成方法
// std::string HttpGetProcessor::buildHtmlResponse(
//     const std::unordered_map<std::string, std::string>& params) 
// {
//     // 获取请求路径
//     std::string path = params.count("path") ? params.at("path") : "/";
    
//     // 默认文件处理
//     if (path == "/") path = "/index.html";
    
//     // 构造完整文件路径
//     std::string fullPath = resourceRoot_ + path;

//     // 尝试读取文件
//     std::ifstream file(fullPath, std::ios::binary);
//     if (!file) {
//         return "HTTP/1.1 404 Not Found\r\n"
//                "Content-Type: text/html\r\n\r\n"
//                "<h1>404 - File Not Found</h1>";
//     }

//     // 读取文件内容
//     std::string content(
//         (std::istreambuf_iterator<char>(file)),
//         std::istreambuf_iterator<char>()
//     );

//     // 生成响应头
//     return "HTTP/1.1 200 OK\r\n"
//            "Content-Type: text/html\r\n"
//            "Content-Length: " + std::to_string(content.size()) + "\r\n"
//            "\r\n" + content;
// }

// // 错误响应生
// std::string HttpGetProcessor::generateErrorResponse(int code, const std::string& message) {
//     std::string body = "<h1>" + std::to_string(code) + " " + message + "</h1>";
//     return "HTTP/1.1 " + std::to_string(code) + " " + message + "\r\n"
//             "Content-Type: text/html\r\n"
//             "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
// }



// }
