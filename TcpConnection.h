#pragma once
#include "noncopyable.h"
#include <memory>
#include <string>
#include <atomic>

#include "InetAddress.h"
#include "Buffer.h"
#include "Callbacks.h"
#include "Timestamp.h"

class Channel;
class EventLoop;
class Socket;


/*
TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
=> TcpConnection 设置回调=》 channel=》Poller =》channel回调操作

*/

class TcpConnection:noncopyable,public std::enable_shared_from_this<TcpConnection>
{

public:

    TcpConnection(EventLoop *loop
                    ,const std::string &name
                    ,int sockfd
                    ,const InetAddress &localAddr
                    ,const InetAddress &peerAddr);
    ~TcpConnection();
    EventLoop* getLoop()const {return loop_;}
    const std::string& name(){return name_;}
    const InetAddress& localAddr()const{return localAddr_;}
    const InetAddress& peerAddr()const {return peerAddr_;}

    bool connected()const {return state_==kConnected;};

    // void send(const void *message,int len);//发送数据
    void send(const std::string& buf);
    void shutdown();//关闭连接

    void setConnectionCallback(const ConnectionCallback &cb)
    {connectionCallback_=cb;}
    void setMessageCallback(const MessageCallback &cb)
    {messageCallback_=cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {writeCompleteCallback_=cb;}
    void setHighWaterMarkCallback(const HighWaterMatkCallback &cb,size_t hiWaterMark)
    {highWaterMatkCallback_=cb;highWaterMark_=hiWaterMark;}

    void setCloseCallback(const CloseCallback &cb)
    {closeCallback_=cb;}

    //连接建立
    void connectEstablished();
    //建立销毁
    void connectDestroyed();
    // void send(const std::string& buf);

private:
    enum StateE{kDisconnected,kConnecting, kConnected, kDisconnecting};

    void setState(StateE state){state_=state;};
    
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();


    void sendInLoop(const void* message,size_t len);
    void shutdownInLoop();



    EventLoop *loop_;//绝对不是baseloop，因为tcpConnection都是在subLoop里管理的
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress  localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;//有新连接时的回调
    MessageCallback  messageCallback_;//有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完成以后的回调
    CloseCallback closeCallback_;//关闭回调
    HighWaterMatkCallback highWaterMatkCallback_;//高水位回调
    size_t highWaterMark_; //水位标志
    Buffer inputBuffer_;//接受数据缓冲区
    Buffer outputBuffer_;//发送数据缓冲区


};


