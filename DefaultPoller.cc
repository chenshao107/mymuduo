#include "Poller.h"
#include <stdlib.h>
#include "EPollPoller.h"


Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if(::getenv("MUDUO_USE_POLL")){
        return nullptr;//生成poll实例
    }
    else
    {
        return new EPollPoller(loop);//生存epoll实例
    }
}