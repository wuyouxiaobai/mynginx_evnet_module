#include <atomic>
#include <memory>
#include <utility>
#include <vector>
#include "my_socket.h"



namespace WYXB{

template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::pair<STRUC_MSG_HEADER, std::vector<uint8_t>> data;
        std::atomic<Node*> next;

        Node() : next(nullptr) {}
        explicit Node(const T& msg) : data(msg), next(nullptr) {}
    };

    // 带标签的指针（防止ABA问题）
    struct TaggedPtr {
        Node* ptr;
        uint64_t tag;
    };

    alignas(64) std::atomic<TaggedPtr> head_;  // 头指针（缓存行对齐）
    alignas(64) std::atomic<TaggedPtr> tail_;   // 尾指针

public:
    LockFreeQueue() {
        Node* dummy = new Node();
        head_.store({dummy, 0}, std::memory_order_relaxed);
        tail_.store({dummy, 0}, std::memory_order_relaxed);
    }

    ~LockFreeQueue() {
        while (auto old_head = head_.load(std::memory_order_relaxed).ptr) {
            head_.store({old_head->next.load(std::memory_order_relaxed), 0}, std::memory_order_relaxed);
            delete old_head;
        }
    }

    // 入队操作（多线程安全）
    void Enqueue(const T& msg) {
        Node* new_node = new Node(msg);
        TaggedPtr old_tail, new_tail;

        while (true) {
            old_tail = tail_.load(std::memory_order_acquire);
            Node* old_tail_ptr = old_tail.ptr;
            Node* next = old_tail_ptr->next.load(std::memory_order_acquire);

            // 检查尾指针是否被其他线程更新
            if (old_tail.ptr != tail_.load(std::memory_order_acquire).ptr)
                continue;

            if (next == nullptr) {  // 尾节点未被修改
                new_tail.ptr = new_node;
                new_tail.tag = old_tail.tag + 1;

                // CAS更新next指针
                if (old_tail_ptr->next.compare_exchange_weak(
                        next, new_node, 
                        std::memory_order_release,
                        std::memory_order_relaxed)) 
                {
                    break;
                }
            } else {  // 帮助其他线程推进尾指针
                TaggedPtr updated_tail{next, old_tail.tag + 1};
                tail_.compare_exchange_weak(old_tail, updated_tail,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            }
        }

        // 尝试更新尾指针
        TaggedPtr updated_tail{new_node, old_tail.tag + 1};
        tail_.compare_exchange_strong(old_tail, updated_tail,
            std::memory_order_release,
            std::memory_order_relaxed);
    }

    // 出队操作（线程安全）
    bool TryDequeue(T& msg) {
        TaggedPtr old_head, new_head;
        Node* old_tail;

        while (true) {
            old_head = head_.load(std::memory_order_acquire);
            old_tail = tail_.load(std::memory_order_acquire).ptr;
            Node* next = old_head.ptr->next.load(std::memory_order_acquire);

            if (old_head.ptr != head_.load(std::memory_order_acquire).ptr)
                continue;

            if (old_head.ptr == old_tail) {  // 空队列
                if (next == nullptr) return false;
                // 推进尾指针
                TaggedPtr updated_tail{next, old_head.tag + 1};
                tail_.compare_exchange_strong(
                    {old_tail, 0}, updated_tail,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            } else {
                msg = next->data;
                new_head.ptr = next;
                new_head.tag = old_head.tag + 1;

                // CAS更新头指针
                if (head_.compare_exchange_weak(old_head, new_head,
                        std::memory_order_release,
                        std::memory_order_relaxed)) 
                {
                    delete old_head.ptr;  // 安全回收旧头节点
                    return true;
                }
            }
        }
    }
};

}