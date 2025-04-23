// #pragma once
// #include <stddef.h>
// #include "my_slogic.h"
// #include "my_threadpool.h"
// #include <signal.h> 
// #include <string.h>
// #include "my_macro.h"
// #include "my_conf.h"
// #include "my_func.h"
// #include <memory>


// namespace WYXB
// {
// typedef struct
// {
//     int signo; // 信号对应数字编号
//     const char* signame; // 信号名称
    
//     // 信号处理函数，函数参数和返回值固定，函数体由我们自己实现
//     // 定义一个函数指针
//     void (*handler)(int signo, siginfo_t* siginfo, void* ucontext);
// } ngx_signal_t;

// class Server
// {
// friend class CSocket;
// friend class CLogicSocket;
// friend class Logger;
// public:
//     Server(): g_socket(std::make_shared<CLogicSocket>()), g_threadpool(std::make_shared<CThreadPool>()) {}
//     ~Server() = default;

//     int start(int argc, char*const *argv);

//     void freeresource()
//     {

//         if(gp_envmem)
//         {
//             delete []gp_envmem;
//             gp_envmem = NULL;
//         }

//         Logger::ngx_log_close();
//     }

//     static Server& instance() {
//         static Server server; // 单例对象
//         return server;
//     }

//     // 静态信号处理函数（符合普通函数指针类型）
//     static void static_ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext) {
//         Server& server = instance();  
//         pid_t current_pid = getpid();  
    
//     // 详细日志  
//         Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0,"static_ngx_signal_handler Server Instance Addr: %P, Current PID: %d",   
//            &server, current_pid);
//         instance().ngx_signal_handler(signo, siginfo, ucontext); // 调用实例的实际处理逻辑
//     }


// private:
// //数组 ，定义本系统处理的各种信号，我们取一小部分nginx中的信号，并没有全部搬移到这里，日后若有需要根据具体情况再增加
// //在实际商业代码中，你能想到的要处理的信号，都弄进来
//     static ngx_signal_t signals[];


// private:
// //和设置标题有关的全局变量
//     size_t g_argvneedmem{0}; //保存下argv参数需要的内存大小
//     size_t g_envneedmem{0}; //保存下环境变量需要的内存大小
//     int g_os_argc;//参数个数
//     char **g_os_argv;//原始命令行参数数组，在main中被赋值
//     char *gp_envmem{NULL};//指向自己分配的env环境变量的内存，在ngx_init_setproctitle()函数中会被分配内存
//     int g_daemonized{0};//是否以守护进程运行，开启:1 关闭:0


// //进程本身相关的全局变量
//     static std::atomic<int> ngx_pid; //当前进程
//     static std::atomic<int> ngx_parent; //父进程
//     int ngx_process; //进程类型
//     static std::atomic<int> g_stopEvent;//进程退出事，退出：1 继续：0
//     int ngx_terminate;//进程优雅退出：1 继续：0
//     pid_t ngx_processes[NGX_MAX_PROCESSES]; // 定义一个固定大小的数组  
//     int ngx_last_process;//工作进程数量

// //标记子进程状态变化[一般是子进程发来SIGCHLD信号表示退出],sig_atomic_t:系统定义的类型：访问或改变这些变量需要在计算机的一条指令内完成
// //一般等价于int【通常情况下，int类型的变量通常是原子访问的，也可以认为 sig_atomic_t就是int类型的数据】  
//     sig_atomic_t ngx_reap;

    

// private:
// //设置可执行程序标题相关函数
//     void ngx_init_setproctitle();
//     void ngx_setproctitle(const char *title);


// // 进程相关函数
//     //子进程循环监听epoll消息相关函数
//     void ngx_process_events_and_timers();
//     // 主进程循环，创建worker子进程
//     int ngx_master_process_cycle();
//     // 通知所有子进程退出  
//     void ngx_signal_worker_processes(int signo);
//     // 回收所有子进程  
//     void ngx_reap_worker_processes();
//     //processnums:要创建的子进程数量
//     void ngx_start_worker_processes(int processnums);
//     //描述：创建子进程，并调用worker_process_cycle函数，该函数负责处理请求
//     //inum:子进程编号
//     //pprocname:子进程名称
//     int ngx_spawn_process(int inum,const char *pprocname);
//     //描述：worker子进程的功能函数，每个woker子进程，就在这里循环着了（无限循环【处理网络事件和定时器事件以对外提供web服务】）
//     //     子进程分叉才会走到这里
//     //inum：进程编号【0开始】
//     void ngx_worker_process_cycle(int inum,const char *pprocname);
//     //描述：子进程创建时调用本函数进行一些初始化工作
//     void ngx_worker_process_init(int inum);

// //信号相关
//     // 信号初始化
//     int ngx_init_signals();
//     // 信号触发的处理函数
//     void ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext);
//     // 获取子进程的结束状态，防止单独kill子进程时子进程变成僵尸进程
//     void ngx_process_get_status(void);

// //守护进程
//     int ngx_daemon();

// public:


// //线程池
//     std::shared_ptr<CThreadPool> g_threadpool;

// //socket相关
//     std::shared_ptr<CLogicSocket> g_socket;

// public:
// /// 路由相关
// // 通用回调函数注册方法  
//     bool registGet(const std::string& path, Router::HandlerCallback callback) {  
//         g_socket->registCallback(  
//             HttpRequest::Method::kGet,   
//             path,   
//             callback  
//         );  
//         return true;  
//     }  

//     bool registPut(const std::string& path, Router::HandlerCallback callback) {  
//         g_socket->registCallback( 
//             HttpRequest::Method::kPut,   
//             path,   
//             callback  
//         );  
//         return true;  
//     }  

//     bool registPost(const std::string& path, Router::HandlerCallback callback) {  
//         g_socket->registCallback( 
//             HttpRequest::Method::kPost,   
//             path,   
//             callback  
//         );  
//         return true;  
//     }  

//     bool registDelete(const std::string& path, Router::HandlerCallback callback) {  
//         g_socket->registCallback( 
//             HttpRequest::Method::kDelete,   
//             path,   
//             callback  
//         );  
//         return true;  
//     }  

//     // 通用方法，支持任意 HTTP 方法  
//     bool registRoute(HttpRequest::Method method,   
//                         const std::string& path,   
//                         Router::HandlerCallback callback) {  
//         g_socket->registCallback(method, path, callback);  
//         return true;  
//     }

// /// 中间件相关
// public:
//     void addMiddleware(std::shared_ptr<Middleware> middleware)
//     {
//         g_socket->addMiddleware(middleware);
//     }

// };
// } // namespace WYXB

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

private:
    Server() : 
        g_socket(std::make_shared<CLogicSocket>()), 
        g_threadpool(std::make_shared<CThreadPool>()) {}

    ~Server() = default;
public:
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

