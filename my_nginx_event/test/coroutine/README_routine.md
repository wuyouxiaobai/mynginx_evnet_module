# 协程化线程池改造方案 (基于 libgo)

## 1. 背景

在原有架构中，我们使用 `epoll` 等多路复用机制来处理网络连接事件，并将收到的数据或事件封装为**任务**放进**无锁队列**里，然后由**线程池**不断从队列中取出任务进行处理。这种方式虽然能够支持并发，但线程池内大量线程在处理 IO 密集任务时，可能导致线程上下文切换过多。

为提升并发处理能力，我们可将**线程池**改造成**协程**模型。通过 **libgo** 提供的用户态协程调度，在少量线程里即可创建大量轻量协程，大幅减少系统线程切换开销。


## 2. 总体思路

1. **保留 `epoll`**：原先使用 epoll 等多路复用手段来获取可读可写事件，逻辑不变。
2. **保留无锁队列**：当有可读事件时，网络层将数据封装为任务（`MyTask`）并 `Push()` 到无锁队列中，避免竞争锁开销。
3. **替换线程池为 libgo 协程**：
   - 原先是若干线程（如 10、20 个）死循环 `Pop` 任务进行处理；
   - 现在改为少量物理线程（通常与 CPU 核数相当），每个线程内跑多个协程，不断从队列获取任务并处理。
4. **统一调度接口**：
   - 使用虚基类、函数指针或 `std::function` 映射来根据 `taskType` 调用不同的处理逻辑（如 `HandlerA`, `HandlerB`）。

通过这四步，即可实现“**epoll + 无锁队列** + 协程(替换原先线程池)”的高效并发模型。


## 3. 核心要点

### 3.1 协程消费者逻辑

在 libgo 中，可以用 `go` 宏启动一个协程。假设我们有一个消费者函数：

```cpp
void consumerCoroutine()
{
    while (true) {
        MyTask task;
        // 当队列无数据时，TryPop() 返回 false
        if (!g_taskQueue.TryPop(task)) {
            // 协程空转则让出CPU片刻
            co_sleep(1);
            continue;
        }

        // 根据 taskType 或其他信息调用通用处理逻辑
        auto it = g_handlerMap.find(task.taskType);
        if (it != g_handlerMap.end()) {
            it->second->handle(task.payload);
        } else {
            // 未知任务类型
        }
    }
}
```

- **无锁队列**：`TryPop()` 如果队列为空会立即返回 `false`。
- **协程让出**：`co_sleep(1)` 或 `co_yield()` 均可防止空转，腾出 CPU 给其它协程。


### 3.2 生产者逻辑

在你的真实业务中，生产者逻辑往往在**epoll 监听线程**或别的 IO 线程里：

1. `epoll_wait` 检测某 socket 可读
2. 读取数据，封装成 `MyTask` 对象
3. `g_taskQueue.Push(task)`

协程消费者随时能从队列中读取这批任务。


### 3.3 通用接口

我们可以用“虚基类 + 派生类”或“`std::function` 映射”来根据 `taskType` 调用不同函数。例如：

```cpp
struct ITaskHandler {
    virtual ~ITaskHandler() = default;
    virtual void handle(const std::vector<uint8_t>& data) = 0;
};

struct HandlerA : public ITaskHandler {
    void handle(const std::vector<uint8_t>& data) override {
        // 处理逻辑 A
    }
};

struct HandlerB : public ITaskHandler {
    void handle(const std::vector<uint8_t>& data) override {
        // 处理逻辑 B
    }
};

std::unordered_map<int, std::shared_ptr<ITaskHandler>> g_handlerMap;
```

这样，协程消费者取到 `MyTask` 后，根据 `taskType` 查找相应的处理器即可。


### 3.4 调度与线程

libgo 提供**协程调度器**来管理所有协程。一般做法：

1. 启动少量（`N` 个）物理线程（`std::thread`），数量通常与 CPU 核数一致
2. 每个线程里都执行 `co_sched.RunUntilNoTask()`，让调度器在该线程上运行并调度所有协程
3. 对于每个消费者或生产者，都用 `go` 宏在调度器里创建协程执行

示例（简化版）：

```cpp
int main()
{
    // 注册处理器到 g_handlerMap, 初始化队列等...

    // 启动若干消费协程
    for (int i = 0; i < 8; i++) {
        go consumerCoroutine;
    }

    // 启动几个工作线程跑调度器
    for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
        std::thread([] {
            co_sched.RunUntilNoTask();
        }).detach();
    }

    // 主线程也可参与调度
    co_sched.RunUntilNoTask();
    return 0;
}
```

这样就实现了 “少量线程 + 多协程” 的模式。每个协程负责不断 `Pop` 队列、调用对应的处理器来处理业务逻辑。


## 4. 与原生C++20协程对比

- 如果想使用 C++20 自带的 `co_await` 语法，需要自行写 `asyncPop()` 的 Awaiter 或借助类似 `cppcoro` 库封装。
- 相比之下，libgo 提供了更“直接”的接口和 `go` 宏，业务上手比较容易，升级成本更低。


## 5. 小结

- **只修改线程池层**：用 libgo 协程替代原先的多线程循环取任务的方式；
- **保留 epoll + 无锁队列**：收集 IO 事件不变，仅在有新消息时 `Push` 到队列；
- **通用接口**：通过 `taskType` 映射，灵活调用不同的业务逻辑函数；
- **收益**：大幅减少线程切换开销，同时保留原先的 epoll 事件模型，代码结构清晰易维护。


---

**至此，即完成了“协程化线程池改造方案”在团队内部的初步说明。如有更多性能调优、异常处理或单测部署的需求，可在此文档基础上继续拓展。**