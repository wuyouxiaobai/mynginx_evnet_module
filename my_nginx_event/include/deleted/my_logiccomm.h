// #pragma once

// // 收发命令宏定义

// #define _CMD_START 0
// #define _CMD_PING _CMD_START + 0 // ping命令【心跳包】
// #define _CMD_REGISTER _CMD_START + 5 // 注册
// #define _CMD_LOGIN _CMD_START + 6 // 登录


// // 结构定义
// #pragma pack (1)

// typedef struct STRUCT_REGISTER
// {
//     int iType; // 类型
//     char username[56]; //用户名
//     char password[40]; //密码
//     bool bIsSuccess;

// }*LPSTRUCT_REGISTER;


// typedef struct STRUCT_LOGIN
// {
//     char username[56]; // 用户名
//     char password[40]; //密码
//     bool bIsSuccess;
// }*LPSTRUCT_LOGIN;

// #pragma pack()