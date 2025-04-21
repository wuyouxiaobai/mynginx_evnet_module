#pragma once
#include <atomic>
#include <memory>
#include <utility>
#include <vector>

namespace WYXB {

template<typename T>
class LockFreeQueue {
private:
    struct Node {
        T data;
        std::atomic<Node*> next;

        Node() : next(nullptr) {}
        explicit Node(const T& msg) : data(msg), next(nullptr) {}
    };

    alignas(64) std::atomic<Node*> head_ptr_;
    alignas(64) std::atomic<uint64_t> head_tag_;

    alignas(64) std::atomic<Node*> tail_ptr_;
    alignas(64) std::atomic<uint64_t> tail_tag_;

public:
    std::atomic<int> size{0};

    LockFreeQueue() {
        Node* dummy = new Node();
        head_ptr_.store(dummy, std::memory_order_relaxed);
        tail_ptr_.store(dummy, std::memory_order_relaxed);
        head_tag_.store(0, std::memory_order_relaxed);
        tail_tag_.store(0, std::memory_order_relaxed);
    }

    ~LockFreeQueue() {
        while (auto old_head = head_ptr_.load(std::memory_order_relaxed)) {
            head_ptr_.store(old_head->next.load(std::memory_order_relaxed), std::memory_order_relaxed);
            delete old_head;
        }
    }

    void Enqueue(const T& msg) {
        Node* new_node = new Node(msg);

        while (true) {
            Node* old_tail_ptr = tail_ptr_.load(std::memory_order_acquire);
            uint64_t old_tail_tag = tail_tag_.load(std::memory_order_acquire);

            Node* next = old_tail_ptr->next.load(std::memory_order_acquire);

            if (old_tail_ptr != tail_ptr_.load(std::memory_order_acquire))
                continue;

            if (next == nullptr) {
                if (old_tail_ptr->next.compare_exchange_weak(
                        next, new_node,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {
                    size.fetch_add(1, std::memory_order_relaxed);
                    tail_ptr_.store(new_node, std::memory_order_release);
                    tail_tag_.store(old_tail_tag + 1, std::memory_order_release);
                    break;
                }
            } else {
                tail_ptr_.compare_exchange_weak(
                    old_tail_ptr, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
                tail_tag_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    bool TryDequeue(T& msg) {
        while (true) {
            Node* old_head_ptr = head_ptr_.load(std::memory_order_acquire);
            uint64_t old_head_tag = head_tag_.load(std::memory_order_acquire);

            Node* old_tail_ptr = tail_ptr_.load(std::memory_order_acquire);
            Node* next = old_head_ptr->next.load(std::memory_order_acquire);

            if (old_head_ptr != head_ptr_.load(std::memory_order_acquire))
                continue;

            if (old_head_ptr == old_tail_ptr) {
                if (next == nullptr) return false;
                tail_ptr_.compare_exchange_strong(old_tail_ptr, next,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed);
                tail_tag_.fetch_add(1, std::memory_order_relaxed);
            } else {
                msg = next->data;
                if (head_ptr_.compare_exchange_weak(old_head_ptr, next,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed)) {
                    head_tag_.store(old_head_tag + 1, std::memory_order_release);
                    delete old_head_ptr;
                    size.fetch_sub(1, std::memory_order_relaxed);
                    return true;
                }
            }
        }
    }
};

} // namespace WYXB