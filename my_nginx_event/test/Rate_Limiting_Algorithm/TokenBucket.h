#include <chrono>
#include <mutex>

class TokenBucket {
public:
    TokenBucket(double rate, double capacity)
        : rate_(rate), capacity_(capacity), tokens_(capacity),
          last_refill_time_(std::chrono::steady_clock::now()) {}

    // 尝试获取一个令牌，成功返回 true，失败返回 false
    bool try_acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        refill(); // 更新令牌数量
        if (tokens_ >= 1.0) {
            tokens_ -= 1.0;
            return true;
        }
        return false;
    }

private:
    void refill() {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - last_refill_time_).count();
        double added_tokens = elapsed * rate_;
        if (added_tokens > 0) {
            tokens_ = std::min(capacity_, tokens_ + added_tokens);
            last_refill_time_ = now;
        }
    }

private:
    double rate_;                    // 每秒产生的令牌数
    double capacity_;                // 桶的最大容量
    double tokens_;                  // 当前令牌数
    std::chrono::steady_clock::time_point last_refill_time_;
    std::mutex mutex_;              // 保证线程安全
};
