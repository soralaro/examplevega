/*********************************************************** 
* Date: 2016-06-30 
* 
* Author: 牟韵 
* 
* Email: mouyun1115@163.com 
* 
* Module: 计时器 
* 
* Brief: 
* 
* Note: 过长的时间间隔可能导致变量溢出 
* 
* CodePage: UTF-8 
************************************************************/ 
#ifndef __ZFZ_TIMER_HPP_BY_MOUYUN_2014_09_28__
#define __ZFZ_TIMER_HPP_BY_MOUYUN_2014_09_28__

#include <sys/time.h>

namespace zfz
{

class Timer
{
public:
    Timer() { reset(); }
    ~Timer() {}

private:
    Timer(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer operator=(const Timer&) = delete;

public:
    void reset()
    {
        gettimeofday(&time_begin_, nullptr);
    }

    long tell_ms()
    {
        gettimeofday(&time_end_, nullptr);
        return ((time_end_.tv_sec - time_begin_.tv_sec) * 1000 + (time_end_.tv_usec - time_begin_.tv_usec) / 1000);
    }

    long tell_us()
    {
        gettimeofday(&time_end_, nullptr);
        return ((time_end_.tv_sec - time_begin_.tv_sec) * 1000000 + (time_end_.tv_usec - time_begin_.tv_usec));
    }

private:
    struct timeval time_begin_;
    struct timeval time_end_;
}; // class Timer

} // namespace zfz

#endif