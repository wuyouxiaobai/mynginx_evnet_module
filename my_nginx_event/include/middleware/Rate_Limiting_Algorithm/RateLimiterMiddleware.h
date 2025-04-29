// RateLimiterMiddleware.h
#pragma once
#include "middleware.h"
#include "TokenBucket.h"
#include "my_logger.h"
#include "my_http_request.h"
#include "my_http_response.h"

namespace WYXB {


class RateLimiterMiddleware : public Middleware {
public:
    RateLimiterMiddleware(uint32_t capacity, double fill_rate) 
        : bucket_(capacity, fill_rate) {}

    void before(HttpRequest& req) override;

    void after(HttpResponse& res) override ;

private:
    TokenBucket bucket_;
};

} // namespace WYXB