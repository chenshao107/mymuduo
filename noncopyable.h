#pragma once

/*
noncopyable被继承后，派生类对象可以正常析构，但不可拷贝和拷贝构造。
*/
class noncopyable
{
public:
    noncopyable(const noncopyable&)=delete;
    noncopyable& operator=(const noncopyable&)=delete;
protected:
    noncopyable()=default;
    ~noncopyable()=default;
private:
    
};