# Session 会话管理模块说明

本模块提供基于 Cookie 的会话管理系统，支持会话的创建、刷新、过期检查与存储，可用于用户登录状态保持、会话级数据共享等场景。

---

## 📦 模块结构概览

```
Session             // 单个会话对象，管理数据与过期时间
SessionManager      // 管理所有会话的生命周期和状态
SessionStorage      // 抽象接口，支持不同存储实现
 └── MemorySessionStorage // 内存版本的存储实现
```

---

## 🧠 使用原理

1. 请求到达后，`SessionManager` 从 Cookie 中获取 `sessionId`
2. 若 `sessionId` 存在且有效，加载原有会话；否则创建新会话
3. 会话通过 `Session` 对象管理数据与过期时间
4. 所有会话存储在 `SessionStorage` 中（默认使用内存存储）
5. 响应阶段将 `Set-Cookie` 写入客户端

---

## 🛠 使用示例

✅ 1️⃣ 登录时写入 Session
在登录校验通过后（用户名/密码匹配）：
```cpp
void handleLogin(HttpRequest& req, HttpResponse& resp, SessionManager& sessionManager) {
    std::string username = req.getParam("username");
    std::string password = req.getParam("password");

    if (checkPassword(username, password)) {  // 登录成功
        auto session = sessionManager.getSession(req, &resp);
        session->setValue("username", username);
        session->setValue("userId", "12345"); // 或数据库查到的真实用户ID

        resp.setStatusCode(200);
        resp.setBody("Login success!");
    } else {
        resp.setStatusCode(401);
        resp.setBody("Invalid credentials");
    }
}

```
✅ 2️⃣ 后续请求中读取 session 验证登录状态
```cpp
void handleProfile(HttpRequest& req, HttpResponse& resp, SessionManager& sessionManager) {
    auto session = sessionManager.getSession(req, &resp);
    std::string username = session->getValue("username");

    if (username.empty()) {
        resp.setStatusCode(401);
        resp.setBody("Please log in first");
        return;
    }

    resp.setBody("Hello, " + username + "! Here's your profile.");
}
```
✅ 3️⃣ 登出接口（清除 session）
```cpp
void handleLogout(HttpRequest& req, HttpResponse& resp, SessionManager& sessionManager) {
    std::string sessionId = sessionManager.getSessionIdFromCookie(req);
    sessionManager.destroySession(sessionId);

    // 清除客户端 Cookie
    resp.addHeader("Set-Cookie", "sessionId=; Path=/; Max-Age=0; HttpOnly");
    resp.setBody("Logged out.");
}
```
---

## 📌 接口说明

### Session

- `setValue(key, val)`：设置会话内数据
- `getValue(key)`：获取数据
- `remove(key)`：删除某项数据
- `clear()`：清空全部数据
- `refresh()`：刷新过期时间
- `isExpired()`：判断是否过期

### SessionManager

- `getSession(req, resp)`：获取或创建 Session
- `generateSessionId()`：生成随机会话ID
- `destroySession(id)`：销毁会话
- `setSessionCookie(id, resp)`：设置 Cookie

### MemorySessionStorage

- `save(session)`：保存会话
- `load(id)`：加载会话
- `remove(id)`：删除会话

---

## ✅ 特性总结

| 特性         | 描述 |
|--------------|------|
| 自动刷新     | 每次访问自动延期有效期 |
| 安全性       | 会话 ID 随机 + HttpOnly |
| 抽象存储接口 | 支持替换为 Redis/File 存储 |
| 轻量高效     | 默认使用内存 map 存储 |

---

## 🧩 可扩展方向

- 添加基于 Redis 的 SessionStorage
- 支持 Secure / SameSite Cookie 限制
- Session 数据支持泛型或 json 对象
- 添加会话清理任务（定期清理过期 session）

---

## 🔐 注意事项

- 当前 `MemorySessionStorage` **非线程安全**，多线程环境需加锁
- 如果部署在分布式环境，建议使用集中式存储（如 Redis）

