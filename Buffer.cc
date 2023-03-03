#include "Buffer.h"
#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>

/*
从fd上读取数据，poller工作在LT模式（与边缘出发相对）
buffer有大小，但从fd读数据是未知大小。

*/

ssize_t Buffer::readFd(int fd,int * saveErrno)
{
    char extrabuf[65536]={0};//栈上64k内存
    struct  iovec vec[2];
    const size_t writable = writeableBytes();//这是Buffer底层缓冲区剩余的可写大小。
    vec[0].iov_base=begin()+writerIndex_;
    vec[0].iov_len=writable;

    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof extrabuf;
    
    const int iovcnt = (writable < sizeof extrabuf)?2:1;//一次至少读出64k
    const ssize_t n =::readv(fd,vec,iovcnt);
    if(n<0)
    {
        *saveErrno = errno ;
    }
    else  if(n<=writable)//Buffer的可写缓冲区已经够存储读出来的数据了。
    {
        writerIndex_+=n;
    }
    else//extraBuff里也写了数据。
    {
        writerIndex_=buffer_.size();
        append(extrabuf,n-writable);//writerIndex_开始写

    }
    return n;
    
}
//从fd上写数据
ssize_t Buffer::writeFd(int fd,int * saveErrno)
{
    ssize_t n =::write(fd,peek(),readableBytes());
    if(n<0)
    {
        *saveErrno =errno;
    }
    return n;
}
