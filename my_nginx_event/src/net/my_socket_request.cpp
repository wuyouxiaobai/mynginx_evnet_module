#include "my_socket.h"
#include "my_func.h"
#include "my_memory.h"
#include <arpa/inet.h>
#include <cstring>
#include "my_global.h"


namespace WYXB
{



void CSocket::ngx_read_request_handler(lpngx_connection_t pConn){} //设置数据来时的读回调函数

void CSocket::ngx_write_response_handler(lpngx_connection_t pConn){}


}