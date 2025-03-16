#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "my_server.h"


namespace WYXB
{
//描述：守护进程初始化
//执行失败：返回-1，   子进程：返回0，父进程：返回1
int Server::ngx_daemon()
{
    // (1)创建守护进程第一步，fork()一个子进程
    switch(fork())
    {
    case -1: // fork()失败
    Logger::ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中fork()失败!");
        return -1;
    case 0: // 子进程
        break;
    default: // 父进程
        return 1;
    
    }
    ngx_parent = ngx_pid; // 记录父进程ID
    ngx_pid = getpid(); // 记录子进程ID
    
    // (2)脱离终端，终端关闭，此进程成为守护进程
    // 创建会话，并让本进程成为会话首领
    if(setsid() == -1)
    {
        Logger::ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中setsid()失败!");
        return -1;
    
    }
    
    // （3）将文件或目录的实际权限减去umask设置掩码得到进程对文件或目录的实际权限
    umask(0);// 0表示不设置掩码，拥有文件或目录本身的所有权限


    // （4）打开黑洞设备
    // 当进程希望不再处理某些输出时，可以将输出重定向到 /dev/null，这样任何写入这个文件的数据都会被忽略掉。例如，在守护进程中，常常希望不输出到终端。
    int fd = open("/dev/null", O_RDWR);
    if (fd == -1)
    {
        Logger::ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中open(\"/dev/null\")失败!");        
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) //这一步将标准输入重定向到 /dev/null。因此，任何对标准输入的读取操作都将从 /dev/null 中读取数据，而不会等待用户的输入或返回任何有效数据。这样做是为了确保守护进程不会因试图从终端读取用户输入而阻塞。
    {
        Logger::ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中dup2(STDIN)失败!");        
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) //这一步将标准输出也重定向到 /dev/null。任何写入到标准输出的内容都将被丢弃，而不会显示在终端。这样做是为了防止守护进程产生任何输出，不管是日志、错误信息还是其他信息，因为在后台运行时，这些信息通常并不需要输出到终端。
    {
        Logger::ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中dup2(STDOUT)失败!");
        return -1;
    }
    if (fd > STDERR_FILENO)  //fd应该是3，这个应该成立
    {
        if (close(fd) == -1)  //释放资源这样这个文件描述符就可以被复用；不然这个数字【文件描述符】会被一直占着；
        {
            Logger::ngx_log_error_core(NGX_LOG_EMERG,errno, "ngx_daemon()中close(fd)失败!");
            return -1;
        }
    }
    return 0; //子进程返回0

}




}