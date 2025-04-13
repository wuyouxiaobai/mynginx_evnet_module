# Redis Session 会话存储使用说明

本模块基于 Redis 实现会话存储，替代默认的内存存储方式，适用于分布式部署、跨进程共享 Session 的场景。

---

## ✅ 使用步骤

### 1️⃣ 引入相关头文件

```cpp
#include "RedisSessionStorage.h"
#include "redisClient.h"
#include "sessionManager.h"
```

### 2️⃣ 初始化 SessionManager（使用 Redis 存储）

```cpp
auto redisClient = std::make_shared<RedisClient>("127.0.0.1", 6379);
auto redisStorage = std::make_unique<RedisSessionStorage>(redisClient);
SessionManager sessionManager(std::move(redisStorage));
```

### 3️⃣ 请求处理流程中使用 Session

```cpp
void handleRequest(HttpRequest& req, HttpResponse& resp) {
    auto session = sessionManager.getSession(req, &resp);

    session->setValue("userId", "12345");
    session->setValue("username", "xiaoming");

    std::string name = session->getValue("username");
    resp.setBody("Hello, " + name);
}
```

---

## 🧠 工作原理

- Redis key 结构：`session:{sessionId}`
- 值为 JSON 格式序列化内容，包含 `data` 和 `maxAge`
- 会话自动设置过期时间（TTL）以支持滑动过期
- 使用 `Cookie` 在客户端存储 sessionId，实现会话识别

---

## 🧩 Redis 中存储结构示例

```json
{
  "sessionId": "abc123...",
  "data": {
    "userId": "12345",
    "username": "xiaoming"
  },
  "maxAge": 1800
}
```

---

## ✅ 优势特性

| 特性           | 描述                                 |
|----------------|--------------------------------------|
| 分布式支持     | Redis 支持集群部署，实现多服务共享 Session |
| 自动过期       | 使用 `SETEX` 设置 TTL，防止数据堆积     |
| 安全隔离       | 每个会话独立 key，避免串数据            |
| 结构清晰       | JSON 可扩展，便于调试与监控              |

---

## ⚠️ 注意事项

- Redis 必须保持连接稳定，建议使用连接池或断线重连机制
- 当前实现依赖 `rapidjson` 进行序列化与解析
- Session TTL 建议设置为合理值（如 1800 秒）

---

## 🔧 可拓展方向

- 支持压缩或加密 session 数据
- 支持 JWT + Redis 混合验证方案
- 接入 Redis 连接池或 Redis Sentinel 高可用

