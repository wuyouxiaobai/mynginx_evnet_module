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
#include "my_logger.h"


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

WYXB::my_log_t WYXB::Logger::my_log = {NGX_LOG_INFO, -1};

// 控制台错误，最高等级
void Logger::ngx_log_stderr(int err, const char *fmt, ...)
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
u_char* Logger::ngx_log_errno(u_char* buf, u_char* last, int err)
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
void Logger::ngx_log_error_core(int level, int err, const char *fmt, ...)
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
void Logger::ngx_log_init() {  
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



// 关闭日志文件
void Logger::ngx_log_close()
{
    //关闭日志文件
    if(my_log.fd != STDERR_FILENO && my_log.fd != -1)  
    {        
        close(my_log.fd); //不用判断结果了
        my_log.fd = -1; //标记下，防止被再次close吧        
    }
}









//以一个指定的宽度把一个数字显示在buf对应内存，如果实际显示数字位数比指定宽度小，比如【指定显示宽度10位，而实际显示“123456”，结果会显示“    123456”】，则会在左侧补空格。
     //当然如果你不指定宽度【参数width=0】，则按实际宽度显示
     //你给进来一个%Xd之类的，还能以十六进制数字格式显示出来
//buf：数据存放内存
//last：buf的最后一个字节的地址
//ui64：待显示的数字
//zero：【一般显示的数字位数不足要求的，则用这个字符填充】，比如要显示10位，而实际只有7位，则后边填充3个这个字符；
//hexadecimal：是否显示成十六进制数字 0：不
//width：指定显示宽度
u_char *Logger::ngx_sprintf_num(u_char* buf, u_char* last, u_int64_t ui64, u_char zero, u_int64_t hexadecimal, u_int64_t width)
{

    u_char *p, temp[NGX_INT64_LEN + 1];
    size_t len;
    u_int32_t ui32;

    static u_char   hex[] = "0123456789abcdef";  //跟把一个10进制数显示成16进制有关，换句话说和  %xd格式符有关，显示的16进制数中a-f小写
    static u_char   HEX[] = "0123456789ABCDEF";  //跟把一个10进制数显示成16进制有关，换句话说和  %Xd格式符有关，显示的16进制数中A-F大写

    p = temp + NGX_INT64_LEN; // temp的最后一个字节的地址

    if(hexadecimal == 0)
    {
        if(ui64 <= (u_int64_t)NGX_MAX_UINT32_VALUE)
        {
            ui32 = (u_int32_t)ui64;
            do
            {
                *--p = (u_char)(ui32 % 10 + '0');
            }
            while(ui32 /= 10); // 从后往前一次插入给定数字
        }
        else
        {
            do
            {
                *--p = (u_char)(ui64 % 10 + '0');
            }
            while(ui64 /= 10); // 从后往前一次插入给定数字
        }
    }
    else if(hexadecimal == 1)// 显示十六进制数字，小写格式
    {
        do
        {
            *--p = hex[ui64 & 0xf];
        } while (ui64 >>= 4);
        
    }
    else // 大写格式
    {
        do
        {
            *--p = HEX[ui64 & 0xf];
        } while (ui64 >>= 4);
        
    }

    len = (temp + NGX_INT64_LEN - p); // 实际显示的数字位数

    // 给空缺位置填指定的zero字符
    while (len++ < width && buf < last)
    {
        *buf++ = zero;
    }

    len = (temp + NGX_INT64_LEN - p); 

    if((buf + len) >= last)
    {
        len = last - buf;
    }
    // 复制实际显示的数字到buf
    return ngx_cpymem(buf, p, len);

    
}

//将nginx自定义的数据结构进行标准格式输出
//buf：数据存放内存
//last：数据存放下限
//fmt：格式字符【fmt = "invalid option: \"%s\",%d"】
//args：格式参数【args = "testinfo",123】
//支持格式： %d【%Xd/%xd】:数字,  %s:字符串，%f：浮点, %P：pid_t

u_char *Logger::ngx_vslprintf(u_char* buf, u_char* last, const char* fmt, va_list args)
{
    u_char zero;

    uintptr_t width,sign,hex,frac_width,scale,n;

    int64_t i64; // 保存%d对应的可变参数
    uint64_t ui64; // 保存%ud对应的可变参数，临时作为%f的整数部分也可以
    u_char *p; // 保存%s对应的可变参数
    double f; // 保存%f对应的可变参数
    uint64_t frac; // %f可变参数根据%.2等，取得小数部分的2位后的内容

    while(*fmt && buf < last) // 每次处理一个字符
    {
        if(*fmt == '%')
        {
            //++fmt是先加后用，也就是fmt先往后走一个字节位置，然后再判断该位置的内容
            zero  = (u_char) ((*++fmt == '0') ? '0' : ' ');  //判断%后边接的是否是个'0',如果是zero = '0'，否则zero = ' '，一般比如你想显示10位，而实际数字7位，前头填充三个字符，就是这里的zero用于填充
                                                                //ngx_log_stderr(0, "数字是%010d", 12); 
                                                                
            width = 0; //格式字符% 后边如果是个数字，这个数字最终会弄到width里边来 ,这东西目前只对数字格式有效，比如%d,%f这种
            sign  = 1; //显示的是否是有符号数，这里给1，表示是有符号数，除非你 用%u，这个u表示无符号数 
            hex   = 0; //是否以16进制形式显示(比如显示一些地址)，0：不是，1：是，并以小写字母显示a-f，2：是，并以大写字母显示A-F
            frac_width = 0; //小数点后位数字，一般需要和%.10f配合使用，这里10就是frac_width；
            i64 = 0; //一般用%d对应的可变参中的实际数字，会保存在这里
            ui64 = 0; //一般用%ud对应的可变参中的实际数字，会保存在这里

            // 初始化结束
            // 通过while判断%后是否是一个数字，如果是数字【比如%16】就取出放到width
            while(*fmt >= '0' && *fmt <= '9')
            {
                width = width * 10 + *fmt++ - '0'; // 取出数字，并累加到width里边
            }

            for(;;) // 匹配特殊格式
            {
                switch(*fmt)
                {
                case 'u': // 无符号
                    sign = 0; // 显示无符号数
                    fmt++;
                    continue;
                case 'X': // 大写十六进制，一般是%Xd
                    sign = 0;
                    hex = 2; // 显示大写十六进制
                    fmt++;
                    continue;
                case 'x': // 小写十六进制，一般是%xd
                    sign = 0;
                    hex = 1; // 显示小写十六进制
                    fmt++;
                    continue;
                case '.': // 小数点，必须是%.2f这种，记录了保留到小数点多少位
                    fmt++;
                    while(*fmt >= '0' && *fmt <= '9')
                    {
                        frac_width = frac_width * 10 + *fmt++ - '0'; // 取出数字，并累加到frac_width里边
                    }
                    break;;
                default:
                    break;
                }
                break;
            }

            switch(*fmt)
            {
            case '%': // 打印%
                *buf++ = '%';
                fmt++;
                continue;//回到开头继续判断
            case 'd':
                if(sign)
                {
                    // 有符号
                    i64 = (int64_t)va_arg(args, int64_t); // 取出%d对应的可变参数
                }
                else
                {
                    // 无符号
                    ui64 = (uint64_t)va_arg(args, uint64_t); // 取出%ud对应的可变参数
                
                }
                break;//这break掉，直接跳道switch后边的代码去执行,这种凡是break的，都不做fmt++;  *********************【switch后仍旧需要进一步处理】
            case 'i': //转换ngx_int_t型数据，如果用%ui，则转换的数据类型是ngx_uint_t  
                if(sign)
                {
                    i64 = (int64_t)va_arg(args, intptr_t);
                }
                else
                {
                    ui64 = (uint64_t)va_arg(args, uintptr_t);
                }
                break;
            case 'L'://转换int64j型数据，如果用%uL，则转换的数据类型是uint64 t
                if (sign)
                {
                    i64 = va_arg(args, int64_t);
                } 
                else 
                {
                    ui64 = va_arg(args, uint64_t);
                }
                break;
            case 'p': // 输出指针
                ui64 = (uintptr_t)va_arg(args, void*); // 取出%p对应的可变参数
                hex = 2; // 显示大写十六进制
                sign = 0; // 显示无符号数
                zero = '0'; // 显示前缀0
                width = sizeof(void*) * 2; // 显示指针大小，一般是8字节，所以这里显示16进制
                break;
            case 's': // 输出字符串
                p = va_arg(args, u_char*);// 取出%s对应的可变参数
                while(*p && buf < last)
                {
                    *buf++ = *p++; // 复制字符串到buf
                }
                fmt++;
                continue;
            case 'P':
                i64 = (int64_t)va_arg(args, pid_t); // 取出%P对应的可变参数
                sign = 1; // 显示有符号数
                break;
            case 'f': // 输出浮点数
                f = va_arg(args, double); // 取出%f对应的可变参数
                if(f < 0) // 负数
                {
                    *buf++ = '-';
                    f = -f;
                }
                ui64 = (uint64_t)f; // 取整数部分
                frac = 0;
                // 求小数点后显示多少位
                if(frac_width) // %d.2f，就表示显示2位
                {
                    scale = 1;
                    for(n = 0; n < frac_width; n++)
                    {
                        scale *= 10;
                    }
                    frac = (uint64_t)((f - (double)ui64) * scale + 0.5); // 取小数部分，并四舍五入
                    if(frac >= scale) // 出现了进位，比如0.9
                    {
                        ui64++;
                        frac = 0;
                    }
                
                }

                // 输出整数部分
                buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width);

                // 输出小数点
                if(frac_width)
                {
                    if(buf < last)
                    {
                        *buf++ = '.';
                    }
                    buf = ngx_sprintf_num(buf, last, frac, '0', 0, frac_width); // 输出小数部分
                
                }
                fmt++;
                continue;
            default:
                *buf++ = *fmt++; // 复制%后边的字符到buf
                continue;
            }

            //显示%d的，会走下来，其他走下来的格式日后逐步完善......
            if(sign)
            { // 显示有符号数$0
                if(i64 < 0)
                {
                    *buf++ = '-';
                    ui64 = (uint64_t)-i64;
                }
                else
                {
                    ui64 = (uint64_t)i64;
                
                }
            }

            // 输出数字
            buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width);
            fmt++;
        }
        else // 正常字符
        {
            *buf++ = *fmt++; // 复制字符到buf
        }
    }

    return buf;
}

// 封装ngx_vslprintf函数
u_char *Logger::ngx_slprintf(u_char* buf, u_char* last, const char* fmt, ...)
{
    va_list args;
    u_char *p;
    va_start(args, fmt);
    p = ngx_vslprintf(buf, last, fmt, args);
    va_end(args);
    return p;
}

u_char *Logger::ngx_snprintf(u_char* buf, size_t max, const char* fmt, ...)
{
    u_char *p;
    va_list args;
    va_start(args, fmt);
    p = ngx_vslprintf(buf, buf + max, fmt, args);
    va_end(args);
    return p;
}


}