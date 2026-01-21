//
// Created by OO on 2026/1/21.
//

#include "Epoller.h"

Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)),events_(maxEvent)
{

}

Epoller::~Epoller()
{
    close(epollFd_);
}

bool Epoller::AddFd(int fd, uint32_t events)
{
    if (fd < 0)return false;
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

int Epoller::Wait(int timeoutMs)
{
    //核心系统调用
    //参数1：epoll句柄
    //参数2：接收事件的数组首地址
    //参数3：最多接受事件数
    //参数4：超时时间
    int eventsCnt = epoll_wait(epollFd_,&events_[0],static_cast<int>(events_.size()), timeoutMs);
    return eventsCnt;
}

int Epoller::GetEventFd(size_t i) const
{
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const
{
    return events_[i].events;
}

bool Epoller::ModFd(int fd, uint32_t events)
{
    if (fd < 0)return false;
    struct epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd)
{
    if (fd < 0)return false;
    return epoll_ctl(epollFd_,EPOLL_CTL_DEL,fd,nullptr);
}

int Epoller::GetEpollFd() const
{
    return epollFd_;
}
