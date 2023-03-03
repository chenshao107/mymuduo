#include "Acceptor.h"
#include <sys/types.h>         
#include <sys/socket.h>
#include "Logger.h"
#include "Socket.h"
#include "InetAddress.h"
#include "unistd.h"

static int createNonblocking()
{
    int sockfd=::socket
    (AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
    if(sockfd<0)
    {
        LOG_FATAL
        ("%s:%s:%d listen socket create err:%d \n"
        ,__FILE__,__FUNCTION__,__LINE__,errno);

    }
    return sockfd;
}

Acceptor::Acceptor
(EventLoop *loop,const InetAddress &listenAddr,bool reuseport)
    :loop_(loop)
    ,acceptSocket_(createNonblocking())
    ,acceptChannel_(loop,acceptSocket_.fd())
    ,listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    //tcpServer ::start() Acceptor,listen()
    //如果有新用户的连接，需要执行一个回调
    //将连接fd打包为一个channel 给subloop
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));//ChenshaoDebug
//    acceptChannel_.setReadCallback([&](Timestamp timestamp){return handleRead();});

};
Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
};


void Acceptor::listen()
{
    listenning_=true;
    acceptSocket_.listen();//listen
    acceptChannel_.enableReading();//acceptChannel_ => Poller

};



//listenfd 有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    LOG_INFO("Acceptor::handleRead()");
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd>=0)
    {
        if(NewConnectionCallback_)
        {
            //如果有回调
            //轮询找到subloop唤醒分发当前新客户的channel
            NewConnectionCallback_(connfd,peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else//出错
    {
        LOG_ERROR
        ("%s:%s:%d Accept err:%d \n"
        ,__FILE__,__FUNCTION__,__LINE__,errno);
        if(errno==EMFILE)
        {
            LOG_ERROR
            ("%s:%s:%d sockfd reached  limit !!!\n"
            ,__FILE__,__FUNCTION__,__LINE__);
        }
    }
};