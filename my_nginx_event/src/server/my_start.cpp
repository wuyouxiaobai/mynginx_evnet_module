#include "my_server.h"

namespace WYXB
{
std::atomic<int> Server::g_stopEvent{0};
std::atomic<int> Server::ngx_pid{-1}; //当前进程
std::atomic<int> Server::ngx_parent{-1}; //父进程

int Server::run(int argc, char* const* argv)
{
    ngx_pid = getpid();
    ngx_parent = getppid();
    ngx_last_process = 0;
    g_stopEvent = 0;
    ngx_terminate = 0;

    g_os_argc = argc;
    g_os_argv = (char**)argv;

    // 统计参数和环境变量长度
    g_argvneedmem = 0;
    for (int i = 0; i < argc; ++i)
        g_argvneedmem += strlen(argv[i]) + 1;

    for (int i = 0; environ[i]; ++i)
        g_envneedmem += strlen(environ[i]) + 1;

    ngx_process = NGX_PROCESS_MASTER;
    ngx_reap = 0;

    // 配置加载
    MyConf* myconf = MyConf::getInstance();
    if (!myconf->LoadConf("nginx.conf"))
    {
        Logger::ngx_log_init();
        Logger::ngx_log_stderr(0, "配置文件[%s]载入失败，退出!", "nginx.conf");
        Logger::ngx_log_stderr(0, "程序退出，再见了!");
        freeresource();
        return 2;
    }

    // 日志初始化（依赖配置项）
    Logger::ngx_log_init();

    // 设置进程标题内存
    ngx_init_setproctitle();

    // 守护进程化
    if (myconf->GetIntDefault("Daemon", 0) == 1)
    {
        int daemon_ret = ngx_daemon();
        if (daemon_ret == -1)
        {
            Logger::ngx_log_stderr(0, "守护进程 fork 失败");
            Logger::ngx_log_stderr(0, "程序退出，再见了!");
            freeresource();
            return 1;
        }
        if (daemon_ret == 1)
        {
            // 父进程直接退出，留守护子进程继续执行
            freeresource();
            return 0;
        }
        g_daemonized = 1;
    }

    // 启动 master 进程循环
    ngx_master_process_cycle();

    // 退出清理
    if (ngx_process == NGX_PROCESS_MASTER)
        Logger::ngx_log_stderr(0, "程序退出，再见了!");
    freeresource();
    return 0;
}

}