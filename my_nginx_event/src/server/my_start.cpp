#include "my_server.h"

namespace WYXB
{
std::atomic<int> Server::g_stopEvent{0};
std::atomic<int> Server::ngx_pid{-1}; //当前进程
std::atomic<int> Server::ngx_parent{-1}; //父进程

int Server::run(int argc, char*const *argv)
{
    int exitcode = 0;           //退出代码，先给0表示正常退出
    int i;                      //临时用
    
    g_stopEvent = 0;            //标记程序是否退出，0不退出    
    ngx_terminate = 0;          ////标记程序是否要优雅退出，0不终止      
    
    ngx_pid = getpid();         //获取当前进程的PID
    ngx_parent = getppid();      //获取父进程的PID
    ngx_last_process = 0;      //工作进程数量
    
    //统计argc所占内存
    g_argvneedmem = 0;
    for(i=0;i<argc;i++) //argv =  ./nginx -a -b -c asdfas
    {
        g_argvneedmem += strlen(argv[i]) + 1; //+1是为了保存每个参数的结束符'\0'
    }
    //统计环境变量所占的内存。注意判断方法是environ[i]是否为空作为环境变量结束标记
    for(i=0;environ[i];i++)
    {
        g_envneedmem += strlen(environ[i]) + 1;
    }
    
    g_os_argc = argc; //保存参数个数
    g_os_argv = (char**)argv; //保存原始命令行参数数组
    
    //全局变量必要初始化
    // my_log.fd = -1; //-1：表示日志文件尚未打开；因为后边ngx_log_stderr要用所以这里先给-1
    ngx_process = NGX_PROCESS_MASTER; //本进程类型：主进程
    ngx_reap = 0; //标记子进程状态变化
    
    MyConf* myconf = MyConf::getInstance(); //配置文件对象
    if(myconf->LoadConf("nginx.conf") == false)//把配置文件内容载入到内存
    {
        Logger::ngx_log_init();    //初始化日志
        Logger::ngx_log_stderr(0,"配置文件[%s]载入失败，退出!","nginx.conf");
        //exit(1);终止进程，在main中出现和return效果一样 ,exit(0)表示程序正常, exit(1)/exit(-1)表示程序异常退出，exit(2)表示表示系统找不到指定的文件
        exitcode = 2; //标记找不到文件
        goto lblexit;
    }
    // CMemory::getInstance();
    // CRC32::getInstance();
    Logger::ngx_log_init();             //日志初始化(创建/打开日志文件)，这个需要配置项，所以必须放配置文件载入的后边；     
    
    // if(ngx_init_signals() != 0) //信号初始化
    // {
    //     exitcode = 1;
    //     goto lblexit;
    // }
    // if(Server::instance().g_socket->Initialize() == false)
    // {
    //     exitcode = 1;
    //     goto lblexit;
    // }

    ngx_init_setproctitle();    //把环境变量搬家
    
    //--------------------------------------
    //创建守护进程
    if(myconf->GetIntDefault("Daemon",0) == 1)//读配置文件，拿到配置文件中是否按守护进程方式启动的选项
    {
        //1：按照守护进程方式运行
        int cdaemonresult = ngx_daemon();
        if(cdaemonresult == -1) //fork()失败
        {
            exitcode = 1;    //标记失败
            goto lblexit;
        }
        if(cdaemonresult == 1)
        {
            freeresource();   //只有进程退出了才goto到 lblexit，用于提醒用户进程退出了
                              //而我现在这个情况属于正常fork()守护进程后的正常退出，不应该跑到lblexit()，接下来交给守护进程
            exitcode = 0;
            return exitcode;  //整个进程直接在这里退出
        }
        g_daemonized = 1;    //守护进程标记，标记是否启用了守护进程模式，0：未启用，1：启用了
    }
    ngx_master_process_cycle(); //不管父进程还是子进程，正常工作期间都在这个函数里循环；

lblexit:

    if(ngx_process == NGX_PROCESS_MASTER)
        Logger::ngx_log_stderr(0,"程序退出，再见了!");
    freeresource(); 
 
    return exitcode;
}
}