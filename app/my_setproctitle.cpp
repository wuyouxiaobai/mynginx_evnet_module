#include "my_global.h"
#include <string.h>



namespace WYXB
{
// 设置可执行程序标题相关函数：分配内存，并把环境变量拷贝到新内存
void ngx_init_setproctitle()
{
    //这里无需判断penvmen == NULL,有些编译器new会返回NULL，有些会报异常，但不管怎样，如果在重要的地方new失败了，你无法收场，让程序失控崩溃，助你发现问题为好； 
    gp_envmem = new char[g_envneedmem];
    memset(gp_envmem, 0, g_envneedmem);

    char* ptmp = gp_envmem;
    // 把原来环境变量内容拷贝到新内存
    for(int i = 0; environ[i]; i++)
    {
        size_t len = strlen(environ[i]) + 1; // 包括后续的\0
        strcpy(ptmp, environ[i]);
        environ[i] = ptmp;
        ptmp += len;
    }
    return;

}


// 设置可执行程序标题
void ngx_setproctitle(const char* title)
{
    // 计算新标题长度
    size_t ititlelen = strlen(title); 

    // 计算原始argv内存的总长度【包括各种参数】
    size_t esy = g_argvneedmem + g_envneedmem;
    if(esy <= ititlelen)
    {
        return;
    }

    g_os_argv[1] = NULL;

    // 将标题存入
    char* ptmp = g_os_argv[0];
    strcpy(ptmp, title);
    ptmp += ititlelen;

    //把剩余的原argv以及environ所占的内存全部清0，否则会出现在ps的cmd列可能还会残余一些没有被覆盖的内容
    size_t cha = esy - ititlelen;
    memset(ptmp, 0, cha);
    

}





}