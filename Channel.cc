#include "Channel.h"
#include <sys/epoll.h>
#include "EventLoop.h"
#include "Logger.h"
const int Channel::kNoneEvent=0;
const int Channel::kReadEvent=EPOLLIN|EPOLLPRI;
const int Channel::kWriteEvent=EPOLLOUT;

//Eventloop:channelList Poller
Channel::Channel(EventLoop *loop,int fd)
:loop_(loop)
,fd_(fd)
,events_(0)
,revents_(0)
,index_(-1)
,tied_(false)
{
    LOG_INFO("%p  Channel Constructor\n",this);
}

Channel::~Channel(){
    LOG_INFO("%p  Channel Destructor\n",this);
};

//channel的tie方法什么时候用过?
// 一个TcpConnection的新连接创建的时候TcpConnection=>Channel::tie()
void Channel::tie(const std::shared_ptr<void>&obj){
    tie_=obj;
    tied_=true;
}


/*
当改变channel所表示的fd的events事件后，update负责更新poller里面更改fd对应的事件epoll_ctl
eventLoop->channelList    poller
*/
void Channel::update()
{
    //通过channel所属的eventLoop,用poller的相应方法，注册fd相应的事件
    //add code...
    loop_->updateChannel(this);
}


void Channel::remove()
{
    //在channel所属的eventLoop中，把当前的channel删除掉。
    //add code...
    loop_->removeChannel(this);
}

//fd得到poller通知以后，处理事件
void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_){
        std::shared_ptr<void> guard=tie_.lock();
        if(guard)
        {
            handleEventWrithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWrithGuard(receiveTime);
    }

};

//根据poller通知的channel发生的具体事件，由channel调用具体函数
void Channel::handleEventWrithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n",revents_);
    

    if((revents_&EPOLLHUP)&& !(revents_&EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }
    if(revents_&EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }

    if(revents_&(EPOLLIN|EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    if(revents_&EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
}
