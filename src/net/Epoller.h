//
// Created by OO on 2026/1/21.
//

#ifndef WEBSERVER_EPOLLER_H
#define WEBSERVER_EPOLLER_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <errno.h>


class Epoller
{
public:
    //构造函数：创建epoll实例
    explicit Epoller(int maxEvent = 1024);

    //析构函数：释放资源
    ~Epoller();

    //核心动作
    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);

    int GetEpollFd() const;

    //等待事件
    int Wait(int timeoutMs = -1);

    //获取发生事件的fd
    int GetEventFd(size_t i) const;

    //获取发生事件的类型（EPOLLIN等）
    uint32_t GetEvents(size_t i) const;

private:
    int epollFd_;                       //真正的epoll句柄
    std::vector<struct epoll_event> events_;        //用来存放内核返回的就绪事件
};


#endif //WEBSERVER_EPOLLER_H