#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <vector>
#include <unordered_map>
//muduo库多路事件分发器的核心IO复用的模块

class Channel;
class EventLoop;

class Poller:noncopyable
{

public:
    using ChannelList=std::vector<Channel*>;
    Poller(EventLoop *loop);
    virtual ~Poller()=default;
    //给所有IP复用保留统一接口
    virtual Timestamp poll(int timeoutMS,ChannelList *activeChannels)=0;//启动epoll_wait()
    virtual void updateChannel(Channel * channel)=0;
    virtual void removeChannel(Channel* channel)=0;
    //判断参数channel是否在当前poller当中
    bool hasChannel(Channel* channel)const;

    //EventLoop可以通过该接口获得默认的IO复用的具体实现
    //该方法没有在.cc文件实现，因为要创建出一个实例，涉及具体实现，这个类之提供抽象接口
    static Poller* newDefaultPoller(EventLoop *loop);


protected:
    //map的key：sockfd  value:sockfd所属的channel通道类型
    using ChannelMap=std::unordered_map<int,Channel*>;
    ChannelMap channels_;


private:
    EventLoop *ownerLoop_;
};


