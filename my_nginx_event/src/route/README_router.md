# 🚦 Router 模块（C++）

本模块用于实现 HTTP 请求的路由分发逻辑，支持静态路由与动态路由（正则表达式），可按请求方法（GET/POST/...）与路径将请求分发到对应处理器或回调函数中。

---

## 📌 功能简介

- 支持 `Handler` 类注册，用于封装业务处理逻辑
- 支持 `Callback` 函数注册，快速处理简单请求
- 支持带路径参数的正则动态路由匹配
- 支持多种 HTTP 方法独立注册（GET、POST...）
- 匹配优先级从静态路径到动态路径，性能优先

---

## 🧠 路由匹配流程

```text
1. 精准静态路由 Handler 查找
2. 精准静态路由 Callback 查找
3. 动态正则路由 Handler 匹配
4. 动态正则路由 Callback 匹配
5. 匹配失败返回 false（调用方可设置 404）
```

---

## 💠 注册与使用示例

### 1. 注册静态 Handler

```cpp
router.registerHandler(HttpRequest::GET, "/ping", std::make_shared<PingHandler>());
```

### 2. 注册静态 Callback

```cpp
router.registerCallback(HttpRequest::POST, "/login", [](const HttpRequest& req, HttpResponse* resp) {
    // 登录处理逻辑
});
```

### 3. 注册正则 Handler

```cpp
router.registerRegexHandler(HttpRequest::GET, std::regex("^/user/\\d+$"), std::make_shared<UserDetailHandler>());
```

### 4. 注册正则 Callback（带参数）

```cpp
router.registerRegexCallback(HttpRequest::GET, std::regex("^/video/(\\d+)$"),
[](const HttpRequest& req, HttpResponse* resp) {
    std::string videoId = req.getPathParam(0); // 提取正则 group
    // 处理逻辑
});
```

---

## 🔍 模块接口说明

### 路由注册方法

```cpp
void registerHandler(HttpRequest::Method method, const std::string& path, HandlerPtr handler);
void registerCallback(HttpRequest::Method method, const std::string& path, const HandlerCallback& callback);
void registerRegexHandler(HttpRequest::Method method, const std::regex& regex, HandlerPtr handler);
void registerRegexCallback(HttpRequest::Method method, const std::regex& regex, const HandlerCallback& callback);
```

### 路由执行方法

```cpp
bool route(const HttpRequest& req, HttpResponse* resp);
```

- 匹配成功则调用对应逻辑，返回 `true`
- 匹配失败返回 `false`，可用于返回 404 页面

---

## 🧰 内部结构设计

| 成员变量           | 类型                                 | 说明                     |
|--------------------|--------------------------------------|--------------------------|
| `handlers_`        | `std::unordered_map<RouteKey, HandlerPtr>` | 静态路径 → Handler 映射 |
| `callbacks_`       | `std::unordered_map<RouteKey, Callback>`   | 静态路径 → Callback 映射|
| `regexHandlers_`   | `std::vector<std::tuple<Method, regex, HandlerPtr>>` | 正则路径 Handler 列表 |
| `regexCallbacks_`  | `std::vector<std::tuple<Method, regex, Callback>>`   | 正则路径 Callback 列表 |

---

## 👍 提示

- 动态路由匹配成功时，会删除正则组并追加到 `HttpRequest`
- 如需定制本地无匹配路由处理，可在 `route()` 返回 false 时调用 404 解析器

---

若需增加单测指南、性能测试或实际项目绑定示例，可继续扩展本文档。

