//
// Created by OO on 2026/1/21.
//

#include "HttpConn.h"
bool UserVerify(const std::string &name, const std::string &pwd);
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
    request_.Init();        //重置请求解析状态
}

void HttpConn::process() {
    int saveErrno = 0;
    // 1. 读取socket数据
    ssize_t n = readBuffer_.ReadFd(fd_, &saveErrno);
    if(n <= 0 && saveErrno != EAGAIN) {
        Close();
        return;
    }

    // 2. 解析请求并生成响应
    if(request_.parse(readBuffer_)) {
        if(request_.state() == HttpReqest::FINISH) {

            std::string path = request_.path();

            // 如果请求的是登录
            if(path == "/login") {

                // 获取解析出来的用户名密码
                std::string name = request_.GetPost("user");
                std::string pwd = request_.GetPost("password");
                std::cout << "DEBUG: Login attempt - User: " << name << " Pwd: " << pwd << std::endl;

                if(UserVerify(name, pwd)) {
                    path = "/welcome.html"; // 登录成功，去欢迎页
                } else {
                    path = "/error.html";   // 失败，去错误页
                }
            }else if (path == "/" || path == "/index.html") path = "/index.html";

            response_.Init("../resources", path, true, 200);
            response_.MakeResponse(writeBuffer_);

            // 重置请求解析器，准备处理同一个连接的下一个请求
            request_.Init();

            // 3. 准备 writev 的 iovec 参数
            // 第一个缓冲区：writeBuffer_ 里的响应头
            iov_[0].iov_base = const_cast<char*>(writeBuffer_.Peek());
            iov_[0].iov_len = writeBuffer_.ReadableBytes();
            iovCnt_ = 1;

            // 第二个缓冲区：mmap 映射的文件内容
            if(response_.FileLen() > 0 && response_.File()) {
                iov_[1].iov_base = response_.File();
                iov_[1].iov_len = response_.FileLen();
                iovCnt_ = 2;
            }

            // 4. 重点：改变 Epoll 关注的事件为 EPOLLOUT
            // 意味着：数据准备好了，一旦内核发送缓冲区有空间，就通知我写
            struct epoll_event ev = {0};
            ev.data.fd = fd_;
            ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
            epoll_ctl(epollFd, EPOLL_CTL_MOD, fd_, &ev);
        }
    } else {
        // 解析失败，重置监听
        struct epoll_event ev = {0};
        ev.data.fd = fd_;
        ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        epoll_ctl(epollFd, EPOLL_CTL_MOD, fd_, &ev);
    }
}

void HttpConn::Close()
{
    if (fd_ != -1)
    {
        epoll_ctl(epollFd,EPOLL_CTL_DEL,fd_,nullptr);
        close(fd_);
        fd_ = -1;
        userCount--;
    }
}

// bool HttpConn::write() {
//     ssize_t temp = -1;
//     while(true) {
//         // 调用 writev 集中写
//         temp = writev(fd_, iov_, iovCnt_);
//         if(temp <= 0) {
//             if(errno == EAGAIN) return true; // 缓冲区满了，等下次写事件
//             return false; // 真的出错了
//         }
//
//         // 更新 iov 状态（处理大文件一次写不完的情况）
//         if(static_cast<size_t>(temp) >= iov_[0].iov_len) {
//             temp -= iov_[0].iov_len;
//             iov_[0].iov_len = 0;
//             iov_[1].iov_base = (char*)iov_[1].iov_base + temp;
//             iov_[1].iov_len -= temp;
//         } else {
//             iov_[0].iov_base = (char*)iov_[0].iov_base + temp;
//             iov_[0].iov_len -= temp;
//         }
//
//         if(iov_[1].iov_len == 0) break; // 数据全部发完了
//     }
//     return true;
// }

ssize_t HttpConn::Write(int* saveErrno) {
    ssize_t len = -1;
    do {
        // 集中写：一次性发送 iov_[0] 和 iov_[1]
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }

        // 更新发送进度
        if(static_cast<size_t>(len) >= iov_[0].iov_len) {
            // 头发完了，开始发文件
            size_t remain = len - iov_[0].iov_len;
            iov_[0].iov_len = 0;
            iov_[1].iov_base = (char*)iov_[1].iov_base + remain;
            iov_[1].iov_len -= remain;
        } else {
            // 连头发都没发完
            iov_[0].iov_base = (char*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
        }
    } while(iovCnt_ > 1 && iov_[1].iov_len > 0); // 直到发完为止

    // 如果全部发完了，重置缓冲区，准备接收下一次请求
    if(iov_[1].iov_len == 0) {
        writeBuffer_.RetrieveAll(); // 你需要给 Buffer 加个清空函数
        response_.UnmapFile();      // 释放 mmap
        return 0;
    }
    return len;
}

bool UserVerify(const std::string &name, const std::string &pwd) {
    if(name == "" || pwd == "") { return false; }

    MYSQL* sql;
    // 使用 RAII 机制从池中借一个连接
    SqlConnRAII(&sql, SqlConnPool::Instance());

    char order[256] = { 0 };
    // SQL 查询语句
    snprintf(order, 256, "SELECT password FROM user WHERE username='%s' LIMIT 1", name.c_str());

    if(mysql_query(sql, order)) {
        return false;
    }

    MYSQL_RES *res = mysql_store_result(sql);
    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        std::string password(row[0]);
        if(pwd == password) {
            mysql_free_result(res);
            return true; // 密码匹配成功！
        }
    }
    mysql_free_result(res);
    return false;
}