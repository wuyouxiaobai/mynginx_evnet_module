#include "middlewareChain.h"
#include "my_logger.h"

namespace WYXB
{


void MiddlewareChain::addMiddleware(std::shared_ptr<Middleware> middleware)
{
    middlewares_.push_back(middleware);
}

void MiddlewareChain::processBefore(HttpRequest &request)
{
    for (auto &middleware : middlewares_)
    {
        middleware->before(request);
    }
}

void MiddlewareChain::processAfter(HttpResponse &response)
{
    try
    {
        // 反向处理响应，以保持中间件的正确执行顺序
        for (auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it)
        {
            if (*it)
            { // 添加空指针检查
                (*it)->after(response);
            }
        }
    }
    catch (const std::exception &e)
    {
        Logger::ngx_log_stderr(0, "Error in middleware after processing: %s", e.what());
    }
}

}
