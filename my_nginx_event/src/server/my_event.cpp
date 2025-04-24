#include "my_server.h"



namespace WYXB
{

void Server::ngx_process_events_and_timers() {
    Server::instance().g_socket->ngx_epoll_process_events(5);
    Server::instance().g_socket->printTDInfo();
}



    
}