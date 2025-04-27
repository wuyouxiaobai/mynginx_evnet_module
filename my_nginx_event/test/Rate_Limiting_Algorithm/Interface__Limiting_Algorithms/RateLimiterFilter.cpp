// RateLimiterFilter.cpp
#include "RateLimiterFilter.h"

namespace WYXB {

bool TokenBucket::getToken() {
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - last_time_).count();
    tokens_ = std::min(burst_, tokens_ + rate_ * elapsed);
    last_time_ = now;
    if (tokens_ >= 1.0) {
        tokens_ -= 1.0;
        return true;
    }
    return false;
}

void RateLimiterFilter::doFilter(const HttpRequestPtr& req,
                                 FilterCallback&& fcb,
                                 FilterChainCallback&& fccb) {
    std::string key = req->getPeerIp(); // 默认按 IP 限流
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& bucket = buckets_[key];
        if (!bucket) {
            bucket = std::make_shared<TokenBucket>(rate_, burst_);
        }
        if (!bucket->getToken()) {
            auto resp = std::make_shared<HttpResponse>();
            resp->setStatusCode(HttpResponse::HttpStatusCode::k429TooManyRequests);
            resp->setBody("Too Many Requests");
            fcb(std::move(resp));
            return;
        }
    }
    fccb();
}

} // namespace WYXB
