#include <stddef.h>
#include "my_slogic.h"
#include "my_threadpool.h"
#include <signal.h> 
#include <string.h>
#include "my_macro.h"
#include "my_conf.h"
#include "my_func.h"



namespace WYXB
{
typedef struct
{
    int signo; // 信号对应数字编号
    const char* signame; // 信号名称
    
    // 信号处理函数，函数参数和返回值固定，函数体由我们自己实现
    // 定义一个函数指针
    void (*handler)(int signo, siginfo_t* siginfo, void* ucontext);
} ngx_signal_t;

class Server
{
public:
    Server();

private:

    static Server& instance() {
        static Server server; // 单例对象
        return server;
    }

    // 静态信号处理函数（符合普通函数指针类型）
    static void static_ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext) {
        instance().ngx_signal_handler(signo, siginfo, ucontext); // 调用实例的实际处理逻辑
    }


private:
//数组 ，定义本系统处理的各种信号，我们取一小部分nginx中的信号，并没有全部搬移到这里，日后若有需要根据具体情况再增加
//在实际商业代码中，你能想到的要处理的信号，都弄进来
    static ngx_signal_t signals[];


private:
//和设置标题有关的全局变量
    size_t g_argvneedmem{0}; //保存下argv参数需要的内存大小
    size_t g_envneedmem{0}; //保存下环境变量需要的内存大小
    int g_os_argc;//参数个数
    char **g_os_argv;//原始命令行参数数组，在main中被赋值
    char *gp_envmem{NULL};//指向自己分配的env环境变量的内存，在ngx_init_setproctitle()函数中会被分配内存
    int g_daemonized{0};//是否以守护进程运行，开启:1 关闭:0

//CSocket的全局变量
    CLogicSocket g_socket; //socket通信的逻辑层对象
    CThreadPool g_threadpool; //线程池对象

//进程本身相关的全局变量
    pid_t ngx_pid; //当前进程
    pid_t ngx_parent; //父进程
    int ngx_process; //进程类型
    int g_stopEvent;//进程退出事，退出：1 继续：0
    int ngx_terminate;//进程优雅退出：1 继续：0
    pid_t ngx_processes[NGX_MAX_PROCESSES]; // 定义一个固定大小的数组  
    int ngx_last_process;//工作进程数量

private:
//设置可执行程序标题相关函数
    void ngx_init_setproctitle();
    void ngx_setproctitle(const char *title);


// 进程相关函数
    //子进程循环监听epoll消息相关函数
    void ngx_process_events_and_timers();
    // 主进程循环，创建worker子进程
    void ngx_master_process_cycle();
    // 通知所有子进程退出  
    void ngx_signal_worker_processes(int signo);
    // 回收所有子进程  
    void ngx_reap_worker_processes();
    //processnums:要创建的子进程数量
    void ngx_start_worker_processes(int processnums);
    //描述：创建子进程，并调用worker_process_cycle函数，该函数负责处理请求
    //inum:子进程编号
    //pprocname:子进程名称
    int ngx_spawn_process(int inum,const char *pprocname);
    //描述：worker子进程的功能函数，每个woker子进程，就在这里循环着了（无限循环【处理网络事件和定时器事件以对外提供web服务】）
    //     子进程分叉才会走到这里
    //inum：进程编号【0开始】
    void ngx_worker_process_cycle(int inum,const char *pprocname);
    //描述：子进程创建时调用本函数进行一些初始化工作
    void ngx_worker_process_init(int inum);

//信号相关
    // 信号初始化
    int ngx_init_signals();
    // 信号触发的处理函数
    void ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext);
    // 获取子进程的结束状态，防止单独kill子进程时子进程变成僵尸进程
    void ngx_process_get_status(void);

//守护进程
    int ngx_daemon();
};
} // namespace WYXB
