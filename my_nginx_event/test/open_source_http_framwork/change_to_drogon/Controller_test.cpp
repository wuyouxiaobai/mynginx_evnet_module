#include <Server.h>

// 示例 Controller 类

class HelloController {
public:
    void registerRoutes(WYXB::Server& app) {
        app.get("/hello/world", [](const WYXB::HttpRequest& req, WYXB::HttpResponse* res) {
            res->setBody("Hello from HelloController!");
        });
    }
};