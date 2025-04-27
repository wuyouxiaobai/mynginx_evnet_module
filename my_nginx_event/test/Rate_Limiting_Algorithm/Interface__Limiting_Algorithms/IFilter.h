// IFilter.h
#pragma once
#include "my_http_request.h"
#include "my_http_response.h"
#include <functional>


namespace WYXB {
using HttpResponsePtr = std::shared_ptr<HttpResponse>;
using HttpRequestPtr = std::shared_ptr<HttpRequest>;
using FilterCallback = std::function<void(HttpResponsePtr)>;
using FilterChainCallback = std::function<void()>;

// 所有Filter都继承这个接口
class IFilter
{
public:
    virtual ~IFilter() = default;
    virtual void doFilter(const HttpRequestPtr& req,
                          FilterCallback&& fcb,
                          FilterChainCallback&& fccb) = 0;
};

using IFilterPtr = std::shared_ptr<IFilter>;

} // namespace WYXB
