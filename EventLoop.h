#pragma once
#include <functional>
#include "noncopyable.h"
#include <vector>
#include <atomic>
#include "Timestamp.h"
#include <memory>
#include <mutex>
#include "CurrentThread.h"
class Channel;
class Poller;
//事件循环类，包含俩大模块，channel poller（epoll的抽象）
class EventLoop:noncopyable
{
public:
    using Functor =std::function<void()>;

    EventLoop(/* args */);
    ~EventLoop();

    void loop();//开启事件循环
    void quit();//推出事件循环

    Timestamp pollReturnTime()const{return pollReturnTime_;};

    void runInLoop(Functor cb);//在当前loop中执行cb
    void queueInLoop(Functor cb);//把cb放入队列中，唤醒loop所在线程，执行cb

    void wakeup();//mainRectior 唤醒subRector，唤醒loop所在线程

    //eventLoop 的方法-》poll的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);//用于断言

    //判断是否是在创建他的线程里，是就runInLoop(),不是就queueInLoop()
    bool isInLoopThread()const{return threadId_==CurrentThread::tid();};




private:
    void handleRead();//wakeup
    void doPendingFunctors();//执行cb



    using ChannelList=std::vector<Channel*>;
    //c++11提供的原子操作布尔值，通过CAS实现 
    // compare and swap 是一条CPU原子指令，不会有线程问题
    std::atomic_bool looping_;
    std::atomic_bool quit_;//标志退出loop循环 一般在其他线程调用quit
    const pid_t threadId_;//记录当前loop所在线程的id
    Timestamp pollReturnTime_;//poller 返回发生事件的时间点
    std::unique_ptr<Poller> poller_;
    
    //主要作用，当mainloop获得一个新用户的channel，
    // 通过轮巡算法选择一个sobloop，通过该成员唤醒该subloop
    int wakeupFd_; 
    
    std::unique_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;
    Channel *currentActiveChannel_;//用于断言 调试

    std::atomic_bool callingPendingFunctors_;//表示当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;//存放回调操作的函数,需要锁来保护
    std::mutex mutex_;//互斥锁，用来保护上面vector容器的线程安全操作。
};


