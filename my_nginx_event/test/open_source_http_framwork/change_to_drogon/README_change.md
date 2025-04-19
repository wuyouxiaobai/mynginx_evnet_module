# WYXB Server（Drogon 风格改进）

本项目是对原有 C++ 服务端框架的重构与升级，借鉴了 [Drogon](https://github.com/drogonframework/drogon) 的现代 C++ 风格。整体框架保留了原有的线程池、epoll、多进程和信号管理等底层能力，同时在上层接口设计上向 Drogon 靠拢，提供更优雅、更易用的开发体验。

---

## ✨ 改进亮点（对齐 Drogon 风格）

### ✅ 1. 启动入口简洁化

原来：
```cpp
Server::instance().start(argc, argv);
```

现在：
```cpp
Server::instance().run(argc, argv);
```

并支持链式路由、中间件注册。

---

### ✅ 2. 路由注册 DSL 化

支持如下链式写法（风格接近 Drogon）：
```cpp
Server::instance()
    .get("/api/hello", handler1)
    .post("/api/data", handler2)
    .run(argc, argv);
```

也支持通用注册：
```cpp
.on(HttpRequest::Method::kPut, "/api/xx", handler)
```

---

### ✅ 3. 中间件注册链式化

```cpp
Server::instance()
    .addMiddleware(std::make_shared<LoggerMiddleware>())
    .addMiddleware(std::make_shared<AuthMiddleware>());
```

---

### ✅ 4. 支持注册 Controller 模块（可拓展）

```cpp
template<typename ControllerType>
void registerController() {
    ControllerType ctl;
    ctl.registerRoutes(*this);
}
```

---

### ✅ 5. 中间件执行链（建议拓展）

预留了中间件链式 before/after 调用流程，建议在 `handleRequest()` 中执行：
```cpp
for (auto& mw : middlewareList)
    if (!mw->beforeHandle(req, res)) return;

// 调用处理器 handler

for (auto it = middlewareList.rbegin(); it != middlewareList.rend(); ++it)
    (*it)->afterHandle(req, res);
```

---

## 📁 示例用法（对齐 Drogon）

```cpp
#include "server.h"

int main(int argc, char* argv[]) {
    using namespace WYXB;

    Server::instance()
        .addMiddleware(std::make_shared<LoggerMiddleware>())
        .get("/hello", [](const HttpRequest& req, HttpResponse& res) {
            res.setBody("Hello Drogon-style!");
        })
        .run(argc, argv);
}
```

---

## 🧩 后续可拓展方向

- 支持 `Controller` 类自动注册（参考 Drogon）
- 请求参数自动绑定 JSON → struct
- 异步 handler 返回 future/promise/coroutine
- 路由支持通配符、RESTful、正则
- WebSocket、定时器、静态资源模块解耦

---

如需进一步协助实现完整的 controller 注册或参数绑定机制，请联系开发维护者。
