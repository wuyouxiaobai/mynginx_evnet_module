#include <sys/types.h>
#include <unistd.h>
#include "my_func.h"
#include "my_macro.h"
#include <errno.h>
#include "my_global.h"
#include <signal.h>
#include "my_conf.h"
#include "string.h"
#include <sys/wait.h>


namespace WYXB
{
// 函数申明
static void ngx_start_worker_processes(int processnums);
static int ngx_spawn_process(int threadnums,const char *pprocname);
static void ngx_worker_process_cycle(int inum,const char *pprocname);
static void ngx_worker_process_init(int inum);
static void ngx_reap_worker_processes();
static void ngx_signal_worker_processes(int signo);
// 申明主进程
static u_char master_process[] = "master process";


// 主进程循环，创建worker子进程
void ngx_master_process_cycle()
{
    sigset_t set;        //信号集

    sigemptyset(&set);   //清空信号集

    //设置屏蔽掉的信号
    //fork()子进程是也可以这么做
    sigaddset(&set, SIGCHLD); //子进程状态变化
    sigaddset(&set, SIGALRM); //定时器超时 
    sigaddset(&set, SIGIO);   //异步IO事件
    sigaddset(&set, SIGINT);  //终端中断
    sigaddset(&set, SIGHUP);  //连接断开，终端挂起
    sigaddset(&set, SIGUSR1); //用户自定义信号1
    sigaddset(&set, SIGUSR2); //用户自定义信号2
    sigaddset(&set, SIGWINCH); //终端窗口大小变化
    sigaddset(&set, SIGTERM); //终止进程
    sigaddset(&set, SIGQUIT); //终端退出
    //.........可以根据开发的实际需要往其中添加其他要屏蔽的信号......

    //设置，此时无法接受的信号；阻塞期间，你发过来的上述信号，多个会被合并为一个，暂存着，等你放开信号屏蔽后才能收到这些信号。。。
    if(sigprocmask(SIG_BLOCK, &set, NULL) == -1) //第一个参数用了SIG_BLOCK表明设置 进程 新的信号屏蔽字 为 “当前信号屏蔽字 和 第二个参数指向的信号集的并集
    {
        ngx_log_error_core(NGX_LOG_ALERT,errno,"ngx_master_process_cycle()中sigprocmask()失败!");
    }
    //即便sigprocmask失败，程序流程也会继续

    //首先我设置主进程标题---------begin
    size_t size;
    int i;
    size = sizeof(master_process); 
    size += g_argvneedmem; //加上argv参数长度
    if(size < 1000) //长度小于这个，我才设置标题
    {
        char title[1000] = {0};
        strcpy(title, (const char*)master_process);
        strcat(title, " ");
        for(i=0; i < g_os_argc; i++)
        {
            strcat(title, g_os_argv[i]);
        }
        ngx_setproctitle(title); //设置进程标题
        ngx_log_error_core(NGX_LOG_NOTICE,0,"%s %P 【master进程】启动并开始运行......!",title,ngx_pid); //设置标题时顺便记录下来进程名，进程id等信息到日志
    }
    //首先我设置主进程标题---------end

    //从配置文件中读取要创建的worker进程数量
    MyConf* config = MyConf::getInstance(); //初始化配置文件
    int workprocess = config->GetIntDefault("WorkerProcesses", 1); // 工作进程的数量，默认为1
    ngx_start_worker_processes(workprocess);  //这里要创建worker子进程


    if(ngx_process == NGX_PROCESS_MASTER)
    {
        //创建子进程后，父进程的执行流程会返回到这里，子进程不会走进来   
        sigemptyset(&set); //信号屏蔽字为空，表示不屏蔽任何信号



        // for(;;)
        // {
        //     sigsuspend(&set); //阻塞在sigsuspend()，等待信号的到来，直到收到信号后，sigsuspend()返回，然后继续执行。
        // }

        // 主进程的主循环  
        for (;;) {  
            if (g_stopEvent) 
            { 
                // 如果收到退出信号，优雅退出  
                ngx_log_error_core(NGX_LOG_NOTICE, 0, "Master process exiting...");  
                ngx_signal_worker_processes(SIGTERM); // 通知所有子进程退出  

                // 等待所有子进程退出  
                ngx_reap_worker_processes();  

                break; // 退出主循环  
            }  

            sigsuspend(&set); // 阻塞在这里，等待信号的到来  
        }  
    }
    
    return;
    
}

// 通知所有子进程退出  
static void ngx_signal_worker_processes(int signo) {  
    for (int i = 0; i < ngx_last_process; i++) {  
        if (ngx_processes[i] == -1) {  
            continue; // 跳过无效的子进程  
        }  
        kill(ngx_processes[i], signo); // 向子进程发送信号  
    }  
}  

// 回收所有子进程  
static void ngx_reap_worker_processes() {  
    pid_t pid;  
    int status;  

    // 使用阻塞模式回收所有子进程  
    while ((pid = waitpid(-1, &status, 0)) > 0) {  
        ngx_log_error_core(NGX_LOG_NOTICE, 0, "Worker process %P exited with status %d.", pid, WEXITSTATUS(status));  
    }  

    // 如果没有子进程，waitpid 会返回 -1，errno 会被设置为 ECHILD  
    if (pid == -1 && errno != ECHILD) {  
        ngx_log_error_core(NGX_LOG_ALERT, errno, "waitpid() failed while reaping worker processes.");  
    }  
}



//描述：根据给定的参数创建指定数量的子进程，因为以后可能要扩展功能，增加参数，所以单独写成一个函数
//processnums:要创建的子进程数量
static void ngx_start_worker_processes(int processnums)
{
    //创建子进程，由master进程执行该函数实现
    for(int i=0;i<processnums;i++)
    {
        if(ngx_process == NGX_PROCESS_MASTER)
            ngx_spawn_process(i, "worker process");
        else
            break;
    }

    
}

//描述：创建子进程，并调用worker_process_cycle函数，该函数负责处理请求
//inum:子进程编号
//pprocname:子进程名称
static int ngx_spawn_process(int inum,const char *pprocname)
{
    pid_t pid;
    
    pid = fork();
    switch (pid)
    {
    case -1:
        ngx_log_error_core(NGX_LOG_ALERT,errno,"ngx_spawn_process()fork()产生子进程num=%d,procname=\"%s\"失败!",inum,pprocname);
        return -1;
    case 0: //子进程进入循环处理请求
        ngx_parent = ngx_pid;
        ngx_pid = getpid();
        ngx_worker_process_cycle(inum,pprocname); //所有子进程在该函数循环处理请求
    
    default: //父进程直接退出
        ngx_processes[inum] = pid;
        ngx_last_process++;
        break;
    }
    //父进程分支会走到这里，子进程流程不往下边走-------------------------
    //若有需要，以后再扩展增加其他代码......
    return pid;
}

//描述：worker子进程的功能函数，每个woker子进程，就在这里循环着了（无限循环【处理网络事件和定时器事件以对外提供web服务】）
//     子进程分叉才会走到这里
//inum：进程编号【0开始】
static void ngx_worker_process_cycle(int inum,const char *pprocname) 
{
    sigset_t empty_set;  
    sigemptyset(&empty_set); // 清空信号屏蔽集  
    if (sigprocmask(SIG_SETMASK, &empty_set, NULL) == -1) {  
        perror("sigprocmask failed in child process");  
    }  

    //设置一下变量
    ngx_process = NGX_PROCESS_WORKER;  //设置进程的类型，是worker进程
    
    // 重新为子进程设置进程名，不要与父进程重复
    ngx_worker_process_init(inum);

    ngx_setproctitle(pprocname); //设置进程名
    ngx_log_error_core(NGX_LOG_NOTICE,0,"%s %P 【worker进程】启动并开始运行......!",pprocname,ngx_pid); //设置标题时顺便记录下来进程名，进程id等信息到日志

    //测试代码，测试线程池的关闭
    //sleep(5); //休息5秒        
    //g_threadpool.StopAll(); //测试Create()后立即释放的效果

    //暂时先放个死循环，我们在这个循环里一直不出来
    //setvbuf(stdout,NULL,_IONBF,0); //这个函数. 直接将printf缓冲区禁止， printf就直接输出了。
    


    for(;;)
    {
        // 检查退出标志  
        if (g_stopEvent)   
        {  
            ngx_log_error_core(NGX_LOG_NOTICE, 0, "Worker process %P is exiting gracefully...", ngx_pid);  
            break; // 跳出循环，准备退出  
        }  
        // 处理网络事件和定时器事件
        ngx_process_events_and_timers(); //处理网络事件和定时器事件


    }
    
    // 子进程退出
    g_threadpool.StopAll();
    g_socket.Shotdown_subproc();

}

//描述：子进程创建时调用本函数进行一些初始化工作
static void ngx_worker_process_init(int inum)
{
    sigset_t set; //信号集：用于定义一组信号，主要用于管理和控制信号的处理。可以通过信号集来屏蔽某些信号，或者检查某个信号是否被设置。
    sigemptyset(&set); //清空信号集
    


    //将空的信号集设置为新的屏蔽信号集
    if(sigprocmask(SIG_SETMASK,&set,NULL) == -1) //原来是屏蔽那10个信号【防止fork()期间收到信号导致混乱】，现在不再屏蔽任何信号【接收任何信号】
    {
        ngx_log_error_core(NGX_LOG_ALERT,errno,"ngx_worker_process_init()中sigprocmask()失败!");
    }


    // 线程池代码，要比socket相关内容优先
    MyConf* config = MyConf::getInstance(); //初始化配置文件
    int tmpthreadnum = config->GetIntDefault("ProcMsgRecvWorkThreadCount", 5); // 处理接收消息线程池中线程的数量，默认为5
    if(g_threadpool.Create(tmpthreadnum) == false)// 工作进程创建线程池中线程（处理接受消息队列中的消息）
    {
        //内存没释放，但是简单粗暴退出；
        exit(-2);
    }
    sleep(1);


    if(g_socket.Initialize_subproc() == false) // 初始化子进程
    {
        //内存没释放，但是简单粗暴退出；
        exit(-2);
    }


    //工作线程中创建listenfd，所有工作线程通过端口重用监听同一个端口，通过linux内核解决的负载均衡解决惊群问题
    g_socket.ngx_epoll_init();           //初始化epoll相关内容，同时 往监听socket上增加监听事件，从而开始让监听端口履行其职责
    //g_socket.ngx_epoll_listenportstart();//往监听socket上增加监听事件，从而开始让监听端口履行其职责【如果不加这行，虽然端口能连上，但不会触发ngx_epoll_process_events()里边的epoll_wait()往下走】
    

    //....将来再扩充代码
    //....
}


}