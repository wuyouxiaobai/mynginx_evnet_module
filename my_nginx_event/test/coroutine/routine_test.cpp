#include <iostream>
#include <thread>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

#include "libgo/coroutine.h" // 引入libgo头文件
#include "lockfree_queue.h"  // 你自己的无锁队列声明

// ------------------(1) 定义任务结构 ------------------
struct MyTask {
    int taskType;                 // 不同任务类型
    std::vector<uint8_t> payload; // 任务数据
};

// ------------------(2) 通用接口：根据type调用不同实现 ------------------
class ITaskHandler {
public:
    virtual ~ITaskHandler() = default;
    virtual void handle(const std::vector<uint8_t>& data) = 0;
};

// 示例：HandlerA
class HandlerA : public ITaskHandler {
public:
    void handle(const std::vector<uint8_t>& data) override {
        std::cout << "[HandlerA] process data size=" << data.size() << std::endl;
        // ... 做任何需要的处理
    }
};

// 示例：HandlerB
class HandlerB : public ITaskHandler {
public:
    void handle(const std::vector<uint8_t>& data) override {
        std::cout << "[HandlerB] process data size=" << data.size() << std::endl;
        // ... 做任何需要的处理
    }
};

// 用一个map存放 type -> Handler对象
std::unordered_map<int, std::shared_ptr<ITaskHandler>> g_handlerMap;

// ------------------(3) 全局无锁队列 ------------------
LockFreeQueue<MyTask> g_taskQueue;

// ------------------(4) 协程消费者函数 ------------------
void consumerCoroutine()
{
    while (true) {
        // （A）从队列中取任务
        MyTask task;
        if (!g_taskQueue.TryPop(task)) {
            // 没取到任务，挂起当前协程片刻，避免空转
            co_sleep(1); 
            continue;
        }

        // （B）调用统一接口
        auto iter = g_handlerMap.find(task.taskType);
        if (iter != g_handlerMap.end()) {
            iter->second->handle(task.payload);
        } else {
            std::cout << "[Warning] No handler found for taskType=" 
                      << task.taskType << std::endl;
        }
    }
}

// ------------------(5) 生产者线程 - epoll + push 任务示例 ------------------
void producerThread()
{
    // 这里仅示例如何往队列里推送任务
    // 实际情况是：epoll检测到可读后，你会构造MyTask并push进来
    for (int i = 0; i < 10; ++i) {
        MyTask t;
        t.taskType = (i % 2 == 0) ? 1 : 2;
        t.payload = {0x01, 0x02, 0x03, (uint8_t)i};
        g_taskQueue.Push(t);

        // 模拟阻塞或其他操作
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main()
{
    // (A) 初始化 handlerMap
    g_handlerMap[1] = std::make_shared<HandlerA>();
    g_handlerMap[2] = std::make_shared<HandlerB>();

    // (B) 启动若干“消费者协程”
    int coroutineCount = 4; // 你可根据实际情况设置
    for (int i = 0; i < coroutineCount; ++i) {
        go consumerCoroutine;  
    }

    // (C) 启动生产者线程（仅示例，也可放你epoll读事件线程）
    std::thread tProducer(producerThread);

    // (D) 启动libgo调度器的工作线程（通常与CPU核数相当）
    int threadCount = std::thread::hardware_concurrency();
    for (int i = 0; i < threadCount; ++i) {
        std::thread([]{
            // 每个工作线程跑调度器循环
            co_sched.RunUntilNoTask();
        }).detach();
    }

    // (E) 主线程也可参与调度
    co_sched.RunUntilNoTask();

    tProducer.join();
    return 0;
}
