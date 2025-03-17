// #pragma once

// #include <iostream>
// #include <vector>
// #include <mutex>
// #include <memory>
// #include <cassert>
// #include <cstdlib>


// namespace WYXB
// {
// class Block
// {
// public:
//     Block(void* ptr) : mptr(ptr), isUse(false) {}
//     void* getPtr() const { return mptr; }
//     bool isInUsed() const { return isUse; }
//     void setUse(bool use) { isUse = use; }
// private:
//     void* mptr;
//     bool isUse;


// };

// template<typename T>
// class MemoryPool
// {
// public:
//     MemoryPool(size_t blockCount)
//         : mBlockSize_(sizeof(T)), mBlockCount_(blockCount)
//     {
//         pool_ = std::malloc(mBlockSize_ * mBlockCount_);
//         if(!pool_)
//         {
//             throw std::bad_alloc();
//         }
//         for(size_t i = 0; i < blockCount; ++i)
//         {
//             auto blockPtr = static_cast<char*>(pool_) + i * mBlockSize_;
//             mBlocks_.emplace_back(new Block(blockPtr)); // 使用 raw pointer
//             mFreeBlocks_.push_back(mBlocks_.back().get()); // 使用原始指针直接存储
//         }
//     }

//     ~MemoryPool()
//     {
//         std::free(pool_);
//     }

//     T* allocate()
//     {
//         std::lock_guard<std::mutex> lock(mtx_);
//         if(mFreeBlocks_.empty())
//         {
//             return nullptr; // 无可用块
//         }
//         Block* block = mFreeBlocks_.back();
//         mFreeBlocks_.pop_back();
//         block->setUse(true);
//         return static_cast<T*>(block->getPtr());
//     }

//     void deallocate(T* ptr)
//     {
//         std::lock_guard<std::mutex> lock(mtx_);
//         for(auto& block : mBlocks_)
//         {
//             if(block->getPtr() == ptr && block->isInUsed())
//             {
//                 block->setUse(false);
//                 mFreeBlocks_.push_back(block.get()); // 存储原始指针
//                 return;
//             }
//         }
//     }

// private:
//     size_t mBlockSize_;
//     size_t mBlockCount_;
//     void* pool_;
//     std::vector<std::unique_ptr<Block>> mBlocks_;
//     std::vector<Block*> mFreeBlocks_; // 改为存储原始指针
//     std::mutex mtx_;
// };


// }