// RateLimiterMiddleware.h
#pragma once
#include "RateLimiterMiddleware.h"
#include "HttpException.h"

namespace WYXB {



void RateLimiterMiddleware::before(HttpRequest& req){
    if (!bucket_.try_consume()) {
        throw HttpException(HttpResponse::k429TooManyRequests, "Too Many Requests");
    }
}

void RateLimiterMiddleware::after(HttpResponse& res) {
    // 限流无需后处理，直接传递

}


} // namespace WYXB