#include <cstddef>
#include <cstring>
#include <stdexcept>

class ZeroCopyBuffer {
public:
    ZeroCopyBuffer(char* buffer, std::size_t size)
        : buffer_(buffer), size_(size), position_(0) {}

    // 获取当前写入位置的指针和剩余可用大小
    std::pair<char*, std::size_t> get_write_area() {
        return { buffer_ + position_, size_ - position_ };
    }

    // 更新写入位置
    void advance(std::size_t bytes) {
        if (position_ + bytes > size_) {
            throw std::runtime_error("超出缓冲区大小");
        }
        position_ += bytes;
    }

    // 重置缓冲区
    void reset() {
        position_ = 0;
    }

    // 获取当前缓冲区内容的指针和大小
    std::pair<const char*, std::size_t> data() const {
        return { buffer_, position_ };
    }

private:
    char* buffer_;
    std::size_t size_;
    std::size_t position_;
};
