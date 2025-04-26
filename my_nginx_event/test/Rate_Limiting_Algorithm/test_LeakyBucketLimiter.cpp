#include <iostream>
#include <thread>
#include "LeakyBucketLimiter.h"

int main() {
    LeakyBucketLimiter limiter(5, 10);  // 每秒漏5个，最多积压10个请求

    for (int i = 0; i < 20; ++i) {
        if (limiter.tryConsume()) {
            std::cout << "请求 " << i << " 通过\n";
        } else {
            std::cout << "请求 " << i << " 被限流\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 模拟每100ms一次请求
    }
}
