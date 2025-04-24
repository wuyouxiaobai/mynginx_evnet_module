#include <signal.h>
#include <string.h>
#include <errno.h>
#include <wait.h>
#include "my_server.h"

namespace WYXB {

// 信号结构体定义
// 包括信号编号、信号名称、信号处理函数指针
ngx_signal_t Server::signals[] = {
    { SIGHUP,  "SIGHUP",  static_ngx_signal_handler },
    { SIGINT,  "SIGINT",  static_ngx_signal_handler },
    { SIGTERM, "SIGTERM", static_ngx_signal_handler },
    { SIGCHLD, "SIGCHLD", static_ngx_signal_handler },
    { SIGQUIT, "SIGQUIT", static_ngx_signal_handler },
    { SIGIO,   "SIGIO",   static_ngx_signal_handler },
    { SIGSYS,  "SIGSYS",  nullptr }, // 忽略非法系统调用
    { 0,       nullptr,    nullptr }  // 终止标志
};

// 初始化信号处理
int Server::ngx_init_signals() {
    struct sigaction sa;
    for (ngx_signal_t* sig = signals; sig->signo != 0; ++sig) {
        memset(&sa, 0, sizeof(sa));
        if (sig->handler == nullptr) {
            sa.sa_handler = SIG_IGN; // 忽略信号
        } else {
            sa.sa_sigaction = sig->handler;
            sa.sa_flags = SA_SIGINFO;
        }
        sigemptyset(&sa.sa_mask);
        if (sigaction(sig->signo, &sa, nullptr) == -1) {
            Logger::ngx_log_error_core(NGX_LOG_EMERG, errno, "sigaction(%s) failed", sig->signame);
            return -1;
        }
    }
    return 0;
}

// 信号处理函数
void Server::ngx_signal_handler(int signo, siginfo_t* siginfo, void* ucontext) {
    ngx_signal_t* sig;
    const char* action = "";

    for (sig = signals; sig->signo != 0; ++sig) {
        if (sig->signo == signo) break;
    }

    if (ngx_process == NGX_PROCESS_MASTER) {
        switch (signo) {
            case SIGTERM:
                g_stopEvent = 1;
                action = "shutting down";
                break;
            case SIGQUIT:
                g_stopEvent = 1;
                action = "gracefully shutting down";
                break;
            case SIGCHLD:
                ngx_reap = 1;
                break;
        }
    } else if (ngx_process == NGX_PROCESS_WORKER) {
        if (signo == SIGTERM || signo == SIGQUIT) {
            g_stopEvent = 1;
            action = "exiting";
        }
    }

    if (siginfo && siginfo->si_pid) {
        Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0,
            "process %P signal %d (%s) received from %P %s stopevent %d",
            ngx_pid.load(), signo, sig->signame, siginfo->si_pid, action, g_stopEvent.load());
    } else {
        Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0,
            "process %P signal %d (%s) received %s stopevent %d",
            ngx_pid.load(), signo, sig->signame, action, g_stopEvent.load());
    }

    if (signo == SIGCHLD) {
        ngx_process_get_status();
    }
}

// 处理子进程退出状态，避免僵尸进程
void Server::ngx_process_get_status() {
    pid_t pid;
    int status;
    int one = 0;

    while (true) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0) return;      // 没有子进程退出
        if (pid == -1) {
            int err = errno;
            if (err == EINTR) continue;
            if (err == ECHILD && one) return;
            Logger::ngx_log_error_core(NGX_LOG_ALERT, err, "waitpid() failed!");
            return;
        }
        one = 1;
        if (WTERMSIG(status)) {
            Logger::ngx_log_error_core(NGX_LOG_ALERT, 0, "pid = %P exited on signal %d!", pid, WTERMSIG(status));
        } else {
            Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0, "pid = %P exited with code %d!", pid, WEXITSTATUS(status));
        }
    }
}

} // namespace WYXB
