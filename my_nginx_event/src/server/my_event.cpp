#include "my_server.h"



namespace WYXB
{
// 处理网络事件和定时器事件，按照nginx引入同名函数
// 子进程反复调用，查看自己管理的epoll事件是否有发生的变化
// 这是nginxs的核心处理函数，处理惊群
void Server::ngx_process_events_and_timers()
{
    Server::instance().g_socket->ngx_epoll_process_events(5);

    //统计信息打印，考虑到测试的时候总会收到各种数据信息，所以上边的函数调用一般都不会卡住等待收数据
    // Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_process_events()中EPOLLIN事件发生，正常读事件后面2.");
    Server::instance().g_socket->printTDInfo();
    // Logger::ngx_log_stderr(errno,"CSocekt::ngx_epoll_process_events()中EPOLLIN事件发生，正常读事件后面3.");
    //...再完善
}


    
}