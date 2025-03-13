// #include <string>
// #include <unordered_map>
// #include <fstream>
// #include <sstream>
// #include <stdexcept>


// namespace WYXB
// {

// class HttpGetProcessor {


//     friend class CSocket;

    
// public:
//     // 构造函数：指定资源根目录和严格模式
//     explicit HttpGetProcessor(const std::string& resourceRoot = "./www", 
//                                 bool strictMode = false)
//         : resourceRoot_(resourceRoot), strictMode_(strictMode) {}

//     // 主处理接口：输入原始请求，输出完整HTTP响应
//     std::string processRequest(const std::string& rawRequest) {
//         try {
//             auto params = parseRequestLine(rawRequest);
//             return buildHtmlResponse(params);
//         } catch (const std::exception& e) {
//             return generateErrorResponse(500, "Internal Server Error: " + std::string(e.what()));
//         }
//     }

// private:
//     std::string resourceRoot_;
//     bool strictMode_;

//     // 核心解析方法
//     std::unordered_map<std::string, std::string> parseRequestLine(const std::string& request) ;

//     // 响应生成方法
//     std::string buildHtmlResponse(const std::unordered_map<std::string, std::string>& params) ;

//     // 错误响应生成
//     std::string generateErrorResponse(int code, const std::string& message) ;

//     // URL解码辅助方法
//     static std::string urlDecode(const std::string& str) ;
// };
// }
