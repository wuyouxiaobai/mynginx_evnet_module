// #include <sys/types.h>
// #include "my_macro.h"
// #include "my_macro.h"
// #include <stdarg.h>
// #include <stdint.h>

// namespace WYXB {



// //以一个指定的宽度把一个数字显示在buf对应内存，如果实际显示数字位数比指定宽度小，比如【指定显示宽度10位，而实际显示“123456”，结果会显示“    123456”】，则会在左侧补空格。
//      //当然如果你不指定宽度【参数width=0】，则按实际宽度显示
//      //你给进来一个%Xd之类的，还能以十六进制数字格式显示出来
// //buf：数据存放内存
// //last：buf的最后一个字节的地址
// //ui64：待显示的数字
// //zero：【一般显示的数字位数不足要求的，则用这个字符填充】，比如要显示10位，而实际只有7位，则后边填充3个这个字符；
// //hexadecimal：是否显示成十六进制数字 0：不
// //width：指定显示宽度
// static u_char *ngx_sprintf_num(u_char* buf, u_char* last, u_int64_t ui64, u_char zero, u_int64_t hexadecimal, u_int64_t width)
// {

//     u_char *p, temp[NGX_INT64_LEN + 1];
//     size_t len;
//     u_int32_t ui32;

//     static u_char   hex[] = "0123456789abcdef";  //跟把一个10进制数显示成16进制有关，换句话说和  %xd格式符有关，显示的16进制数中a-f小写
//     static u_char   HEX[] = "0123456789ABCDEF";  //跟把一个10进制数显示成16进制有关，换句话说和  %Xd格式符有关，显示的16进制数中A-F大写

//     p = temp + NGX_INT64_LEN; // temp的最后一个字节的地址

//     if(hexadecimal == 0)
//     {
//         if(ui64 <= (u_int64_t)NGX_MAX_UINT32_VALUE)
//         {
//             ui32 = (u_int32_t)ui64;
//             do
//             {
//                 *--p = (u_char)(ui32 % 10 + '0');
//             }
//             while(ui32 /= 10); // 从后往前一次插入给定数字
//         }
//         else
//         {
//             do
//             {
//                 *--p = (u_char)(ui64 % 10 + '0');
//             }
//             while(ui64 /= 10); // 从后往前一次插入给定数字
//         }
//     }
//     else if(hexadecimal == 1)// 显示十六进制数字，小写格式
//     {
//         do
//         {
//             *--p = hex[ui64 & 0xf];
//         } while (ui64 >>= 4);
        
//     }
//     else // 大写格式
//     {
//         do
//         {
//             *--p = HEX[ui64 & 0xf];
//         } while (ui64 >>= 4);
        
//     }

//     len = (temp + NGX_INT64_LEN - p); // 实际显示的数字位数

//     // 给空缺位置填指定的zero字符
//     while (len++ < width && buf < last)
//     {
//         *buf++ = zero;
//     }

//     len = (temp + NGX_INT64_LEN - p); 

//     if((buf + len) >= last)
//     {
//         len = last - buf;
//     }
//     // 复制实际显示的数字到buf
//     return ngx_cpymem(buf, p, len);

    
// }

// //将nginx自定义的数据结构进行标准格式输出
// //buf：数据存放内存
// //last：数据存放下限
// //fmt：格式字符【fmt = "invalid option: \"%s\",%d"】
// //args：格式参数【args = "testinfo",123】
// //支持格式： %d【%Xd/%xd】:数字,  %s:字符串，%f：浮点, %P：pid_t

// u_char *ngx_vslprintf(u_char* buf, u_char* last, const char* fmt, va_list args)
// {
//     u_char zero;

//     uintptr_t width,sign,hex,frac_width,scale,n;

//     int64_t i64; // 保存%d对应的可变参数
//     uint64_t ui64; // 保存%ud对应的可变参数，临时作为%f的整数部分也可以
//     u_char *p; // 保存%s对应的可变参数
//     double f; // 保存%f对应的可变参数
//     uint64_t frac; // %f可变参数根据%.2等，取得小数部分的2位后的内容

//     while(*fmt && buf < last) // 每次处理一个字符
//     {
//         if(*fmt == '%')
//         {
//             //++fmt是先加后用，也就是fmt先往后走一个字节位置，然后再判断该位置的内容
//             zero  = (u_char) ((*++fmt == '0') ? '0' : ' ');  //判断%后边接的是否是个'0',如果是zero = '0'，否则zero = ' '，一般比如你想显示10位，而实际数字7位，前头填充三个字符，就是这里的zero用于填充
//                                                                 //ngx_log_stderr(0, "数字是%010d", 12); 
                                                                
//             width = 0; //格式字符% 后边如果是个数字，这个数字最终会弄到width里边来 ,这东西目前只对数字格式有效，比如%d,%f这种
//             sign  = 1; //显示的是否是有符号数，这里给1，表示是有符号数，除非你 用%u，这个u表示无符号数 
//             hex   = 0; //是否以16进制形式显示(比如显示一些地址)，0：不是，1：是，并以小写字母显示a-f，2：是，并以大写字母显示A-F
//             frac_width = 0; //小数点后位数字，一般需要和%.10f配合使用，这里10就是frac_width；
//             i64 = 0; //一般用%d对应的可变参中的实际数字，会保存在这里
//             ui64 = 0; //一般用%ud对应的可变参中的实际数字，会保存在这里

//             // 初始化结束
//             // 通过while判断%后是否是一个数字，如果是数字【比如%16】就取出放到width
//             while(*fmt >= '0' && *fmt <= '9')
//             {
//                 width = width * 10 + *fmt++ - '0'; // 取出数字，并累加到width里边
//             }

