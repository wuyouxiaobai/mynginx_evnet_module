#include "AuthMiddleware.h"
#include "my_macro.h"
#include "HttpException.h"

namespace WYXB {

void AuthMiddleware::before(HttpRequest& req) {
    std::string authHeader = req.getHeader("Authorization");


    const std::string prefix = "Bearer ";
    if (authHeader.size() <= prefix.size() || authHeader.substr(0, prefix.size()) != prefix) {
        Logger::ngx_log_error_core(NGX_LOG_WARN, 0, "AuthMiddleware: Header format invalid");
        throw HttpException(HttpResponse::HttpStatusCode::k401Unauthorized , "Unauthorized: Invalid header format");
    }

    std::string token = authHeader.substr(prefix.size());
    if (!validator_(token)) {
        Logger::ngx_log_error_core(NGX_LOG_WARN, 0, "AuthMiddleware: Token invalid");
        throw HttpException(HttpResponse::HttpStatusCode::k403Forbidden, "Forbidden: Token verification failed");
    }

    Logger::ngx_log_error_core(NGX_LOG_WARN, 0, "AuthMiddleware: Authentication passed");
}

void AuthMiddleware::after(HttpResponse& res) {
    // 你可以在这里添加审计日志、Token刷新逻辑等
}


}
