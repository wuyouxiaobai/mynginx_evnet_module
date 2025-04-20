# CustomHttp性能测试与瓶颈分析指南


---

## ✨ 性能测试（Performance Testing）

### 1. 使用 `wrk` 进行压测

```bash
wrk -t8 -c100 -d30s http://127.0.0.1:8080/hello/world
```

参数说明：
- `-t8`：8 个线程并发发起请求
- `-c100`：100 个连接（并发连接数）
- `-d30s`：压测持续 30 秒

输出包含：
- QPS（每秒请求数）
- 延迟分布（平均 / P95 / P99）
- 吞吐量（每秒传输的数据）

### 2. 其他压测工具推荐

| 工具名 | 功能说明 |
|--------|----------|
| `ab` (ApacheBench) | 简单的单线程压测工具 |
| `hey`              | 更现代化，支持并发的压测工具（Go 实现） |
| `curl`             | 适合功能性接口测试和手动脚本模拟 |

---

## 🔧 性能瓶颈定位工具

### 1. 使用 `perf` 进行采样分析（推荐）

#### 执行记录并生成报告：
```bash
perf record -g ./httpserver
perf report
```

#### 实时查看热点函数：
```bash
perf top
```

### 2. 使用 Flamegraph（火焰图）可视化热点函数

```bash
perf record -F 99 -g ./your_http_server
perf script > out.perf
stackcollapse-perf.pl out.perf > out.folded
flamegraph.pl out.folded > flame.svg
```

生成的 `flame.svg` 可用浏览器查看，直观看到哪些函数最耗时。

### 3. 使用 `gprof` 分析函数调用时间（适用于单线程调试）

```bash
g++ -pg main.cpp -o your_http_server
./your_http_server
```
停止程序后：
```bash
gprof ./your_http_server gmon.out > report.txt
```

### 4. 使用 `valgrind + callgrind` 生成函数调用图

```bash
valgrind --tool=callgrind ./your_http_server
kcachegrind callgrind.out.*
```

用于查看函数之间调用关系、每个函数耗时。

---

## 📊 建议关注的性能指标

| 指标           | 含义说明 |
|----------------|----------|
| QPS / RPS      | 每秒请求/查询数 |
| 延迟（P50/P95）| 中位延迟、95分位延迟 |
| CPU 利用率     | 是否存在热点线程或函数 |
| 上下文切换次数 | 是否存在频繁锁竞争 |
| 内存使用        | 是否存在泄露或持续增长 |

---

## ⚡ 常见瓶颈与优化建议

| 模块         | 潜在问题             | 优化建议                     |
|--------------|----------------------|------------------------------|
| 路由查找     | 每次遍历匹配慢       | 改用 Trie 树或 Hash 匹配表   |
| 中间件执行   | 存在阻塞操作         | 改为异步协程或限流中间件     |
| 线程池调度   | 队列阻塞/线程不足     | 增加线程数，使用 lock-free 队列 |
| Socket 层     | epoll 重复触发慢      | 使用 ET 模式 + 批量处理事件   |
| 日志系统     | 同步写盘耗时         | 使用异步日志系统（如 spdlog） |
| 内存管理     | 每请求 new/delete     | 使用对象池 / arena 分配器     |

---

## 🚀 高级建议（可选）

- 使用 `gperftools::ProfilerStart/Stop` 实现运行时性能采样
- 集成 `Prometheus` 输出实时性能指标（如 QPS / 队列长度）
- 使用 `strace` 或 `ltrace` 追踪系统调用或库调用行为
- 使用 `SystemTap` 或 eBPF 进行内核级分析

---

## 📅 推荐测试流程

1. 使用 `wrk` 测试基础吞吐能力 ✅
2. 使用 `perf` 分析热点函数 ✅
3. 使用 `valgrind + callgrind` 查看函数调用关系 ✅
4. 使用 `flamegraph` 生成可视化图谱 ✅
5. 引入内存池、协程、batch 化优化处理路径 ✅

---

如果你希望我为此框架生成自动化测试脚本（如 wrk + Lua 压测）、Prometheus 集成指标导出、火焰图一键脚本等，可以继续告诉我 ✅

