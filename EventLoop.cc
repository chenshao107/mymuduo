#include "EventLoop.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
#include "Logger.h"
// #include <mutex>
#include <memory>

//防止一个线程创建多个EventLoop,检测到非空就不创建了
__thread EventLoop *t_loopInThisThread =nullptr;

const  int kPollTimeMs=10000;//设置默认的超时时间

//用于创建wakefd，用来notify唤醒subReactor,处理新来的channel
int createEventfd()
{
    int evfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(eventfd==0)
    {
        LOG_FATAL("eventfd error:%d\n",errno);
    }
    return evfd;
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,callingPendingFunctors_(false)
    ,threadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))
    ,wakeupFd_(createEventfd())
    ,wakeupChannel_(new Channel(this,wakeupFd_))
    ,currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d",this threadId_);
    if(t_loopInThisThread)
    {
        //如果该线程已经有一个循环了，那不用创建了，出错了
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n"
        ,t_loopInThisThread,threadId_);

    }
    else
    {
        t_loopInThisThread=this;
    }

    //设置wakeupFd的事件类型，以及发生事件后的回调操作。
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    //让每一个EventLoop都监听wakeupChannel的EPOLLIN读事件了，
    // main Reactor就能向sub写东西来唤醒它
    wakeupChannel_->enableReading();



};

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread=0;

};

void EventLoop::loop()
{
    looping_ =true;
    quit_=false;

    LOG_INFO("EventLoop %p start looping \n",this);
    while (!quit_)
    {
        activeChannels_.clear();


        //监听两类fd，一种是client fd，一种是wakeup fd，（用于线程间通讯）
        pollReturnTime_=poller_->poll(kPollTimeMs,&activeChannels_);
        for(Channel*channel:activeChannels_)
        {
            //poller监听哪些channel发生事件，然后上报给EventLoop,通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
            //执行当前EventLoop事件循环需要处理的回调操作。
            /*
            io线程 mainloop 回调是 accept fd<= channel subloop  
            mainloop 事先注册一个回调cb （需要subloop来执行） 
            wakeup subloop后，执行下面的方法，执行之前mainloop注册的cb操作
            */
            doPendingFunctors();//这是让mainloop唤醒subloop后，subloop该干的事

        }

    }
    LOG_INFO("EventLoop %p  stop looping. \n",this);
    looping_=false;
    
}

//退出事件循环，如果不在对应loop中，要把它唤醒
void EventLoop::quit()
{
    quit_=true;
    //如果是在其他线程中调用quit，在subloop中，调用mainloop（IO）的quit
    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)//在当前loop中执行cb
{
    if(isInLoopThread())//在当前的loop线程中执行cb
    {
        cb();
    }
    else//在非当前loop线程中执行cb,唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }

}
void EventLoop::queueInLoop(Functor cb)//把cb放入队列中，唤醒loop所在线程，执行cb
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    //唤醒相应需要执行上面回调操作的线程
    //callingPendingFunctors_ 如果为true，说明在执行回调，
    //如果这时不调用wakeup（），他执行完回调后阻塞在poll
    if(!isInLoopThread()||callingPendingFunctors_)
    {
        wakeup();//唤醒loop所在线程

    }
}


void EventLoop::handleRead()
{
    uint64_t one =1;
    ssize_t n =read(wakeupFd_,&one,sizeof one);
    if(n!=sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8",n);
    }
    
}

//mainRectior 唤醒subRector，唤醒loop所在线程
//唤醒loop所在线程，向wakeupFd写一个数据，wakeupchannel就发生读事件
void EventLoop::wakeup()
{
    uint64_t one=1;
    ssize_t n=write(wakeupFd_,&one,sizeof one);
    if(n!=sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() writes %ld bytes instead of 8",n);

    }
}
//eventLoop 的方法-》poll的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)//用于断言
{
    return poller_->hasChannel(channel);
}


void EventLoop::doPendingFunctors()//执行cb
{
    std::vector<Functor> Functors;
    callingPendingFunctors_=true;

    {
        //v
        std::unique_lock<std::mutex> lock(mutex_);
        swap(Functors,pendingFunctors_);
    }

    for(const Functor& functor:Functors)
    {
        functor();//执行当前需要执行的回调操作
    }
    callingPendingFunctors_=false;
}