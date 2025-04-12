#pragma once
#include <functional>
#include "middleware.h"
#include "my_logger.h"
#include "my_http_request.h"
#include "my_http_response.h"

namespace WYXB {

class AuthMiddleware : public Middleware {
public:
    explicit AuthMiddleware(const std::string& validToken)
        : expectedToken_(validToken) {}

    void before(HttpRequest& req) override;
    void after(HttpResponse& res) override;

private:
    std::string expectedToken_;
    using TokenValidator = std::function<bool(const std::string&)>;

    TokenValidator validator_;
};

}
