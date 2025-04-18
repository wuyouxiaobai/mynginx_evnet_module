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
