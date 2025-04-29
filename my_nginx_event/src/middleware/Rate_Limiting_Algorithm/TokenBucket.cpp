// TokenBucket.h
#pragma once
#include <chrono>
#include <mutex>
#include <atomic>
#include "TokenBucket.h"

namespace WYXB{



bool TokenBucket::try_consume(uint32_t tokens = 1) {
    std::lock_guard<std::mutex> lock(mutex_);
    refill_tokens(); // 先补充令牌
    
    if (tokens_ >= tokens) {
        tokens_ -= tokens;
        return true;
    }
    return false;
}


void TokenBucket::refill_tokens() {
    auto now = std::chrono::steady_clock::now();
    double elapsed_sec = std::chrono::duration<double>(now - last_time_).count();
    last_time_ = now;
    
    double new_tokens = elapsed_sec * fill_rate_;
    tokens_ = std::min(static_cast<double>(capacity_), tokens_ + new_tokens);
}



}