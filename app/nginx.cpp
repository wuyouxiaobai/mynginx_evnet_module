#include "stddef.h"
#include "my_slogic.h"
#include "my_threadpool.h"
#include <signal.h> 
#include <string.h>
#include "my_global.h"
#include "my_macro.h"
#include "my_conf.h"
#include "my_func.h"
#include "my_crc32.h"

namespace WYXB
{

//本文件用的函数声明
static void freeresource(); //释放资源

//和设置标题有关的全局变量
size_t g_argvneedmem = 0; //保存下argv参数需要的内存大小
size_t g_envneedmem = 0; //保存下环境变量需要的内存大小
int g_os_argc;//参数个数
char **g_os_argv;//原始命令行参数数组，在main中被赋值
char *gp_envmem=NULL;//指向自己分配的env环境变量的内存，在ngx_init_setproctitle()函数中会被分配内存
int g_daemonized = 0;//是否以守护进程运行，开启:1 关闭:0

//CSocket的全局变量
WYXB::CLogicSocket g_socket; //socket通信的逻辑层对象
WYXB::CThreadPool g_threadpool; //线程池对象

//进程本身相关的全局变量
pid_t ngx_pid; //当前进程
pid_t ngx_parent; //父进程
int ngx_process; //进程类型
int g_stopEvent;//进程退出事，退出：1 继续：0
int ngx_terminate;//进程优雅退出：1 继续：0
pid_t ngx_processes[NGX_MAX_PROCESSES]; // 定义一个固定大小的数组  
int ngx_last_process;//工作进程数量


sig_atomic_t ngx_reap; //标记子进程状态变化[一般是子进程发来SIGCHLD信号表示退出],sig_atomic_t:系统定义的类型：访问或改变这些变量需要在计算机的一条指令内完成
                                   //一般等价于int【通常情况下，int类型的变量通常是原子访问的，也可以认为 sig_atomic_t就是int类型的数据】       


}

//程序主入口函数--------------------------------------
int main(int argc, char*const *argv)
{
    int exitcode = 0;           //退出代码，先给0表示正常退出
    int i;                      //临时用
    
    WYXB::g_stopEvent = 0;            //标记程序是否退出，0不退出    
    WYXB::ngx_terminate = 0;          ////标记程序是否要优雅退出，0不终止      
    
    WYXB::ngx_pid = getpid();         //获取当前进程的PID
    WYXB::ngx_parent = getppid();      //获取父进程的PID
    WYXB::ngx_last_process = 0;      //工作进程数量
    
    //统计argc所占内存
    WYXB::g_argvneedmem = 0;
    for(i=0;i<argc;i++) //argv =  ./nginx -a -b -c asdfas
    {
        WYXB::g_argvneedmem += strlen(argv[i]) + 1; //+1是为了保存每个参数的结束符'\0'
    }
    //统计环境变量所占的内存。注意判断方法是environ[i]是否为空作为环境变量结束标记
    for(i=0;environ[i];i++)
    {
        WYXB::g_envneedmem += strlen(environ[i]) + 1;
    }
    
    WYXB::g_os_argc = argc; //保存参数个数
    WYXB::g_os_argv = (char**)argv; //保存原始命令行参数数组
    
    //全局变量必要初始化
    WYXB::my_log.fd = -1; //-1：表示日志文件尚未打开；因为后边ngx_log_stderr要用所以这里先给-1
    WYXB::ngx_process = NGX_PROCESS_MASTER; //本进程类型：主进程
    WYXB::ngx_reap = 0; //标记子进程状态变化
    
    WYXB::MyConf* myconf = WYXB::MyConf::getInstance(); //配置文件对象
    if(myconf->LoadConf("nginx.conf") == false)//把配置文件内容载入到内存
    {
        WYXB::ngx_log_init();    //初始化日志
        WYXB::ngx_log_stderr(0,"配置文件[%s]载入失败，退出!","nginx.conf");
        //exit(1);终止进程，在main中出现和return效果一样 ,exit(0)表示程序正常, exit(1)/exit(-1)表示程序异常退出，exit(2)表示表示系统找不到指定的文件
        exitcode = 2; //标记找不到文件
        goto lblexit;
    }
    WYXB::CMemory::getInstance();
    WYXB::CRC32::getInstance();
    WYXB::ngx_log_init();             //日志初始化(创建/打开日志文件)，这个需要配置项，所以必须放配置文件载入的后边；     
    
    if(WYXB::ngx_init_signals() != 0) //信号初始化
    {
        exitcode = 1;
        goto lblexit;
    }
    if(WYXB::g_socket.Initialize() == false)
    {
        exitcode = 1;
        goto lblexit;
    }

    WYXB::ngx_init_setproctitle();    //把环境变量搬家
    
    //--------------------------------------
    //创建守护进程
    if(myconf->GetIntDefault("Daemon",0) == 1)//读配置文件，拿到配置文件中是否按守护进程方式启动的选项
    {
        //1：按照守护进程方式运行
        int cdaemonresult = WYXB::ngx_daemon();
        if(cdaemonresult == -1) //fork()失败
        {
            exitcode = 1;    //标记失败
            goto lblexit;
        }
        if(cdaemonresult == 1)
        {
            WYXB::freeresource();   //只有进程退出了才goto到 lblexit，用于提醒用户进程退出了
                              //而我现在这个情况属于正常fork()守护进程后的正常退出，不应该跑到lblexit()，接下来交给守护进程
            exitcode = 0;
            return exitcode;  //整个进程直接在这里退出
        }
        WYXB::g_daemonized = 1;    //守护进程标记，标记是否启用了守护进程模式，0：未启用，1：启用了
    }
    WYXB::ngx_master_process_cycle(); //不管父进程还是子进程，正常工作期间都在这个函数里循环；

lblexit:
    //(5)该释放的资源要释放掉
    if(WYXB::ngx_process == NGX_PROCESS_MASTER)
        WYXB::ngx_log_stderr(0,"程序退出，再见了!");
    WYXB::freeresource();  //一系列的main返回前的释放动作函数
    //printf("程序退出，再见!\n");    
    return exitcode;
}

namespace WYXB
{

void freeresource()
{
    //(1)对于因为设置可执行程序标题导致的环境变量分配的内存，我们应该释放
    if(gp_envmem)
    {
        delete []gp_envmem;
        gp_envmem = NULL;
    }

    //(2)关闭日志文件
    if(WYXB::my_log.fd != STDERR_FILENO && WYXB::my_log.fd != -1)  
    {        
        close(WYXB::my_log.fd); //不用判断结果了
        WYXB::my_log.fd = -1; //标记下，防止被再次close吧        
    }
}

}
