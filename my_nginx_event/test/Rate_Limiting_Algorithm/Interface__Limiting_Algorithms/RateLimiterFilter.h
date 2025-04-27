// RateLimiterFilter.h
#pragma once
#include "IFilter.h"
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace WYXB {

class TokenBucket
{
public:
    TokenBucket(double rate, double burst)
        : rate_(rate), burst_(burst), tokens_(burst),
          last_time_(std::chrono::steady_clock::now()) {}

    bool getToken();

private:
    double rate_;
    double burst_;
    double tokens_;
    std::chrono::steady_clock::time_point last_time_;
};

class RateLimiterFilter : public IFilter
{
public:
    RateLimiterFilter(double rate, double burst) 
        : rate_(rate), burst_(burst) {}

    void doFilter(const HttpRequestPtr& req,
                  FilterCallback&& fcb,
                  FilterChainCallback&& fccb) override;

private:
    double rate_;
    double burst_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<TokenBucket>> buckets_;
};

} // namespace WYXB
