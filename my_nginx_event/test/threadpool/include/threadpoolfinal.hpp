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
#include <thread>
#include <future>
#include <iostream>

/*
让线程提交任务更方便
1 pool.submitTask(func, args...)
  submitTask使用可变参模板

2 使用c++线程库中的packaged_task(function对象) 
  使用future代替result节省代码
 
 */



// 使用future代替result节省代码
// Any类型，可以接受任意数据类型
/*
    class Any
    {
    public:
        Any() = default;
        ~Any() = default;
        Any(const Any&) = delete; // 禁止左值引用的拷贝构造
        Any& operator=(const Any&) = delete; // 禁止左值引用的赋值构造
        Any(Any&&) = default; 

        Any& operator=(Any&&) = default; 
        // 相当于
        // Any& operator=(Any&& other) {  
        // if (this != &other) { // 防止自赋值  
        //     base_ = std::move(other.base_); // 移动 base_ 的所有权  
        // }  
        // return *this; // 返回当前对象的引用  
        // }
        

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
        Task();
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

*/


const int TASK_MAX_TRESHHOLD = 2;
const int THREAD_MAX_TRESHHOLD = 1024;
const int THREAD_MAX_IDLE_TIME = 60; // 单位: s

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

    Thread(ThreadFunc func)
        :func_(func),
        threadId_(generateId_++)
    {}
    ~Thread() = default;

    // 启动线程
    void start()
    {
        // 创建线程并执行
        std::thread t(func_, threadId_);
        t.detach(); // 分离线程
    }
    
    // 获得线程id
    int getId() const
    {
        return threadId_;
    }

    
   

private:
    /* data */
    ThreadFunc func_;

    static int generateId_;
    int threadId_; // 保存线程id


};

int Thread::generateId_ = 0;


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

    // Task任务 -》 函数对象
    using Task = std::function<void()>;
    std::queue<Task> taskQue_; // 任务队列
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
    void threadFunc(int threadid)
    {
        // std::cout << "thread" << std::this_thread::get_id() << std::endl;
        auto lastTime = std::chrono::high_resolution_clock().now();

        for(;;)
        {
            
            Task task;
            {

                // 先获得锁
                std::unique_lock<std::mutex> lock(taskQueMtc_);

                std::cout << " tid " << std::this_thread::get_id()
                << " try to get task " << std::endl;

                // 如果任务队列为空并且线程池还没有关闭，等待
                while (taskQue_.size() == 0)
                {

                    // 线程池结束
                    if(!isPoolRunning_)
                    {
                        threads_.erase(threadid);
            
                        std::cout << "threadid: " << std::this_thread::get_id() << " exit " << std::endl;
                        exitCond_.notify_all();
                        return;
                    }
                    
                    // cached模式下，如果创建过多线程，但是空闲时间超过60s，考虑结束多余线程
                    if(poolMode_  == PoolMode::MODE_CACHED)
                    {
                        if(std::cv_status::timeout ==
                            notEmpty_.wait_for(lock, std::chrono::seconds(1)))
                        {
                            auto now = std::chrono::high_resolution_clock().now();
                            auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                            if(dur.count() >= THREAD_MAX_IDLE_TIME
                                && curThreadSize_ > initThreadSize_)
                            {
                                // 开始回收线程
                                // 记录线程数量相关变量值的修改

                                // 把线程对象从线程列表中删除 
                                // 同过threadid =》 删除thread对象
                                threads_.erase(threadid);
                                curThreadSize_--;
                                idleThreadSize_--;
                                std::cout << "threadid: " << std::this_thread::get_id() << " exit " << std::endl;
                                return;
                            }
                        }
                    }
                    else
                    {
                        // 等待信号notempty
                        notEmpty_.wait(lock);
                
                    }
                    // // 线程池结束
                    // if(!isPoolRunning_){
                    //     threads_.erase(threadid);
            
                    //     std::cout << "threadid: " << std::this_thread::get_id() << " exit " << std::endl;
                    //     exitCond_.notify_all();
                    //     return;
                    // }
                }
                
                
                
                idleThreadSize_--;
                // 取出任务
                task = taskQue_.front();
                taskQue_.pop();
                taskSize_--;
                std::cout << " tid " << std::this_thread::get_id()
                << " success to get task " << std::endl;
                // 如果还有任务剩余，通知其他线程执行任务
                if(taskQue_.size() > 0){
                    notEmpty_.notify_all();
                }
                // 取出任务，进行通知可以继续生产
                notFull_.notify_all();


            }// 出了作用域就把锁释放
            
            // 当前线程执行任务
            if(task !=  nullptr){
                // task->exec();
                task();
            }
            
            idleThreadSize_++;
            lastTime = std::chrono::high_resolution_clock().now(); // 执行完任务的时间
        }

    }
    // 检查pool运行状态
    std::atomic_bool checkRuningState() const
    {
        return isPoolRunning_.load();
    }

