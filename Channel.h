#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>


//能不包含头文件就不包含头文件，减少信息暴露，或者出现循环包含。
class EventLoop;


/*
理清楚Eventloop Channel Poller之间的关系 <- Reactor模型上对应Demultiplex 
channel理解为通道，封装了sockfd和其感兴趣的event，如epoll，epollout事件
还绑定了poller返回的具体事件

*/
class Channel:noncopyable
{

public:
    using EventCallback=std::function<void()> ;
    using ReadEventCallback=std::function<void(Timestamp)>;

    Channel(EventLoop *loop,int fd);
    ~Channel();
    //fd得到poller通知以后，处理事件的，调用相应的回调函数
    void handleEvent(Timestamp receiveTime);
    
    //设置回调函数对象
    void setReadCallback(ReadEventCallback cb){readCallback_=std::move(cb);}
    void setWriteCallback(EventCallback cb){writeCallback_=std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_=std::move(cb);}
    void setErrorCallback(EventCallback cb){errorCallback_=std::move(cb);}

    //防止当channel被手动remove掉，但他还在执行回调操作
    void tie(const std::shared_ptr<void>&);
    
    int fd()const{return fd_;}
    int events(){return events_;}
    void set_revents(int revt)
    {
        revents_=revt;
    }



    void enableReading (){events_|=kReadEvent;update();}
    void disableReading(){events_&=~kReadEvent;update();}
    void enableWriteing(){events_|=kWriteEvent;update();}
    void disableWriteing(){events_&=~kWriteEvent;update();}
    void disableAll(){events_=kNoneEvent;update();}

    //返回fd当前的事件状态
    bool isNoneEvent()const{return events_==kNoneEvent;}
    bool isWriting()const{return events_&kWriteEvent;}
    bool isReading()const{return events_&kReadEvent;}

    int index(){return index_;}
    void set_index(int idx){index_=idx;}

    //one loop per thread
    EventLoop* ownerLoop(){return loop_;}
    void remove();
private:
    void update();
    void handleEventWrithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    EventLoop *loop_;//事件循环
    const int fd_;      //fd,poller 监听的对象
    int events_;        //注册fd感兴趣的对象
    int revents_;       //poller返回具体发生的事件
    int index_;         //

    std::weak_ptr<void> tie_;

    //用来指向自己，用于线程使用channel前，判断channel是否正在被析构
    bool tied_;

    //因为channel通道里面能够获知fd最终发生的具体的事件revents
    //所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback     writeCallback_;
    EventCallback     closeCallback_;
    EventCallback     errorCallback_;

};


