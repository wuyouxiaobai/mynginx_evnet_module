#include "my_macro.h"
#include "my_global.h"
#include <string.h>
#include "my_macro.h"
#include "my_conf.h"
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>


namespace WYXB
{
//全局变量----------------------
//错误等级，与my_macro.h中的定义相同
static u_char err_level[][20] =
{
    {"stderr"},    //0：控制台错误
    {"emerg"},     //1：紧急
    {"alert"},     //2：警戒
    {"crit"},      //3：严重
    {"error"},     //4：错误
    {"warn"},      //5：警告
    {"notice"},    //6：注意
    {"info"},      //7：信息
    {"debug"}      //8：调试
};

my_log_t my_log;

// 控制台错误，最高等级
void ngx_log_stderr(int err, const char *fmt, ...)
{
    va_list args;
    u_char errstr[NGX_MAX_ERROR_STR + 1];
    u_char *p, *last;

    memset(errstr, 0, sizeof(errstr));

    last = errstr + NGX_MAX_ERROR_STR;

    p = ngx_cpymem(errstr, "nginx: ", 7);
    va_start(args, fmt); 
    p = ngx_vslprintf(p, last, fmt, args); // 组合出字符串保存在errstr里
    va_end(args);

    if(err) // 如果错误代码不是0，有错误发生
    {
       p = ngx_log_errno(p, last, err);
    }
    //如果位置不够，换行也要插入末尾，哪怕覆盖其他内容
    if(p >= (last - 1))
    {
        p = (last - 1) - 1;
    }
    *p++ = '\n';

    // 往标准错误【一般指屏幕】输出信息
    write(STDERR_FILENO, errstr, p - errstr);

    if(my_log.fd > STDERR_FILENO) // 如果日志文件打开了，就把信息写入日志文件
    {
        err = 0; //不要再次把错误信息弄到字符串里，否则字符串里重复了
        p--; // 回退到换行符
        *p = 0; // 把换行符替换成0，以便写入日志文件
        ngx_log_error_core(NGX_LOG_STDERR, err, (const char*)errstr);
    }
    




} 


// 给一段内存，一个错误编号，组合出一个字符串，如：【错误编号：错误原因】，放到给的内存中
// buf: 给的内存
// last: 放的数据不要超过这里
// err: 错误编号，取得这个错误编号对应的错误字符串，保存到buffer中
u_char* ngx_log_errno(u_char* buf, u_char* last, int err)
{
    char* errstr = strerror(err); // 取得错误字符串
    size_t len = strlen(errstr); // 取得错误字符串长度

    // 插入字符串
    char leftstr[10] = {0};
    sprintf(leftstr, " (%d: ", err);
    size_t leftlen = strlen(leftstr);

    char rightstr[] = ") ";
    size_t rightlen = strlen(rightstr);

    size_t extralen = leftlen + rightlen; // 左右的额外宽度
    if((buf + len + extralen) < last) // 错误消息：（错误编号：错误原因）
    {
        buf = ngx_cpymem(buf, leftstr, leftlen);
        buf = ngx_cpymem(buf, errstr, len);
        buf = ngx_cpymem(buf, rightstr, rightlen);
    
    }
    return buf;
}


//往日志文件中写日志，代码中有自动加换行符，所以调用时字符串不用刻意加\n；
//日志过定性为标准错误，直接往屏幕上写日志【比如日志文件打不开，则会直接定位到标准错误，此时日志打印到屏幕上】
//level:一个等级数字，我们把日志分成一些等级，以方便管理、显示、过滤等等，如果这个等级数字比配置文件中的等级数字"LogLevel"大，那么该条信息不被写到日志文件中
//err：是个错误代码，如果不是0，就应该转换成显示对应的错误信息,一起写到日志文件中，
//ngx_log_error_core(5,8,"这个XXX工作的有问题,显示的结果是=%s","YYYY");
void ngx_log_error_core(int level, int err, const char *fmt, ...)
{
    u_char *last;
    u_char errstr[NGX_MAX_ERROR_STR + 1];

    memset(errstr, 0, sizeof(errstr));
    last = errstr + NGX_MAX_ERROR_STR;

    struct timeval tv;
    struct tm tm;
    time_t sec;
    u_char *p; // 指向要保存数据大目标内存
    va_list args;

    memset(&tm, 0, sizeof(tm));
    memset(&tv, 0, sizeof(tv));

    gettimeofday(&tv, NULL); // 获取当前时间，返回自1970-01-01 00:00:00到现在经历的秒数【第二个参数是时区，一般不关心】    

    sec = tv.tv_sec;
    localtime_r(&sec, &tm);    //把参数1的time_t转换为本地时间，保存到参数2中去，带_r的是线程安全的版本，尽量使用
    tm.tm_mon++;
    tm.tm_year += 1900;

    u_char strcurrtime[40] = {0}; //组合出当前时间字符串，格式：2019-08-12 10:10:10
    ngx_slprintf(strcurrtime,
                (u_char*)-1,
                "%4d-%02d-%02d %02d:%02d:%02d",
                tm.tm_year,tm.tm_mon,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);

    p = ngx_cpymem(errstr, strcurrtime, strlen((const char*)strcurrtime));
    p = ngx_slprintf(p, last, " [%s] ", err_level[level]);
    p = ngx_slprintf(p, last, "%P: ", ngx_pid); //  2019-01-08 20:50:15 [crit] 2037:

    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args); 
    va_end(args);

