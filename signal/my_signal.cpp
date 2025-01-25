#include <signal.h> 
#include <string.h>
#include "my_func.h"
#include "my_macro.h"
#include "my_global.h"
#include <errno.h>
#include <wait.h>


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

// 声明信号处理函数
static void ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext); //static表示该函数只在当前文件内可见
static void ngx_process_get_status(void); //获取子进程的结束状态，防止单独kill子进程时子进程变成僵尸进程

//数组 ，定义本系统处理的各种信号，我们取一小部分nginx中的信号，并没有全部搬移到这里，日后若有需要根据具体情况再增加
//在实际商业代码中，你能想到的要处理的信号，都弄进来
ngx_signal_t signals[] = {
    // signo, signame, handler
    { SIGHUP, "SIGHUP", ngx_signal_handler }, //终端断开信号，对于守护进程常用于reload重载配置文件通知--标识1 
    { SIGINT, "SIGINT", ngx_signal_handler }, //中断信号，对于用户终端常用于退出程序通知--标识2 
    { SIGTERM, "SIGTERM", ngx_signal_handler }, //终止信号，对于kill命令常用于杀死进程通知--标识15 
    { SIGCHLD, "SIGCHLD", ngx_signal_handler }, //子进程结束信号，对于父进程常用于回收僵尸进程通知--标识17 
    { SIGQUIT, "SIGQUIT", ngx_signal_handler }, //退出信号，对于用户终端常用于退出程序通知--标识3 
    { SIGIO, "SIGIO", ngx_signal_handler }, //异步IO信号，对于网络编程常用于通知事件发生--标识29 
    { SIGSYS, "SIGSYS, SIG_IGN", NULL}, //忽略这个信号，SIGSYS表示收到了一个无效系统调用，如果我们不忽略，进程会被操作系统杀死，--标识31
    //...日后根据需要再继续增加
    { 0, NULL, NULL } //数组结束标志
};
    
// 初始化信号的函数，用于注册信号处理函数
// 返回值：成功返回0，失败返回-1
int ngx_init_signals()
{
    ngx_signal_t *sig; // 信号数组指针
    struct sigaction sa; // 信号处理结构体
    
    for(sig = signals; sig->signo != 0; sig++)
    {
        // 设置信号结构体的处理函数
        memset(&sa, 0, sizeof(sa));
        
        if(sig->handler == NULL)
        {
            // 忽略信号
            sa.sa_handler = SIG_IGN; 
            //sa_handler:这个标记SIG_IGN给到sa_handler成员，表示忽略信号的处理程序，否则操作系统的缺省信号处理程序很可能把这个进程杀掉；
            //其实sa_handler和sa_sigaction都是一个函数指针用来表示信号处理程序。只不过这两个函数指针他们参数不一样， sa_sigaction带的参数多，信息量大，
            //而sa_handler带的参数少，信息量少；如果你想用sa_sigaction，那么你就需要把sa_flags设置为SA_SIGINFO；
        }
        else //如果信号处理函数不为空，这当然表示我要定义自己的信号处理函数
        {
            sa.sa_sigaction = sig->handler;//sa_sigaction：指定信号处理程序(函数)，注意sa_sigaction也是函数指针，是这个系统定义的结构sigaction中的一个成员（函数指针成员）；
            sa.sa_flags = SA_SIGINFO; //sa_flags：int型，指定信号的一些选项，设置了该标记(SA_SIGINFO)，就表示信号附带的参数可以被传递到信号处理函数中
                                        //说白了就是你要想让sa.sa_sigaction指定的信号处理程序(函数)生效，你就把sa_flags设定为SA_SIGINFO
        } 

        sigemptyset(&sa.sa_mask);//这里.sa_mask是个信号集（描述信号的集合），用于表示要阻塞的信号，sigemptyset()这个函数把信号集中的所有信号清0，本意就是不准备阻塞任何信号；
    
        // 设置信号处理函数，信号到来后调用自己设置的处理函数。有个老的同类函数叫signal，不过signal这个函数被认为是不可靠信号语义，不建议使用，大家统一用sigaction
        if(sigaction(sig->signo, &sa, NULL) == -1)//参数1：要操作的信号
                                                     //参数2：主要就是那个信号处理函数以及执行信号处理函数时候要屏蔽的信号等等内容
                                                      //参数3：返回以往的对信号的处理方式【跟sigprocmask()函数边的第三个参数是的】，跟参数2同一个类型，我们这里不需要这个东西，所以直接设置为NULL；
        {
            ngx_log_error_core(NGX_LOG_EMERG,errno,"sigaction(%s) failed",sig->signame); //显示到日志文件中去的 
            return -1; //有失败就直接返回
        }
        else
        {
            //ngx_log_error_core(NGX_LOG_EMERG,errno,"sigaction(%s) succed!",sig->signame);     //成功不用写日志 
            //ngx_log_stderr(0,"sigaction(%s) succed!",sig->signame); //直接往屏幕上打印看看 ，不需要时可以去掉
        }
    }  
    return 0; //成功                              
}

