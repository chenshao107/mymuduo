#include "EPollPoller.h"
#include "Logger.h"
#include "errno.h"
#include "unistd.h"
#include "Channel.h"
#include <string.h>


//指示channel的状态
const int kNew=-1;//channel 的成员index_=-1 表示未添加
const int kAdded=1;//已添加
const int kDeleted=2;//删除



EPollPoller::EPollPoller(EventLoop * loop)
:Poller(loop)
,epollfd_(::epoll_create1(EPOLL_CLOEXEC))
,events_(kInitEventListSize)//vector<epoll_event>
{
    if(epollfd_<0)
    {
        LOG_FATAL("epoll_create error:%d \n",errno);
    }

};

EPollPoller::~EPollPoller()
{
    close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMS,ChannelList *activeChannels)
{
    //实际上应该用LOG——DEBUG输出日志更合理，因为在高并发的情况下，调用很是频繁
    //如果用info打印，会降低函数效率
    LOG_INFO("func=%s => fd total count:%d\n",
    __FUNCTION__,static_cast<int>( channels_.size()));
    int numEvents = ::epoll_wait(this->epollfd_,&*events_.begin()
    ,static_cast<int>( events_.size()),timeoutMS);
    int saveErrno= errno; //因为其他线程循环的操作都有可能写errno，所以先记录下来
    Timestamp now(Timestamp::now());
    if(numEvents>0)
    {
        LOG_INFO("%d events happened \n",numEvents);
        fillActiveChannels(numEvents,activeChannels);
        //如果发生事件的个数和当前最大容量相同，则可能实际发生事件的个数更多
        //需要扩容
        if(numEvents==events_.size())
        {
            events_.resize(events_.size()*2);
        }
    }
    else if(numEvents==0)
    {
        LOG_DEBUG("%s timeout!\n",__FUNCTION__);
    }
    else
    {
        if(saveErrno!=EINTR)//如果不为中断引起,为其他错误引起
        {
            errno=saveErrno;//重新赋值自己线程的errno，因为日志类读取全局变量errno
            LOG_ERROR("EPollPoller::poll() err!\n");
        }
        
    }
    return now;
};

/*
            eventloop
    channellist     poller
                    channelMap<fd,channel*>
*/

void EPollPoller::updateChannel(Channel *channel)
{
    const int index=channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n",
    __FUNCTION__,channel->fd(),channel->events(),index);

    if(index==kNew||index==kDeleted)
    {
        if(index==kNew)
        {
            int fd=channel->fd();
            channels_[fd]=channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }
    else//channel已经在poller上注册过了
    {
        int fd=channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD,channel);
        }
    }


};

//从poller中删除channel
void EPollPoller::removeChannel(Channel* channel)
{
    
    int fd=channel->fd();
    channels_.erase(fd);

    int index =channel->index();

    LOG_INFO("func=%s => fd=%d \n",
    __FUNCTION__,channel->fd());
    if(index==kAdded)
    {
        update(EPOLL_CTL_DEL,channel);

    }
    channel->set_index(kNew);


};


    
//填写活动的连接
void EPollPoller::fillActiveChannels(int numEvents,ChannelList *activeChannels)
{
    for (int  i = 0; i < numEvents; i++)
    {
        Channel *channel =static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);//Eventloop拿到了所有发生事件的channel列表。

    }
    
};
//channel update remove->eventloop->updateChannel removeChannel->poller
void EPollPoller::update(int operation, Channel* channel)
{

    epoll_event event;
    memset(&event,0,sizeof event);
    int fd=channel->fd();
    event.events=channel->events();
    event.data.ptr=channel;
//    event.data.fd=fd;//ShaobinDebug

    if(::epoll_ctl(epollfd_,operation,fd,&event)<0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d \n",errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d \n",errno);
        }
    }

};