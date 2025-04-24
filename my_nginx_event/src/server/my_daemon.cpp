#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "my_server.h"


namespace WYXB
{
int Server::ngx_daemon()
{

    switch(fork())
    {
    case -1: 
    Logger::ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中fork()失败!");
        return -1;
    case 0: 
        break;
    default: 
        return 1;
    
    }
    ngx_parent = ngx_pid.load(); 
    ngx_pid = getpid(); 
    

    if(setsid() == -1)
    {
        Logger::ngx_log_error_core(NGX_LOG_EMERG, errno, "ngx_daemon()中setsid()失败!");
        return -1;
    
    }

    umask(0);


    int fd = open("/dev/null", O_RDWR);
    if (fd == -1)
    {
        Logger::ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中open(\"/dev/null\")失败!");        
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) 
    {
        Logger::ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中dup2(STDIN)失败!");        
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) 
    {
        Logger::ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中dup2(STDOUT)失败!");
        return -1;
    }
    if (fd > STDERR_FILENO)  
    {
        if (close(fd) == -1)  
        {
            Logger::ngx_log_error_core(NGX_LOG_EMERG,errno, "ngx_daemon()中close(fd)失败!");
            return -1;
        }
    }
    return 0; 

}




}