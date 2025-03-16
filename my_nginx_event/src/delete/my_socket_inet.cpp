// #include "my_socket.h"
// #include <netinet/in.h>
// #include "my_func.h"


// namespace WYXB
// {
// // 将socket绑定的地址转换成文本格式
// // 参数sa：客户端ip地址信息
// // 参数port：1，表示把端口信息放到组合成的字符串；0，不会包含端口信息
// // 参数text：文本
// // 参数len：文本宽度
//  // 获得对端信息相关
// size_t CSocket::ngx_sock_ntop(struct sockaddr *sa, int port, u_char *text, size_t len) // 根据参数1获得对端信息，获得地址端口字符串，返回这个字符串长度
// {
//     struct sockaddr_in *sin;
//     u_char *p;

//     switch(sa->sa_family)
//     {
//     case AF_INET:
//         sin = (struct sockaddr_in *)sa;
//         p = (u_char *)&sin->sin_addr;
//         if(port) // 包含端口信息
//         {
//             p = ngx_snprintf(text, len, "%ud.%ud.%ud.%ud:%d", p[0], p[1], p[2], p[3], ntohs(sin->sin_port)); 
//         }
//         else
//         {
//             p = ngx_snprintf(text, len, "%ud.%ud.%ud.%ud", p[0], p[1], p[2], p[3]); 
//         }
//         return (p - text);
//         break;
//     default:
//         return 0;
//         break;
       



//     }
//     return 0;


// }





// }