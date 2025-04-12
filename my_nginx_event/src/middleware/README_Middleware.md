# Middleware 使用说明

本模块基于 **责任链模式** 实现了可插拔的中间件系统，可用于在请求前/响应后注入通用逻辑（如鉴权、日志、限流等）。

## 📦 模块结构

```
Middleware (抽象接口)
 ├── before(HttpRequest& req)     // 请求前处理
 └── after(HttpResponse& res)     // 响应后处理

MiddlewareChain
 ├── addMiddleware(std::shared_ptr<Middleware>) // 添加中间件
 ├── processBefore(HttpRequest&)                // 顺序调用 before()
 └── processAfter(HttpResponse&)                // 逆序调用 after()
```

## 🚀 使用方式

```cpp
MiddlewareChain chain;
chain.addMiddleware(std::make_shared<LoggingMiddleware>());
chain.addMiddleware(std::make_shared<AuthMiddleware>(validator));

try {
    chain.processBefore(request);   // 所有 before 执行
    // 处理业务逻辑...
    chain.processAfter(response);   // 所有 after 逆序执行
} catch (const HttpException& ex) {
    response.setStatusCode(ex.code());
    response.setBody(ex.what());
}
```

## ✅ 适用场景

- 鉴权认证（如 JWT、Token）
- 请求日志记录
- 请求限流 / 黑名单判断
- 跨域处理（CORS）
- 请求耗时统计 / 性能分析
- 全局异常处理
