#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include "my_conf.h"
#include "my_server.h"

namespace WYXB {

static u_char master_process[] = "master process";

int Server::ngx_master_process_cycle() {
    if (ngx_init_signals() != 0) {
        Logger::ngx_log_error_core(NGX_LOG_ALERT, errno, "ngx_init_signals failed!");
        return 1;
    }

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGWINCH);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGQUIT);

    if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1) {
        Logger::ngx_log_error_core(NGX_LOG_ALERT, errno, "sigprocmask failed in master!");
    }

    size_t size = sizeof(master_process) + g_argvneedmem;
    if (size < 1000) {
        char title[1000] = {0};
        strcpy(title, (const char*)master_process);
        strcat(title, " ");
        for (int i = 0; i < g_os_argc; ++i) {
            strcat(title, g_os_argv[i]);
        }
        ngx_setproctitle(title);
        Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0, "%s %P [master] started", title, ngx_pid.load());
    }

    MyConf* config = MyConf::getInstance();
    int workprocess = config->GetIntDefault("WorkerProcesses", 1);
    ngx_start_worker_processes(workprocess);

    if (ngx_process == NGX_PROCESS_MASTER) {
        sigemptyset(&set);
        for (;;) {
            sigsuspend(&set);
        
            // 如果是子进程退出信号，则立即处理子进程状态
            if (ngx_reap) {
                ngx_reap_worker_processes();
                ngx_reap = 0;  // 复位标志
            }
        
            if (g_stopEvent) {
                Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0, "Master exiting...");
                ngx_signal_worker_processes(SIGTERM);
                ngx_reap_worker_processes();
                break;
            }
        
            Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0, "Master received signal, stopevent = %d", g_stopEvent.load());
        }
    }
    return 0;
}

void Server::ngx_signal_worker_processes(int signo) {  
    for (int i = 0; i < ngx_last_process; i++) {  
        if (ngx_processes[i] == -1) {  
            continue; // 跳过无效的子进程  
        }  
        kill(ngx_processes[i], signo); // 向子进程发送信号  
    }  
}  

void Server::ngx_reap_worker_processes() {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        int i;
        for (i = 0; i < ngx_last_process; ++i) {
            if (ngx_processes[i] == pid) break;
        }

        if (WIFSIGNALED(status) || WEXITSTATUS(status) != 0) {
            Logger::ngx_log_error_core(NGX_LOG_ALERT, 0,
                "Worker %P crashed (status=%d), restarting...", pid, WEXITSTATUS(status));
            ngx_processes[i] = ngx_spawn_process(i, "worker process");
        } else {
            Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0,
                "Worker %P exited normally", pid);
            ngx_processes[i] = -1;
        }
    }
    if (pid == -1 && errno != ECHILD) {
        Logger::ngx_log_error_core(NGX_LOG_ALERT, errno, "waitpid() failed while reaping workers.");
    }
}


void Server::ngx_start_worker_processes(int processnums) {
    for (int i = 0; i < processnums; ++i) {
        if (ngx_process == NGX_PROCESS_MASTER) ngx_spawn_process(i, "worker process");
        else break;
    }
}

int Server::ngx_spawn_process(int inum, const char* pprocname) {
    pid_t pid = fork();
    if (pid == -1) {
        Logger::ngx_log_error_core(NGX_LOG_ALERT, errno, "fork failed for process %d (%s)", inum, pprocname);
        return -1;
    } else if (pid == 0) {
        ngx_parent = ngx_pid.load();
        ngx_pid = getpid();
        ngx_worker_process_cycle(inum, pprocname);
    } else {
        ngx_processes[inum] = pid;
        ++ngx_last_process;
    }
    return pid;
}

void Server::ngx_worker_process_cycle(int inum, const char* pprocname) {
    if (ngx_init_signals() != 0 || !Server::instance().g_socket->Initialize()) {
        Logger::ngx_log_error_core(NGX_LOG_ALERT, errno, "Worker init failed!");
        exit(-1);
    }

    sigset_t empty_set;
    sigemptyset(&empty_set);
    sigprocmask(SIG_SETMASK, &empty_set, nullptr);

    ngx_process = NGX_PROCESS_WORKER;
    ngx_worker_process_init(inum);

    ngx_setproctitle(pprocname);
    Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0, "%s %P [worker] started", pprocname, ngx_pid.load());

    while (!g_stopEvent) {
        ngx_process_events_and_timers();
    }

    Logger::ngx_log_error_core(NGX_LOG_NOTICE, 0, "Worker %P exiting...", ngx_pid.load());
    Server::instance().g_threadpool->StopAll();
    Server::instance().g_socket->Shotdown_subproc();
}

void Server::ngx_worker_process_init(int inum) {
    sigset_t set;
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, nullptr);

    Logger::ngx_log_error_core(NGX_LOG_INFO, 0, "Initializing threadpool...");
    MyConf* config = MyConf::getInstance();
    int threads = config->GetIntDefault("ProcMsgRecvWorkThreadCount", 5);
    if (!Server::instance().g_threadpool->Create(threads)) exit(-2);
    sleep(1);

    Logger::ngx_log_error_core(NGX_LOG_INFO, 0, "Initializing socket threads...");
    if (!Server::instance().g_socket->Initialize_subproc()) exit(-2);

    Server::instance().g_socket->ngx_epoll_init();
}

} // namespace WYXB
