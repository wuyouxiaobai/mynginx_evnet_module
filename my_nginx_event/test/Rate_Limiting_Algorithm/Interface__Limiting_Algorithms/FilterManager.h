// FilterManager.h
#pragma once
#include "IFilter.h"
#include <vector>

namespace WYXB {

// 简单串行执行Filter链
class FilterManager
{
public:
    void addFilter(IFilterPtr filter) {
        filters_.emplace_back(std::move(filter));
    }

    void doFilters(const HttpRequestPtr& req, 
                   FilterCallback&& finalCallback,
                   std::function<void()> controllerCallback) {
        doFilterImpl(req, 0, std::move(finalCallback), std::move(controllerCallback));
    }

private:
    void doFilterImpl(const HttpRequestPtr& req, 
                      size_t index,
                      FilterCallback&& finalCallback,
                      std::function<void()> controllerCallback) {
        if (index >= filters_.size()) {
            controllerCallback();  // 执行最终 Controller 逻辑
            return;
        }

        filters_[index]->doFilter(req,
            [finalCallback](HttpResponsePtr resp) { // fcb: 有Filter拦截
                finalCallback(std::move(resp));
            },
            [this, &req, index, finalCallback = std::move(finalCallback), controllerCallback]() mutable { // fccb: 继续下一个
                doFilterImpl(req, index + 1, std::move(finalCallback), std::move(controllerCallback));
            }
        );
    }

private:
    std::vector<IFilterPtr> filters_;
};

} // namespace WYXB
