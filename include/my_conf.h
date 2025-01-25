#pragma once
#include <memory>
#include <vector>
#include "my_global.h"

namespace WYXB
{

class MyConf
{
private:
    MyConf(){};
public:
    ~MyConf(){};
 
public:
    static MyConf* getInstance() {
        static MyConf instance;  // 静态局部变量，确保只实例化一次
        return &instance;
    }

    // // 禁止拷贝和赋值
    // MyConf(const MyConf&) = delete;
    // MyConf& operator=(const MyConf&) = delete;
public:
    using myConfItemPtr = std::unique_ptr<myConfItem>;

    bool LoadConf(const char* myconfname);
    const char* GetString(const char* key);
    int GetIntDefault(const char* key, const int default_value);

public:
    std::vector<myConfItemPtr> m_ConfigItemList;
    
};




 
    
}
