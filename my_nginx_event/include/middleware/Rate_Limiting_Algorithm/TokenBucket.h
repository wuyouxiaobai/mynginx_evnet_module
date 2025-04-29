// TokenBucket.h
#pragma once
#include <chrono>
#include <mutex>
#include <atomic>

namespace WYXB{

class TokenBucket {
public:
    TokenBucket(uint32_t capacity, double fill_rate_per_sec) 
        : capacity_(capacity),
          tokens_(capacity),
          fill_rate_(fill_rate_per_sec),
          last_time_(std::chrono::steady_clock::now()) {}

    bool try_consume(uint32_t tokens = 1) ;

private:
    void refill_tokens() ;
    std::mutex mutex_;
    uint32_t capacity_;  // 桶总容量
    double tokens_;      // 当前令牌数（允许小数积累）
    double fill_rate_;   // 令牌填充速率（个/秒）
    std::chrono::steady_clock::time_point last_time_; // 最后更新时间
};

}