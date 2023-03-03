#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func,const std::string &name)
    :started_(false)
    ,joined_(false)
    ,tid_(0)
    ,func_(std::move(func))
    ,name_(name)
{

};

;
Thread::~Thread()
{
    //检测线程是否运行起来,没运行起来的话不需要析构
    //区别守护线程 和 普通进程
    if(started_&&!joined_)
    {
        thread_->detach();//thread类提供的设置分离线程的方法
    }
};
void Thread::start()//一个thread对象记录的就是一个线程的详细信息
{
    started_=true;
    sem_t sem;
    sem_init(&sem,false,0);//进程间通讯,false表示线程共享
    thread_ = std::shared_ptr<std::thread>
    (
        //用lambda表达式生成线程函数，函数用于开启线程
        new std::thread
        ([&]()
            {
                //获取线程的tid值
                tid_=CurrentThread::tid();
                sem_post(&sem);
                func_();//开启一个新线程，专门执行该函数
            }
        )
    );
    //这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem);

};
void Thread::join()
{
    joined_=true;
    thread_->join();
};


void Thread::setDefaulName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32]={0};
        snprintf(buf,sizeof buf,"Thread%d",num);
        name_=buf;
    }
};
