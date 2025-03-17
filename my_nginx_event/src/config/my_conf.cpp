
#include "my_conf.h"
#include <fstream>
#include <string>
#include <functional>
#include <algorithm>
#include <my_global.h>
#include <cstring>
#include <iterator>
#include <limits.h>
#include <iostream>
#include <unistd.h> 

namespace WYXB
{

bool MyConf::LoadConf(const char* myconfname)
{
    // 获取可执行文件所在目录  
    char exePath[PATH_MAX];  
    ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX);  
    if (count == -1) {  
        std::cerr << "Error: Unable to get executable path." << std::endl;  
        return false;  
    }  
    exePath[count] = '\0'; // 确保字符串以 '\0' 结尾  

    // 提取可执行文件所在目录  
    std::string exeDir = std::string(exePath).substr(0, std::string(exePath).find_last_of('/'));  

    // 拼接配置文件的绝对路径（config 文件夹下的配置文件）  
    std::string configPath = exeDir + "/../config/" + myconfname;

    // 打开配置文件  
    std::ifstream conf_file(configPath);  
    if (!conf_file.is_open()) {  
        std::cerr << "Error: Unable to open configuration file: " << configPath << std::endl;  
        return false;  
    }  
    
    std::string line;
    std::function<void(std::string&)> Trim = [](std::string& str) {
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), str.end());
    }; 
    while (std::getline(conf_file, line))
    {

        // 去除空白行
        Trim(line);
        // 处理空行和注释行
        if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '[') {
            continue;
        }
        
        // 分割键值对
        auto delimiterPos = line.find('=');
        if(delimiterPos != std::string::npos)
        {
            myConfItemPtr item = std::make_unique<myConfItem>();
            
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);
            
            Trim(key);
            Trim(value);
            
            strncpy(item->ItemName, key.c_str(), sizeof(item->ItemName)-1);
            strncpy(item->ItemContent, value.c_str(), sizeof(item->ItemContent)-1);
            
            m_ConfigItemList.push_back(std::move(item));
        }
    }
    return true;

}
const char* MyConf::GetString(const char* key)
{
    auto it = std::find_if(m_ConfigItemList.begin(), m_ConfigItemList.end(), 
        [key](const myConfItemPtr& item) {
            return strcmp(item->ItemName, key) == 0;
        }
    );
    
    if(it != m_ConfigItemList.end())
    {
        return (*it)->ItemContent;
    }

    return nullptr;
}
int MyConf::GetIntDefault(const char* key, const int default_value)
{
    auto it = std::find_if(m_ConfigItemList.begin(), m_ConfigItemList.end(),
        [key](const myConfItemPtr& item) {
            return strcasecmp(item->ItemName, key) == 0;
        });

    if (it != m_ConfigItemList.end()) {
        return atoi((*it)->ItemContent);
    }

    return default_value; // 如果未找到，则返回默认值
}


    
}


#ifdef UNIT_TEST

int main()
{
    // 获取 MyConf 实例
    WYXB::MyConf& conf = WYXB::MyConf::getInstance();
    // 加载配置文件
    conf.LoadConf("nginx.conf");

    std::string key; // 用于存储用户输入的 key

    // 进入循环，允许用户输入 key 进行查询
    while (true)
    {
        std::cout << "请输入要查询的配置项的 Key (输入 'exit' 退出): ";
        std::getline(std::cin, key); // 从控制台读取输入

        if (key == "exit") // 输入 'exit' 以退出循环
        {
            break;
        }

        // 获取并打印字符串类型的值
        const char* value = conf.GetString(key.c_str());
        if (value)
        {
            std::cout << key << ": " << value << std::endl;
        }
        else
        {
            // 如果没有找到值，尝试获取整数值，并设定默认值
            int intValue = conf.GetIntDefault(key.c_str(), -1); // 默认值设为 -1 表示未找到
            if (intValue != -1)
            {
                std::cout << key << ": " << intValue << std::endl;
            }
            else
            {
                std::cout << "未找到对应的配置项: " << key << std::endl;
            }
        }
    }

    return 0;
}

#endif