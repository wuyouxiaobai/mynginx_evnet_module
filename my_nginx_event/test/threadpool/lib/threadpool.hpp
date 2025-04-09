#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <iostream>
#include <unordered_map>

// Any类型，可以接受任意数据类型
class Any
{
public:
    Any() = default;
    ~Any() = default;
    Any(const Any&) = delete; // 禁止左值引用的拷贝构造
    Any& operator=(const Any&) = delete; // 禁止左值引用的赋值构造
    Any(Any&&) = default; 

    Any& operator=(Any&&) = default; 
    /* 相当于
    Any& operator=(Any&& other) {  
    if (this != &other) { // 防止自赋值  
        base_ = std::move(other.base_); // 移动 base_ 的所有权  
    }  
    return *this; // 返回当前对象的引用  
    }*/
    

    // 这个方法能把Any对象里保存的data取出
    // 模板类代码只能写在头文件
    template<typename T>
    T cast_()
    {
        Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
        if(pd == nullptr){
            throw "type is unmatch";
        }
        return pd->data_;
    }



    template<typename T>
    Any(T data) : base_(std::make_unique<Derive<T>>(data))
    {}
private:
    // 基类类型
    class Base
    {
    public:
        virtual ~Base() = default;
    };

    // 派生类型
    template<typename T>
    class Derive : public Base
    {
    public:
        T data_;
    public:
        Derive(T Derivedate) : data_(Derivedate)
        {}
        ~Derive() = default;
    };
    
private:
    std::unique_ptr<Base> base_;
    

};

class Result;
// 任务抽象基类，用户自定义
class Task
{
private:
    Result* result_; // 为了避免在智能指针的交叉引用，写成裸指针
public:
    Task(/* args */);
    ~Task() = default;
    // 用户自定义run方法
    virtual Any run() = 0;

    void setResult(Result* res);

    void exec();


};

// 信号量
class Semaphore
{
private:
    /* data */
    int resLimit_;
    std::mutex mtx_;
    std::condition_variable cond_;
    std::atomic_bool isExit_;

public:
    Semaphore(int resLimit = 0)
        : resLimit_(resLimit),
        isExit_(false)
    {}
    ~Semaphore()
    {
        isExit_ = true;
    }
    //获取信息量
    void wait()
    {
        if(isExit_) return;
        std::unique_lock<std::mutex> lock(mtx_);
        cond_.wait(lock, [&]() -> bool{return resLimit_ > 0;});
        resLimit_--;
    }
    // 增加信号量
    void post()
    {
        if(isExit_) return;
        std::unique_lock<std::mutex> lock(mtx_);
        resLimit_++;
        cond_.notify_all();
    }
    
};


// 实现接受线程池完成的任务返回类型Result
class Result
{
private:
    /* data */
    Any any_; // 存储任务结果返回值
    Semaphore sem_; // 线程通信信号量
    std::shared_ptr<Task> task_;// 指向任务对象
    std::atomic_bool isValid_;// 任务是否有效
    


public:
    Result(std::shared_ptr<Task>task, std::atomic_bool isValid_ = true);
    ~Result() = default;
    Any get(); // 获得任务执行结果返回值
    void setVal(Any any); // 设置任务结果并增加信号量



};



// 线程池模式
enum class PoolMode
{
    MODE_FIXED, 
    MODE_CACHED,

};



// 线程类型
class Thread
{
public:
    // 线程函数对象类型
    using ThreadFunc = std::function<void(int)>;

    Thread(ThreadFunc func);
    ~Thread();

    // 启动线程
    void start();
    
    // 获得线程id
    int getId() const;

    
   

private:
    /* data */
    ThreadFunc func_;

    static int generateId_;
    int threadId_; // 保存线程id


};




// 线程池类型
// 线程池是考虑先做完所所有任务在释放线程和线程池
class ThreadPool
{
private:
    /* data */
    // std::vector<std::unique_ptr<Thread>> threads_;
    std::unordered_map<int, std::unique_ptr<Thread>> threads_; // 线程列表,（线程id和线程指针）
    size_t initThreadSize_; // 初始线程数量
    int threadSizeThreshHold_; //线程上限数量
    std::atomic_int curThreadSize_; // 线程池线程总数量
    std::atomic_int idleThreadSize_; // 记录空闲线程数量

    std::queue<std::shared_ptr<Task>> taskQue_; // 任务队列
    std::atomic_int taskSize_; //任务数量
    int taskQueMaxThreshHold_; //任务队列上限数量

    std::mutex taskQueMtc_; // 保证任务队列线程安全
    std::condition_variable notFull_; // 表示任务队列不满
    std::condition_variable notEmpty_; // 表示任务队列不空
    std::condition_variable exitCond_; // 线程池析构信号量

    PoolMode poolMode_; // 当前线程池的模式
    std::atomic_bool isPoolRunning_;// 当前线程池是否启动
    

private:
    // 定义线程函数
    void threadFunc(int threadid);
    // 检查pool运行状态
    std::atomic_bool checkRuningState() const;

public:
    ThreadPool(/* args */);
    ~ThreadPool();

    // 开启线程池
    void start(int initThreadSize = std::thread::hardware_concurrency());
    // 设置线程池工作模式
    void setMode(PoolMode poolMode);
    // 设置task任务队列上限
    void settaskQueMaxThreshHold(int taskQueMaxThreshHold);
    // 设置线程池cached模式下线程数量阈值
    void setthreadSizeThreshHold(int threadSizeThreshHold);
    // 给线程池提交任务
    Result submitTask(std::shared_ptr<Task> sp);


    ThreadPool(const ThreadPool&) = delete;// 禁止拷贝构造
    ThreadPool& operator=(const ThreadPool&) = delete;// 禁止赋值构造
};
















#endif