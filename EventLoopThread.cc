#include "EventLoopThread.h"
#include "EventLoop.h"
#include <mutex>

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb
                ,const std::string &name)
                :loop_(nullptr)
                ,exiting_(false)
                ,thread_(std::bind(&EventLoopThread::threadFunc,this))
                ,mutex_()
                ,cond_()
                ,callback_(cb)
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if(loop_!=nullptr)
    {
        loop_->quit();
        thread_.join();
    }
};


//开启新线程，并返回新线程创建的loop
EventLoop* EventLoopThread::startLoop()
{
    thread_.start();//启动线程函数，
    EventLoop *loop=nullptr;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_==nullptr)
        {
            //在此阻塞,再次返回说明cond_.notify_one()被调用或者被其他干扰唤醒
            cond_.wait(lock);
        }
        loop=loop_;

        
    }
    return loop;

};


//下面这个方法是在单独的新线程里面运行的
void EventLoopThread::threadFunc()
{   
    //创建一个独立的Eventloop,和上面的线程是一一对应的，one loop per thread
    EventLoop  loop;

    //有回调就执行回调
    if(callback_)
    {
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_=&loop;
        cond_.notify_one();//通知一个线程
    }
    loop.loop();//eventloop loop  -> Poller.poll()


    //关闭程序
    std::unique_lock<std::mutex> lock(mutex_);
    loop_=nullptr;

}