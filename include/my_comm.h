#ifndef __NGX_COMM_H__
#define __NGX_COMM_H__

#define _PKG_MAX_LENGTH 30000 // 每个包的最大长度【包头+包体】

// 通信 收包状态定义
#define _PKG_HD_INIT 0 // 初始状态，准备接收数据包头
#define _PKG_HD_RECVING 1 // 接收包头中
#define _PKG_BD_INIT 2 // 包头接受完，接受包体
#define _PKG_BD_RECVING 3 // 接收包体

#define _DATA_BUFSIZE_ 20 // 包头缓冲区大小

// 结构定义
#pragma pack (1) // 对齐方式，1字节对齐【结构之间成员不做任何字节对齐，紧密排列在一起】

// 网络通信相关结构
// 包头结构
typedef struct COMM_PKG_HEADER
{
    unsigned short pkgLen; // 报文总长度【包头+包体】
    unsigned short msgCode; // 消息类型
    int crc32; // 数据效验
}*LPCOMM_PKG_HEADER;

#pragma pack() // 取消指定对齐，恢复缺省对齐

#endif