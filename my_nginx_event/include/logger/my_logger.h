#pragma once
#include <sys/types.h>
#include <cstdarg>
namespace WYXB
{
// 日志
typedef struct
{
    int LogLevel;
    int fd; // 日志文件描述符
}my_log_t;

class Logger
{


public:
    // 日志相关函数
    static void   ngx_log_init();
    static void   ngx_log_stderr(int err, const char *fmt, ...);
    static void   ngx_log_error_core(int level,  int err, const char *fmt, ...);
    static void   ngx_log_close();


private:
    static u_char *ngx_log_errno(u_char *buf, u_char *last, int err);

private:
    static u_char *ngx_sprintf_num(u_char* buf, u_char* last, u_int64_t ui64, u_char zero, u_int64_t hexadecimal, u_int64_t width);
    static u_char *ngx_snprintf(u_char *buf, size_t max, const char *fmt, ...);
    static u_char *ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...);
    static u_char *ngx_vslprintf(u_char *buf, u_char *last,const char *fmt,va_list args);

    static my_log_t my_log;
};




}