#include <iostream>
#include "ZeroCopyBuffer.h"

int main() {
    char buffer[1024];
    ZeroCopyBuffer zcb(buffer, sizeof(buffer));

    // 获取写入区域
    auto [write_ptr, write_size] = zcb.get_write_area();

    // 模拟写入数据
    const char* message = "Hello, Zero-Copy!";
    std::size_t message_len = std::strlen(message);

    if (write_size >= message_len) {
        std::memcpy(write_ptr, message, message_len);
        zcb.advance(message_len);
    }

    // 获取已写入的数据
    auto [data_ptr, data_size] = zcb.data();
    std::cout.write(data_ptr, data_size);
    std::cout << std::endl;

    return 0;
}