public:
    ThreadPool()
        :initThreadSize_(4),
        taskSize_(0),
        taskQueMaxThreshHold_(TASK_MAX_TRESHHOLD),
        threadSizeThreshHold_(THREAD_MAX_TRESHHOLD),
        curThreadSize_(0),
        poolMode_(PoolMode::MODE_FIXED),
        isPoolRunning_(false),
        idleThreadSize_(0)
    {}

    ~ThreadPool()
    {
        isPoolRunning_ = false;
        

        // 等待线程池所有线程结束
        std::unique_lock<std::mutex> lock(taskQueMtc_);
        notEmpty_.notify_all();
        exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0;});
    } 

    // 开启线程池
    void start(int initThreadSize = std::thread::hardware_concurrency())
    {
        // 启动线程池
        isPoolRunning_ = true;
        // 记录初始线程数量
        initThreadSize_ = initThreadSize;
        // 记录当前线程池中线程数量
        curThreadSize_ = initThreadSize;

        //创建线程对象
        for (int i = 0; i < initThreadSize_; i++)
        {
            // 创建thread线程对象的时候，把线程函数给到thread对象
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
            int threadId = ptr->getId(); // 通过智能指针调用Thread提供的getId方法
            threads_.emplace(threadId, std::move(ptr));
            // threads_.emplace_back(std::move(ptr));
        }
        // 启动线程
        for (int i = 0; i < initThreadSize_; i++)
        {
            threads_[i]->start();
            idleThreadSize_++;
        }
        
    }
    
    // 设置线程池工作模式
    void setMode(PoolMode poolMode)
    {
        if(checkRuningState()) return;

        poolMode_ = poolMode;
    }
    
    // 设置task任务队列上限
    void settaskQueMaxThreshHold(int taskQueMaxThreshHold)
    {
        if(checkRuningState()) return;
        taskQueMaxThreshHold_ = taskQueMaxThreshHold;
    }   
    
    // 设置线程池cached模式下线程数量阈值
    void setthreadSizeThreshHold(int threadSizeThreshHold)
    {
        if(checkRuningState()) return;
        if(poolMode_ == PoolMode::MODE_CACHED)
        {
            threadSizeThreshHold_ = threadSizeThreshHold;
        }
    
    }
    
    // 给线程池提交任务
    // 使用可变参模板让submitTask可以接受任意任务函数和任意数量参数
    // 返回future<>
    template<typename Func, typename... Args>
    auto submitTask(Func&& func, Args... args) -> std::future<decltype(func(args...))>
    {
        // 打包任务,放进任务队列
        using RType = decltype(func(args...));
        auto task = std::make_shared<std::packaged_task<RType()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
        );
        std::future<RType> result = task->get_future();

        // 获得锁
        std::unique_lock<std::mutex> lock(taskQueMtc_);
        // 线程通信 等待任务队列空余
        // 用户提交任务，最长不能阻塞1s，否则提交任务失败
        if(!notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_;}))
        {
            std::cerr << "task queue is full" << std::endl;
            auto task = std::make_shared<std::packaged_task<RType()>>([]()->RType {return RType()/*返回一个RType类型的对象*/;});
            (*task)();
            return task->get_future();
        }
        
        // 如果空余
        // taskQue_.emplace(sp);
        // using Task = std::function<void()>;
        // 通过增加中间函数层间接的将任务传进队列
        // 传入匿名函数对象，通过该对象调用任务
        taskQue_.emplace([task](){(*task)();});
        taskSize_++;

        // 线程通信，执行任务
        notFull_.notify_all();

        // cached模式下任务紧急，场景：小儿快的任务； 根据任务判断是否需要创建新线程，每个任务很快就完成，就不会出现大量线程同时存在的问题
        // fix模式如果任务过重，使用cached模式会创建过多线程，并且每个线程都占用大量资源。
        if(poolMode_ == PoolMode::MODE_CACHED
            && taskSize_ > idleThreadSize_
            && curThreadSize_ < threadSizeThreshHold_)
        {

            std::cout << ">>>> create new thread <<<<" << std::endl;
            // 创建新线程
            // 创建thread线程对象的时候，把线程函数给到thread对象
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
            int threadId = ptr->getId(); // 通过智能指针调用Thread提供的getId方法
            threads_.emplace(threadId, std::move(ptr));
            threads_[threadId]->start();
            idleThreadSize_++;
            curThreadSize_++;
        }

        return result;

    }


    ThreadPool(const ThreadPool&) = delete;// 禁止拷贝构造
    ThreadPool& operator=(const ThreadPool&) = delete;// 禁止赋值构造
};
















#endif