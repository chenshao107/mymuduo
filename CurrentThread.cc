#include "CurrentThread.h"




namespace CurrentThread
{
    __thread int t_cachedTid=0;
    
    void cacheTid()
    {
        if(t_cachedTid==0)
        {
            //通过liunx系统调用 获得当前线程id
            t_cachedTid=static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}