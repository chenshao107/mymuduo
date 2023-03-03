#include "Logger.h"
#include <iostream>
#include "Timestamp.h"

//获得日志唯一的实例对象
Logger& Logger::instance()
{
    static Logger logger;
    return logger;
};
//设置日志级别
void Logger::setLogLevel(int level)
{
    logLevel_=level;
};
//写日志
void Logger::log(std::string msg)
{
    switch (logLevel_)
    {
    case INFO/* constant-expression */:
        std::cout<<"[INFO]";
        break;
    case ERROR/* constant-expression */:
        std::cout<<"[ERROR]";
        break;
    case FATAL/* constant-expression */:
        std::cout<<"[FATAL]";
        break;
    case DEBUG/* constant-expression */:
        std::cout<<"[DEBUG]";
        break;
    default:
        break;
    }    
    //打印时间
    std::cout<<Timestamp::now().toString()<<" : "<<msg<<std::endl;
};