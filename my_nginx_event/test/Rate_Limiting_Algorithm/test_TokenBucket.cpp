#include <iostream>
#include <thread>
#include "TokenBucket.h"

int main() {
    TokenBucket limiter(5.0, 10.0); // 每秒 5 个令牌，最大 10 个

    for (int i = 0; i < 20; ++i) {
        if (limiter.try_acquire()) {
            std::cout << "Request " << i << " allowed.\n";
        } else {
            std::cout << "Request " << i << " throttled.\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}
