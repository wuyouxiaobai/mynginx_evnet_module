#pragma once
#include <chrono>
#include <mutex>

class LeakyBucketLimiter {
public:
    LeakyBucketLimiter(double leak_rate_per_sec, int capacity)
        : leak_rate_(leak_rate_per_sec), capacity_(capacity), water_(0) {
        last_leak_time_ = std::chrono::steady_clock::now();
    }

    // 尝试请求通过
    bool tryConsume(int amount = 1) {
        std::lock_guard<std::mutex> lock(mutex_);
        leak();  // 先漏水
        if (water_ + amount <= capacity_) {
            water_ += amount;
            return true;  // 请求成功
        } else {
            return false; // 桶满，请求失败
        }
    }

private:
    void leak() {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - last_leak_time_).count();
        int leaked = static_cast<int>(elapsed * leak_rate_);

        if (leaked > 0) {
            water_ = std::max(0, water_ - leaked);
            last_leak_time_ = now;
        }
    }

private:
    double leak_rate_;         // 每秒漏出的水量（处理速率）
    int capacity_;             // 桶的容量
    int water_;                // 当前桶内的水量（积压请求）
    std::chrono::steady_clock::time_point last_leak_time_;
    std::mutex mutex_;
};
