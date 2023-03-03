#include "TcpServer.h"
#include "Logger.h"
#include <strings.h>
#include "TcpConnection.h"


//写静态，与外部不冲突
static EventLoop* CheckLoopNotNull(EventLoop *loop  )
{
    if(loop==nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null \n",
            __FILE__,__FUNCTION__,__LINE__);
        
    }
    return loop;
}



TcpServer::TcpServer(EventLoop *loop,
            const InetAddress &listenAddr,
            const std::string &nameArg,
            Option option)
    :loop_(CheckLoopNotNull(loop))
    ,ipPort_(listenAddr.toIpPort())
    ,name_(nameArg)
    ,acceptor_(new Acceptor(loop,listenAddr,option==kReusePort))
    ,threadPool_(new EventLoopThreadPool(loop,name_))
    ,connectionCallback_()
    ,nextConnId_(1)
    ,started_(0)
{
    //当有新用户连接时，会执行Tcpserver：：newConnection回调
    acceptor_->setNewConnectionCallback
        (
            std::bind
            (&TcpServer::newConnection
            ,this
            ,std::placeholders::_1
            ,std::placeholders::_2
            )
        );

    
}




void TcpServer::setThreadNum(int numThreads)//设置地层subloop个数
{
    threadPool_->setThreadNum(numThreads);

}

//开启服务器监听 
void TcpServer::start()
{
    if(started_++ ==0)//防止一个tcpServer对象被start多次
    {
        threadPool_->start(threadInitCallback_);//启动地层的loop线程池
        // loop_->runInLoop([&](){return this->acceptor_->listen();});
        loop_->runInLoop(std::bind(&Acceptor::listen,this->acceptor_.get()));

    }
};

//有一个新客户端的连接，accept会执行这个回调
void TcpServer::newConnection(int sockfd,const InetAddress &peerAddr)
{
    //轮询算法 选个一个subloop，来管理channel
    EventLoop *ioloop=threadPool_->getNextLoop();
    char buf[64]={0};
    snprintf(buf,sizeof buf,"-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName=name_+buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n",
            name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());
    //通过sockfd获取其绑定的本机的ip地址和端口信息。
    sockaddr_in local;
    bzero(&local,sizeof local);
    socklen_t addrlen = sizeof local;
    if(::getsockname(sockfd,(sockaddr*)&local,&addrlen)<0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);
    //根据连接成功的sockfd，创建tcpConnection连接对象。
    TcpConnectionPtr conn(new TcpConnection(
                            ioloop,
                            connName,
                            sockfd,
                            localAddr,
                            peerAddr
                            )
    );

    this->connections_[connName]=conn;

    //下面的回调都是用户设置
    // 给TcpServer -> TcpConnection -> Channel -> Poller->notify channel调用回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writecompleteCallback_);

    //设置了如何关闭连接的回调，
    //用户调用conn->shutdown()
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection,this,std::placeholders::_1)
    );

    //直接调用
    ioloop->runInLoop(std::bind(
        &TcpConnection::connectEstablished,conn
    ));
};

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectInLoop,this,conn)
    );
}
void TcpServer::removeConnectInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectInLoop [%s] - connection %s \n",
        name_.c_str(),conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed,conn)
    );
}

TcpServer::~TcpServer()
{
    for(auto &item:connections_)
    {
        //这个局部的shared_ptr智能指针对象出了右大括号，自动析构TcpConnection,引用计数为0
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        //销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed,conn)
        );
    }
}