// 信号处理函数
static void ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext)
{
    //printf("来信号了\n");    
    ngx_signal_t *sig; // 信号数组指针
    char *action; // 一个字符串，用于记录一个动作字符串以往日志文件中写

    for (sig = signals; sig->signo != 0; sig++) //遍历信号数组    
    {         
        //找到对应信号，即可处理
        if (sig->signo == signo) 
        { 
            break;
        }
    } //end for

    action = (char*)"";
    
    if(ngx_process == NGX_PROCESS_MASTER) // master进程，管理进程，处理信号一般比较多
    {
        // master进程的信号处理
        switch(signo)
        {
        case SIGCHLD: // 子进程退出时会收到该信号
            ngx_reap = 1; // 设置ngx_reap标志，表示有子进程退出，日后master主进程的for(;;)循环中可能会用到这个变量【比如重新产生一个子进程】
            break;
        // ...日后根据需要再继续增加
        default:
            break;
        
        }
    }
    else if(ngx_process == NGX_PROCESS_WORKER) // worker进程，工作进程，处理信号一般比较少
    {
        //worker进程的往这里走
        //......以后再增加
        //....
    }
    else
    {
        //非master非worker进程，先啥也不干
        //do nothing
    }

    // 记录日志
    if(siginfo && siginfo->si_pid)
    {
        ngx_log_error_core(NGX_LOG_NOTICE,0,"signal %d (%s) received from %P%s", signo, sig->signame, siginfo->si_pid, action); 
    
    }
    else
    {
        ngx_log_error_core(NGX_LOG_NOTICE,0,"signal %d (%s) received %s",signo, sig->signame, action);//没有发送该信号的进程id，所以不显示发送该信号的进程id
    }

    //.......其他需要扩展的将来再处理；
    
    //子进程状态有变化，通常是意外退出【既然官方是在这里处理，我们也学习官方在这里处理】
    if (signo == SIGCHLD) 
    {
        ngx_process_get_status(); //获取子进程的结束状态
    } 
}


// 获取子进程的结束状态，防止单独kill子进程时子进程变成僵尸进程
static void ngx_process_get_status(void)
{
    pid_t pid;
    int status;
    int err;
    int one=0; //抄自官方nginx，应该是标记信号正常处理过一次
    
    // 杀死子进程时，父进程会收到SIGCHLD信号。
    for(;;)
    {
        //waitpid，有人也用wait,但老师要求大家掌握和使用waitpid即可；这个waitpid说白了获取子进程的终止状态，这样，子进程就不会成为僵尸进程了；
        //第一次waitpid返回一个> 0值，表示成功，后边显示 2019/01/14 21:43:38 [alert] 3375: pid = 3377 exited on signal 9【SIGKILL】
        //第二次再循环回来，再次调用waitpid会返回一个0，表示子进程还没结束，然后这里有return来退出；
        pid = waitpid(-1, &status, WNOHANG); //第一个参数为-1，表示等待任何子进程，
                                        //第二个参数：保存子进程的状态信息(大家如果想详细了解，可以百度一下)。
                                        //第三个参数：提供额外选项，WNOHANG表示不要阻塞，让这个waitpid()立即返回        
        if(pid == 0) //没有子进程退出，继续循环
        {
            return;
        }
        
        if(pid == -1)//这表示这个waitpid调用有错误，有错误也理解返回出去，我们管不了这么多
        {
            //这里处理代码抄自官方nginx，主要目的是打印一些日志。考虑到这些代码也许比较成熟，所以，就基本保持原样照抄吧；
            err = errno;
            if(err == EINTR) //被信号打断
            {
                continue;
            }
            
            if(err == ECHILD && one) //没有子进程退出，但是已经处理过一次了
            {
                return;
            }
            
            if(err == ECHILD)
            {
                ngx_log_error_core(NGX_LOG_ALERT,err,"waitpid() failed！");
                return;
            }
            ngx_log_error_core(NGX_LOG_ALERT,err,"waitpid() failed!");
            return;
            
        
        }
        //-------------------------------
        //走到这里，表示  成功【返回进程id】 ，这里根据官方写法，打印一些日志来记录子进程的退出
        one = 1; //标记信号正常处理过一次
        if(WTERMSIG(status)) //WTERMSIG(status) 是一个用于提取子进程终止信号编号的宏，常用于分析子进程的退出状态。
        {
            ngx_log_error_core(NGX_LOG_ALERT,0,"pid = %P exited on signal %d!",pid,WTERMSIG(status)); //获取使子进程终止的信号编号
            
        }
        else
        {
            ngx_log_error_core(NGX_LOG_NOTICE,0,"pid = %P exited with code %d!",pid,WEXITSTATUS(status)); //WEXITSTATUS()获取子进程传递给exit或者_exit参数的低八位
        }
    }
    return;
    
}
    
}