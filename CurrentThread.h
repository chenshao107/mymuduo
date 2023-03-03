#pragma once
#include <unistd.h>
#include <sys/syscall.h>


namespace CurrentThread
{
    extern __thread int t_cachedTid;
    void cacheTid();
    inline int tid()
    {
        // __builtin_expect（EXP, N）说明 EXP == N的概率很大
        // 优化跳转指令
        if(__builtin_expect(t_cachedTid==0,0))
        {
            cacheTid();
        }
        return t_cachedTid;
    };
}