//             for(;;) // 匹配特殊格式
//             {
//                 switch(*fmt)
//                 {
//                 case 'u': // 无符号
//                     sign = 0; // 显示无符号数
//                     fmt++;
//                     continue;
//                 case 'X': // 大写十六进制，一般是%Xd
//                     sign = 0;
//                     hex = 2; // 显示大写十六进制
//                     fmt++;
//                     continue;
//                 case 'x': // 小写十六进制，一般是%xd
//                     sign = 0;
//                     hex = 1; // 显示小写十六进制
//                     fmt++;
//                     continue;
//                 case '.': // 小数点，必须是%.2f这种，记录了保留到小数点多少位
//                     fmt++;
//                     while(*fmt >= '0' && *fmt <= '9')
//                     {
//                         frac_width = frac_width * 10 + *fmt++ - '0'; // 取出数字，并累加到frac_width里边
//                     }
//                     break;;
//                 default:
//                     break;
//                 }
//                 break;
//             }

//             switch(*fmt)
//             {
//             case '%': // 打印%
//                 *buf++ = '%';
//                 fmt++;
//                 continue;//回到开头继续判断
//             case 'd':
//                 if(sign)
//                 {
//                     // 有符号
//                     i64 = (int64_t)va_arg(args, int64_t); // 取出%d对应的可变参数
//                 }
//                 else
//                 {
//                     // 无符号
//                     ui64 = (uint64_t)va_arg(args, uint64_t); // 取出%ud对应的可变参数
                
//                 }
//                 break;//这break掉，直接跳道switch后边的代码去执行,这种凡是break的，都不做fmt++;  *********************【switch后仍旧需要进一步处理】
//             case 'i': //转换ngx_int_t型数据，如果用%ui，则转换的数据类型是ngx_uint_t  
//                 if(sign)
//                 {
//                     i64 = (int64_t)va_arg(args, intptr_t);
//                 }
//                 else
//                 {
//                     ui64 = (uint64_t)va_arg(args, uintptr_t);
//                 }
//                 break;
//             case 'L'://转换int64j型数据，如果用%uL，则转换的数据类型是uint64 t
//                 if (sign)
//                 {
//                     i64 = va_arg(args, int64_t);
//                 } 
//                 else 
//                 {
//                     ui64 = va_arg(args, uint64_t);
//                 }
//                 break;
//             case 'p': // 输出指针
//                 ui64 = (uintptr_t)va_arg(args, void*); // 取出%p对应的可变参数
//                 hex = 2; // 显示大写十六进制
//                 sign = 0; // 显示无符号数
//                 zero = '0'; // 显示前缀0
//                 width = sizeof(void*) * 2; // 显示指针大小，一般是8字节，所以这里显示16进制
//                 break;
//             case 's': // 输出字符串
//                 p = va_arg(args, u_char*);// 取出%s对应的可变参数
//                 while(*p && buf < last)
//                 {
//                     *buf++ = *p++; // 复制字符串到buf
//                 }
//                 fmt++;
//                 continue;
//             case 'P':
//                 i64 = (int64_t)va_arg(args, pid_t); // 取出%P对应的可变参数
//                 sign = 1; // 显示有符号数
//                 break;
//             case 'f': // 输出浮点数
//                 f = va_arg(args, double); // 取出%f对应的可变参数
//                 if(f < 0) // 负数
//                 {
//                     *buf++ = '-';
//                     f = -f;
//                 }
//                 ui64 = (uint64_t)f; // 取整数部分
//                 frac = 0;
//                 // 求小数点后显示多少位
//                 if(frac_width) // %d.2f，就表示显示2位
//                 {
//                     scale = 1;
//                     for(n = 0; n < frac_width; n++)
//                     {
//                         scale *= 10;
//                     }
//                     frac = (uint64_t)((f - (double)ui64) * scale + 0.5); // 取小数部分，并四舍五入
//                     if(frac >= scale) // 出现了进位，比如0.9
//                     {
//                         ui64++;
//                         frac = 0;
//                     }
                
//                 }

//                 // 输出整数部分
//                 buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width);

//                 // 输出小数点
//                 if(frac_width)
//                 {
//                     if(buf < last)
//                     {
//                         *buf++ = '.';
//                     }
//                     buf = ngx_sprintf_num(buf, last, frac, '0', 0, frac_width); // 输出小数部分
                
//                 }
//                 fmt++;
//                 continue;
//             default:
//                 *buf++ = *fmt++; // 复制%后边的字符到buf
//                 continue;
//             }

//             //显示%d的，会走下来，其他走下来的格式日后逐步完善......
//             if(sign)
//             { // 显示有符号数$0
//                 if(i64 < 0)
//                 {
//                     *buf++ = '-';
//                     ui64 = (uint64_t)-i64;
//                 }
//                 else
//                 {
//                     ui64 = (uint64_t)i64;
                
//                 }
//             }

//             // 输出数字
//             buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width);
//             fmt++;
//         }
//         else // 正常字符
//         {
//             *buf++ = *fmt++; // 复制字符到buf
//         }
//     }

//     return buf;
// }

// // 封装ngx_vslprintf函数
// u_char *ngx_slprintf(u_char* buf, u_char* last, const char* fmt, ...)
// {
//     va_list args;
//     u_char *p;
//     va_start(args, fmt);
//     p = ngx_vslprintf(buf, last, fmt, args);
//     va_end(args);
//     return p;
// }

// u_char *ngx_snprintf(u_char* buf, size_t max, const char* fmt, ...)
// {
//     u_char *p;
//     va_list args;
//     va_start(args, fmt);
//     p = ngx_vslprintf(buf, buf + max, fmt, args);
//     va_end(args);
//     return p;
// }




// }