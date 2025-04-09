#include "threadpool.hpp"
#include <thread>

const int TASK_MAX_TRESHHOLD = 1024;
const int THREAD_MAX_TRESHHOLD = 1024;
const int THREAD_MAX_IDLE_TIME = 60; // 单位: s


Task::Task(/* args */)
    : result_(nullptr)
{}

void Task::setResult(Result* res)
{
    result_ = res;
}

void Task::exec()
{
    result_->setVal(run()); // 发生多态
}


// 线程池构造
ThreadPool::ThreadPool()
    :initThreadSize_(4),
    taskSize_(0),
    taskQueMaxThreshHold_(TASK_MAX_TRESHHOLD),
    threadSizeThreshHold_(THREAD_MAX_TRESHHOLD),
    curThreadSize_(0),
    poolMode_(PoolMode::MODE_FIXED),
    isPoolRunning_(false),
    idleThreadSize_(0)
{}

// 线程池析构
ThreadPool::~ThreadPool()
{
    isPoolRunning_ = false;
    

    // 等待线程池所有线程结束
    std::unique_lock<std::mutex> lock(taskQueMtc_);
    notEmpty_.notify_all();
    exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0;});
} 

// 开启线程池
void ThreadPool::start(int initThreadSize)
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


// 检查pool运行状态
std::atomic_bool ThreadPool::checkRuningState() const
{
    return isPoolRunning_.load();
}

// 设置线程池工作模式
void ThreadPool::setMode(PoolMode poolMode)
{
    if(checkRuningState()) return;

    poolMode_ = poolMode;
}

// 设置task任务队列上限
void ThreadPool::settaskQueMaxThreshHold(int taskQueMaxThreshHold)
{
    if(checkRuningState()) return;
    taskQueMaxThreshHold_ = taskQueMaxThreshHold;
}

// 设置线程池cached模式下线程数量阈值
void ThreadPool::setthreadSizeThreshHold(int threadSizeThreshHold)
{
    if(checkRuningState()) return;
    if(poolMode_ == PoolMode::MODE_CACHED)
    {
        threadSizeThreshHold_ = threadSizeThreshHold;
    }
    
}

// 给线程池提交任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
    // 获得锁
    std::unique_lock<std::mutex> lock(taskQueMtc_);
    // 线程通信 等待任务队列空余
    // 用户提交任务，最长不能阻塞1s，否则提交任务失败
    if(!notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_;}))
    {
        std::cerr << "task queue is full" << std::endl;
        return Result(sp, false); // 返回一个无效任务 
    }
    
    // 如果空余
    taskQue_.emplace(sp);
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

    return Result(sp);
}

void ThreadPool::threadFunc(int threadid)
{
    // std::cout << "thread" << std::this_thread::get_id() << std::endl;
    auto lastTime = std::chrono::high_resolution_clock().now();

    for(;;)
    {
        
        std::shared_ptr<Task> task;
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
            task->exec();
        }
        
        idleThreadSize_++;
        lastTime = std::chrono::high_resolution_clock().now(); // 执行完任务的时间
    }


    
}






//////// 线程方法实现
int Thread::generateId_ = 0;

Thread::Thread(ThreadFunc func)
    :func_(func),
    threadId_(generateId_++)
{

}


Thread::~Thread()
{

}

// 获得线程id
int Thread::getId() const
{
    return threadId_;
}

// 启动线程
void Thread::start()
{
    // 创建线程并执行
    std::thread t(func_, threadId_);
    t.detach(); // 分离线程
}


//////result

Result::Result(std::shared_ptr<Task> task, std::atomic_bool isValid)
        : task_(task), isValid_(isValid.load())
{
    task_->setResult(this);
}


Any Result::get()
{
    if(!isValid_)
    {
        return "";
    }

    sem_.wait();
    return std::move(any_);
}


void Result::setVal(Any any)
{
    // 保存task返回值
    this->any_ = std::move(any);
    sem_.post();
}