    if(err)
    {
        p = ngx_log_errno(p, last, err);
    }
    if(p >= (last - 1))
    {
        p = (last - 1) - 1;
    }
    *p++ = '\n';
    // 2024-10=22 16:53:13 [notice] 17113:

    size_t n;
    while(1)
    {
        if(level > my_log.LogLevel)
        {
            break; // 打印的日志等级高于配置文件设置登记，就不打印
        }
        // 写日志文件
        n = write(my_log.fd, errstr, p - errstr);
        if(n == -1)
        {
            if(errno == ENOSPC) //磁盘没空间，写失败
            {
                // 暂时什么也 不做
            }
            else
            {
                // 其他错误，考虑把错误信息显示到标准错误设备
                if(my_log.fd != STDERR_FILENO)
                {
                    n = write(STDERR_FILENO, errstr, p - errstr);
                }
            }
        }
        break;
    }

}



//描述：日志初始化，初始化日志
void ngx_log_init() {  
    u_char* plogname = NULL;  

    // 从配置文件读取与日志相关配置信息  
    MyConf* config = MyConf::getInstance();  
    plogname = (u_char*)config->GetString("Log");  

    // 获取可执行文件所在目录  
    char exePath[PATH_MAX];  
    ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX);  
    if (count == -1) {  
        ngx_log_stderr(errno, "[alert] could not get executable path", "");  
        my_log.fd = STDERR_FILENO;  
        return;  
    }  
    exePath[count] = '\0'; // 确保字符串以 '\0' 结尾  

    // 提取可执行文件所在目录  
    std::string exeDir = std::string(exePath).substr(0, std::string(exePath).find_last_of('/'));  

    // 构造日志目录路径（bin 同级的 log 目录）  
    std::string logDir = exeDir + "/../log";  

    // 创建 log 目录（如果不存在）  
    struct stat st;  
    if (stat(logDir.c_str(), &st) == -1) {  
        if (mkdir(logDir.c_str(), 0755) == -1) {  
            ngx_log_stderr(errno, "[alert] could not create log directory", logDir.c_str());  
            my_log.fd = STDERR_FILENO;  
            return;  
        }  
    }  

    // 如果配置文件没有指定日志路径，使用默认路径  
    std::string logFilePath;  
    if (plogname == NULL) {  
        logFilePath = logDir + "/" + NGX_ERROR_LOG_PATH; // 默认日志文件名  
    } else {  
        logFilePath = logDir + "/" + (const char*)plogname;  
    }  

    // 设置日志等级  
    my_log.LogLevel = config->GetIntDefault("LogLevel", 6); // 默认日志等级为 6 (NGX_LOG_NOTICE)  

    // 打开日志文件  
    my_log.fd = open(logFilePath.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);  
    if (my_log.fd == -1) {  
        ngx_log_stderr(errno, "[alert] could not open error log file", logFilePath.c_str());  
        my_log.fd = STDERR_FILENO; // 打开失败则写入标准错误  
    }  
}  
}