//
// Created by OO on 2026/1/22.
//

#ifndef WEBSERVER_TIMER_H
#define WEBSERVER_TIMER_H

#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <algorithm>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode
{
    int id;                     //对应连接的FD
    TimeStamp expires;          //过期时间点
    TimeoutCallBack cb;         //超时回调函数（比如关闭连接）

    //小根堆比较规则
    bool operator<(const TimerNode& t) const
    {
        return expires < t.expires;
    }
};

class HeapTimer
{
public:
    HeapTimer(){heap_.reserve(64);}
    ~HeapTimer(){ clear(); }

    //调整指定FD的过期时间（心跳机制）
    void adjust(int id, int newTimeout);

    //添加定时器
    void add(int id, int timeout, const TimeoutCallBack &cb);

    //手动触发回调并删除
    void doWork(int id);

    //清理所有超时的节点
    void tick();

    //获取下一次超时等待的毫秒数（给epoll_wait用）
    int GetNextTick();

    void clear();

private:
    void del_(size_t i);
    void siftup_(size_t i);
    bool siftdown_(size_t index,size_t n);
    void SwapNode_(size_t i,size_t j);

    std::vector<TimerNode> heap_;
    //映射fd到堆索引，方便O（1）查找
    std::unordered_map<int,size_t> ref_;
};


#endif //WEBSERVER_TIMER_H