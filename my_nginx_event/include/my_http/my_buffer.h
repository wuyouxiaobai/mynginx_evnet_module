#pragma once

#include <vector>
#include <cstddef>
#include <string>
#include <algorithm>

namespace WYXB
{

// 网络库底层缓冲区类
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;  // 预留空间大小
    static const size_t kInitialSize = 4096;  // 初始缓冲区大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(initialSize + kCheapPrepend),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {}
    
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }
    
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }
    
    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // 复位缓冲区
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }
    
    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

  
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    // 把[data, data+len]的内容追加到缓冲区
    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    } 


private:

    char* begin()
    {
        return &*buffer_.begin();
    }

    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        if(writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = kCheapPrepend + readable;
        }
    }

    std::vector<char> buffer_;  // 缓冲区数据
    size_t readerIndex_;         // 读指针位置
    size_t writerIndex_;         // 写指针位置




};


}