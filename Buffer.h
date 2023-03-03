#pragma once

#include <vector>
#include <string>
#include <algorithm>

//    perpendable              readable           writable        
// 0  <=         readerIndex_    <=      writerIndex_   <=   size

//网络库底层的缓冲器类型定义
class Buffer
{

public:
    static const size_t kCheapPrepend   =8;
    static const size_t kInitialSize    =1024;
    explicit Buffer (size_t initialSize = kInitialSize)
        :buffer_(kInitialSize+initialSize)
        ,readerIndex_(kCheapPrepend)
        ,writerIndex_(kCheapPrepend)
    {}

    size_t readableBytes()const
    {
        return writerIndex_-readerIndex_;
    }

    size_t writeableBytes()const
    {
        return buffer_.size()-writerIndex_;
    }
    size_t perendableBytes()const
    {
        return readerIndex_;
    }

    //返回缓冲区中可读数据的起始地址
    const char* peek()const
    {
        return begin()+readerIndex_;
    }

    //onMessage   string <- Buffer
    void retrieve(size_t len)
    {
        if(len<readableBytes())
        {
            //应用只读取了刻度缓冲区数据的一部分，就是len
            // ，还剩下readerIndex_  +len 到 writerInex_
            readerIndex_+=len;
            
        }
        else//len <= readableBytes()
        {
            retrieveAll();

        }
    }

    void retrieveAll()
    {
        readerIndex_= writerIndex_=kCheapPrepend;
    };

    //把onMessage函数上的buffer数据，转成string类型的数据返回。
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);
        retrieve(len);//上面一句把缓冲区中可读的数据 取出来，这里肯定要对缓冲区进行复位操作。
        return result;

    }

    // buffer.size()  - writerIndex  可写，有可能不够。
    void enshuredWriteableBytes(size_t len)
    {
        if(writeableBytes()<len)
        {
            //不够写，要扩容
            makeSpace(len);
        }
    }

    //把要添加的数据放到可写缓冲区中
    //把data，data+len的内容放到writable缓冲区中
    void  append(const char * data,size_t len)
    {
        enshuredWriteableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex_+=len;
    }

    char* beginWrite()
    {
        return begin()+writerIndex_;
    }

    const  char* beginWrite()const
    {
        return begin()+writerIndex_;
    }

    //从fd上读数据
    ssize_t readFd(int fd,int * saveErrno);
    //从fd上写数据
    ssize_t writeFd(int fd,int * saveErrno);

private:
    //返回缓冲区起始地址
    char* begin()
    {
        // vector首元素地址。
        return &(*buffer_.begin());
    }

    const char * begin() const
    {
        return &(*buffer_.begin());
    }

    //扩容
    void makeSpace(size_t len)
    {
        if(writeableBytes()+perendableBytes()<len+kCheapPrepend)
        {
            buffer_.resize(writerIndex_+len);
        }
        else{
            //空间够，但有碎片。要挪一下数据。
            /*
            已读 | 未读 | 空闲
                  |
                  V
            未读|   空    闲

            */
            size_t readable= readableBytes();
            std::copy(begin()+readerIndex_,
                        begin()+writerIndex_,
                        begin()+kCheapPrepend);
            readerIndex_ =kCheapPrepend;
            writerIndex_ = readerIndex_+readable;

            
        }

    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};


