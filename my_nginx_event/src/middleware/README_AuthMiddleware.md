# AuthMiddleware 中间件说明

`AuthMiddleware` 是一个用于处理请求鉴权的中间件，支持对 Authorization 请求头中的 Token 进行格式校验和自定义验证。

## 🧩 功能描述

- 拦截请求前阶段，提取 `Authorization` 头部
- 检查是否为合法 Bearer 格式
- 调用外部注入的 Token 校验函数（如比较、JWT 校验等）
- 校验失败时抛出 `HttpException`

## ✨ 使用方式

```cpp
auto validator = [](const std::string& token) {
    return token == "Bearer 123456";
};

auto auth = std::make_shared<AuthMiddleware>(validator);
middlewareChain.addMiddleware(auth);
```

## 📦 接口说明

```cpp
class AuthMiddleware : public Middleware {
public:
    using TokenValidator = std::function<bool(const std::string&)>;
    AuthMiddleware(TokenValidator validator);

    void before(HttpRequest& req) override;
    void after(HttpResponse& res) override;
};
```

## ⚠️ 异常处理

- 若缺失或格式错误：抛出 `HttpException(401, "...")`
- 若验证失败：抛出 `HttpException(403, "...")`

调用方需在主业务逻辑外 `try-catch` 捕获异常并构造错误响应。

## 🔒 可扩展验证方式

- 固定 token 校验（适用于调试或嵌入式）
- 正则验证格式
- JWT 解码与签名验证（结合 `jwt-cpp` 等库）
- 与数据库或 Redis 结合查询用户权限
