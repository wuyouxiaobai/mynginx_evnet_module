#pragma once

#include <sys/types.h>
#include <stdarg.h>

namespace WYXB
{
// 日志相关函数
void   ngx_log_init();
void   ngx_log_stderr(int err, const char *fmt, ...);
void   ngx_log_error_core(int level,  int err, const char *fmt, ...);
u_char *ngx_log_errno(u_char *buf, u_char *last, int err);
u_char *ngx_snprintf(u_char *buf, size_t max, const char *fmt, ...);
u_char *ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...);
u_char *ngx_vslprintf(u_char *buf, u_char *last,const char *fmt,va_list args);


//设置可执行程序标题相关函数
void   ngx_init_setproctitle();
void   ngx_setproctitle(const char *title);


//子进程循环监听epoll消息相关函数
void ngx_process_events_and_timers();
void ngx_master_process_cycle();

//信号相关
int ngx_init_signals();

//守护进程
int ngx_daemon();

}



