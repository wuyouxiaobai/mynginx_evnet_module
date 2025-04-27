---

# 📚 插拔式 HTTP 限流 Filter 组件使用说明

本组件为自定义 HTTP 框架提供 **插拔式限流功能**，支持令牌桶限流、漏斗限流等算法，独立于业务逻辑，具备以下特点：
- 插拔式注册，不入侵 Controller
- 支持多种限流算法
- 支持自定义限流粒度（如 IP、用户ID）
- 高并发安全（锁保护 + 高效实现）

---

## 🛠️ 使用步骤

### 1. 创建 FilterManager 实例
在服务器启动 (`Server::run()`) 初始化阶段，创建全局唯一的 `FilterManager`：

```cpp
m_filterManager = std::make_shared<FilterManager>();
```

---

### 2. 注册限流 Filter
将限流规则以组件形式注册进 `FilterManager`。

示例（注册一个令牌桶限流器）：

```cpp
m_filterManager->addFilter(std::make_shared<RateLimiterFilter>(5 /*rate*/, 10 /*burst*/));
```

含义：
- `rate = 5` ：平均每秒允许 5 个请求通过
- `burst = 10`：允许短时间最多积累 10 个令牌（即瞬时爆发最多10个请求）

可注册多个不同类型的 Filter，实现更复杂的链式拦截。

---

### 3. 在 HTTP 请求入口统一执行 Filter 链
在收到 HTTP 请求时，执行所有 Filter。  
如果任何一个 Filter 拦截请求，立即返回错误响应；如果全部通过，继续正常路由处理。

示例：

```cpp
onHttpRequest = [this](const HttpRequestPtr& req) {
    m_filterManager->doFilters(req,
        [](HttpResponsePtr resp) {
            // Filter拦截，直接返回响应
            sendHttpResponse(resp);
        },
        [req]() {
            // 所有Filter通过，正常分发请求
            dispatchRequest(req);
        }
    );
};
```

---

## 📄 完整代码示例

```cpp
void Server::run() {
    // 1. 创建 FilterManager
    m_filterManager = std::make_shared<FilterManager>();

    // 2. 注册 RateLimiterFilter (令牌桶限流)
    m_filterManager->addFilter(std::make_shared<RateLimiterFilter>(5, 10));

    // 3. 在收到 HTTP 请求时统一执行 Filter 链
    onHttpRequest = [this](const HttpRequestPtr& req) {
        m_filterManager->doFilters(req,
            [](HttpResponsePtr resp) {
                sendHttpResponse(resp); // 被限流直接返回
            },
            [req]() {
                dispatchRequest(req);   // 正常处理业务
            }
        );
    };
}
```

---

## 🚦 Filter处理流程示意图

```
Http请求进入
    ↓
FilterManager::doFilters()
    ↓
顺序执行每个Filter
    ↓
    ├── 若某个Filter拒绝 → 立即返回错误响应
    └── 所有Filter通过 → 正常dispatch到业务Handler
```

---

## 📈 后续扩展建议

- 支持按不同 **IP / 用户ID / API路径** 单独限流
- 支持限流配置 **热更新**
- 支持不同限流算法（如漏斗限流、滑动窗口限流）
- 支持根据请求Header / Token灵活选择限流维度
- 通过策略模式 (`IRateLimiter`) 支持动态切换限流策略

---

# ✅ 小结

| 特性            | 说明                                      |
|:----------------|:------------------------------------------|
| 插拔式          | 随时添加或移除Filter |
| 高并发          | 全程线程安全，支持高并发访问 |
| 灵活扩展        | 支持多种限流策略切换 |
| 无侵入          | 不影响现有 Controller 层代码逻辑 |

---
