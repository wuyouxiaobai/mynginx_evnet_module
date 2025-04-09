#include "threadpoolfinal.hpp"
#include <thread>
#include <chrono>
#include <future>
#include <iostream>

int sum1(int a, int b)
{
    return a + b;
}

int sum2(int a, int b, int c)
{
    return a + b + c;
}



int main(int argv, char* args[]){
    // 实现线程池的析构释放资源
    {
        ThreadPool pool;
        // 设置线程模式
        // pool.setMode(PoolMode::MODE_CACHED);
        // 开始线程池
        pool.start(2);

        std::future<int> r1 = pool.submitTask(sum1, 1, 2);
        std::future<void> r2 = pool.submitTask([](){
            std::this_thread::sleep_for(std::chrono::seconds(5));
        });
        std::future<void> r3 = pool.submitTask([](){
            std::this_thread::sleep_for(std::chrono::seconds(5));
        });
        std::future<void> r4 = pool.submitTask([](){
            std::this_thread::sleep_for(std::chrono::seconds(5));
        });

        r4 = pool.submitTask([](){
            std::this_thread::sleep_for(std::chrono::seconds(5));
        });

        r4 = pool.submitTask([](){
            std::this_thread::sleep_for(std::chrono::seconds(5));
        });
        std::cout << r1.get() << std::endl;
     



   
        // ulong sum2 = result2.get().cast_<ulong>();
        // ulong sum3 = result3.get().cast_<ulong>();


        // pool.submitTask(std::make_shared<myTask>());
        // pool.submitTask(std::make_shared<myTask>());
        // pool.submitTask(std::make_shared<myTask>());
        // pool.submitTask(std::make_shared<myTask>());
        // ulong sum = sum1;
        // std::cout << "sum result" << sum << std::endl;
    }


    std::this_thread::sleep_for(std::chrono::seconds(5));
    getchar();
    return 0;
}