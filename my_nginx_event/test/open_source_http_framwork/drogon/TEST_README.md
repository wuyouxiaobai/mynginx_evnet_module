下面是一个使用 **Drogon** 框架实现最简单“Hello, World” HTTP 服务的完整示例，包括 CMake 配置、main 函数、编译运行说明。

---

## ✅ 项目结构

```
hello_drogon/
├── CMakeLists.txt
└── main.cpp
```

---

## 📄 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.5)
project(hello_drogon)

set(CMAKE_CXX_STANDARD 17)

find_package(Drogon REQUIRED)

add_executable(hello_drogon main.cpp)
target_link_libraries(hello_drogon PRIVATE Drogon::Drogon)
```

---

## 📄 main.cpp

```cpp
#include <drogon/drogon.h>

int main() {
    // 注册路由，GET 请求 /hello 时返回 "hello world"
    drogon::app().registerHandler(
        "/hello",
        [](const drogon::HttpRequestPtr&,
           std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setBody("hello world");
            callback(resp);
        },
        {drogon::Get}
    );

    // 监听 0.0.0.0:8080
    drogon::app().addListener("0.0.0.0", 8080);
    drogon::app().run();
    return 0;
}
```

---

## 🛠️ 编译 & 运行

```bash
mkdir build
cd build
cmake ..
make
./hello_drogon
```

---

## 🌐 测试

浏览器访问：

```
http://localhost:8080/hello
```

你应该能看到：

```
hello world
```

---

如需我打包成压缩包或生成项目目录结构，请告诉我 👍