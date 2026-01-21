//
// Created by OO on 2026/1/21.
//

#ifndef WEBSERVER_HTTPCONN_H
#define WEBSERVER_HTTPCONN_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include "Epoller.h"

class HttpConn
{
public:
    HttpConn();
    ~HttpConn();

    //初始化
    void init(int fd,const sockaddr_in& addr);

    //关闭
    void Close();

    //由线程池调用
    void process();

    //静态成员：所有HttpConn共享一个epoll管家
    static int userCount;
    static int epollFd;

private:
    int fd_;
    struct sockaddr_in addr_;
    //每个连接自带两个缓冲区
    //readBuffer_：存放socket读出来的raw数据
    //writeBuffer_：存放准备发给用户的HTML数据
    //后面专门封装Buffer类
};


#endif //WEBSERVER_HTTPCONN_H