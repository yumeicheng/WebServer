//
// Created by OO on 2026/1/21.
//

#include "HttpConn.h"

int HttpConn::userCount = 0;
int HttpConn::epollFd = -1;
HttpConn::HttpConn()
{

}

HttpConn::~HttpConn()
{

}

void HttpConn::init(int fd, const sockaddr_in& addr)
{
    fd_ = fd;
    addr_ = addr;
    userCount++;

    //readBuffer
}

