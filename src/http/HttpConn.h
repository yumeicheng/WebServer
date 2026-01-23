//
// Created by OO on 2026/1/21.
//

#ifndef WEBSERVER_HTTPCONN_H
#define WEBSERVER_HTTPCONN_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <sys/uio.h>        //writev需要的头文件

#include "Buffer.h"
#include "HttpResponse.h"
#include "HttpReqest.h"
#include "Epoller.h"
#include "SqlConnPool.h"
#include "Log.h"

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

    //专门负责写数据
    //bool write();

    //供 main 函数调用发送数据
    ssize_t Write(int* saveErrno);

    //静态成员：所有HttpConn共享一个epoll管家
    static int userCount;
    static int epollFd;

private:
    int fd_;
    struct sockaddr_in addr_;
    //每个连接自带两个缓冲区
    Buffer readBuffer_;//readBuffer_：存放socket读出来的raw数据
    Buffer writeBuffer_;//writeBuffer_：存放准备发给用户的HTML数据
    HttpReqest request_;
    HttpResponse response_;

    struct iovec iov_[2];   //两个缓冲区，一个指向箭头，一个指向文件
    int iovCnt_;
};


#endif //WEBSERVER_HTTPCONN_H