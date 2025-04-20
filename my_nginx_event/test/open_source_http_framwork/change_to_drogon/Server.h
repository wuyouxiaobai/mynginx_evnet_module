#pragma once
#include <stddef.h>
#include <signal.h>
#include <string.h>
#include <memory>
#include <atomic>
#include <queue>
#include <string>
#include <vector>
#include <functional>

#include "my_slogic.h"
#include "my_threadpool.h"
#include "my_macro.h"
#include "my_conf.h"
#include "my_func.h"

namespace WYXB
{

// 信号定义
typedef struct {
    int signo;
    const char* signame;
    void (*handler)(int signo, siginfo_t* siginfo, void* ucontext);
} ngx_signal_t;

class Server {
friend class CSocket;
friend class CLogicSocket;
friend class Logger;

public:
    Server() : 
        g_socket(std::make_shared<CLogicSocket>()), 
        g_threadpool(std::make_shared<CThreadPool>()) {}

    ~Server() = default;

    static Server& instance() {
        static Server server;
        return server;
    }

    // 改为 run 接口
    int run(int argc, char* const* argv);

    // 链式中间件注册
    Server& addMiddleware(std::shared_ptr<Middleware> middleware) {
        g_socket->addMiddleware(std::move(middleware));
        return *this;
    }

    // 链式路由注册
    Server& on(HttpRequest::Method method, const std::string& path, Router::HandlerCallback callback) {
        g_socket->registCallback(method, path, callback);
        return *this;
    }

    Server& get(const std::string& path, Router::HandlerCallback cb) {
        return on(HttpRequest::Method::kGet, path, cb);
    }

    Server& post(const std::string& path, Router::HandlerCallback cb) {
        return on(HttpRequest::Method::kPost, path, cb);
    }

    Server& put(const std::string& path, Router::HandlerCallback cb) {
        return on(HttpRequest::Method::kPut, path, cb);
    }

    Server& del(const std::string& path, Router::HandlerCallback cb) {
        return on(HttpRequest::Method::kDelete, path, cb);
    }

    void freeresource() {
        if (gp_envmem) {
            delete[] gp_envmem;
            gp_envmem = NULL;
        }
        Logger::ngx_log_close();
    }

    // 注册控制器
    template<typename ControllerType>
    void registerController() {
        ControllerType ctl;
        ctl.registerRoutes(*this);
    }

    // 信号处理器（静态 -> 实例）
    static void static_ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext) {
        instance().ngx_signal_handler(signo, siginfo, ucontext);
    }

private:
    // 信号配置数组
    static ngx_signal_t signals[];

    // 启动参数和环境变量管理
    size_t g_argvneedmem{0};
    size_t g_envneedmem{0};
    int g_os_argc;
    char **g_os_argv;
    char *gp_envmem{NULL};
    int g_daemonized{0};

    // 进程管理
    static std::atomic<int> ngx_pid;
    static std::atomic<int> ngx_parent;
    int ngx_process;
    static std::atomic<int> g_stopEvent;
    int ngx_terminate;
    pid_t ngx_processes[NGX_MAX_PROCESSES];
    int ngx_last_process;
    sig_atomic_t ngx_reap;

private:
    // 标题设置
    void ngx_init_setproctitle();
    void ngx_setproctitle(const char *title);

    // 主-子进程逻辑
    int ngx_master_process_cycle();
    void ngx_signal_worker_processes(int signo);
    void ngx_reap_worker_processes();
    void ngx_start_worker_processes(int processnums);
    int ngx_spawn_process(int inum,const char *pprocname);
    void ngx_worker_process_cycle(int inum,const char *pprocname);
    void ngx_worker_process_init(int inum);
    void ngx_process_events_and_timers();

    // 信号相关
    int ngx_init_signals();
    void ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext);
    void ngx_process_get_status();

    // 守护进程支持
    int ngx_daemon();

public:
    std::shared_ptr<CThreadPool> g_threadpool;
    std::shared_ptr<CLogicSocket> g_socket;
};


} // namespace WYXB
