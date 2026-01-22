#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cerrno>
#include "ThreadPool.h"
#include "Epoller.h"
#include "SafeQueue.h"

using namespace std;


// ==========================================
// 3. 工具函数
// ==========================================
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 重点：重新激活 EPOLLONESHOT 屏蔽的 FD
void reset_oneshot(int epfd, int fd) {
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

// 重点：贪婪读取 (ET模式必须一次读干净)
bool read_all(int fd, string& res) {
    char buf[1024];
    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n > 0) {
            res.append(buf, n);
        } else if (n == 0) {
            return false; // 对方关了
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return true; // 读完了
            return false; // 出错
        }
    }
}

// ==========================================
// 4. 业务逻辑 (工人执行)
// ==========================================
void handle_client(int conn_fd, int epfd) {
    string request;
    if (read_all(conn_fd, request)) {
        // 极简 HTTP 解析与响应
        string body = "<h1>C++ High Performance Server V2.0</h1>";
        string response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html\r\n"
                          "Content-Length: " + to_string(body.size()) + "\r\n"
                          "\r\n" + body;

        send(conn_fd, response.c_str(), response.size(), 0);

        // 重点：处理完业务，重新激活该 FD 的监听
        reset_oneshot(epfd, conn_fd);
    } else {
        // 对方断开或出错，直接关闭，不 MOD
        close(conn_fd);
    }
}

// ==========================================
// 5. 主循环 (Reactor)
// ==========================================
int main() {
    Epoller epoller(1024);
    int port = 9090; // 使用 9090 避免冲突
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listen_fd, (sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 1024);
    set_nonblocking(listen_fd);

    epoller.AddFd(listen_fd,EPOLLIN | EPOLLET);
    ThreadPool pool(8); // 8个工人
    cout << "Server started on port " << port << "..." << endl;

    while (true) {
        int eventCnt = epoller.Wait(-1);
        for (int i = 0; i < eventCnt; ++i) {
            int fd = epoller.GetEventFd(i);

            if (fd == listen_fd) {
                // 重点：贪婪 Accept (解决 1.45 req/s 的关键)
                while (true) {
                    struct sockaddr_in client{};
                    socklen_t len = sizeof(client);
                    int conn_fd = accept(listen_fd, (sockaddr*)&client, &len);
                    if (conn_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        break;
                    }
                    set_nonblocking(conn_fd);
                    epoller.AddFd(conn_fd,EPOLLIN | EPOLLET | EPOLLONESHOT);
                }
            } else {
                // 业务数据到来
                int epoll_fd = epoller.GetEpollFd();
                pool.enqueue([fd, epoll_fd] {
                    handle_client(fd, epoll_fd);
                });
            }
        }
    }
    return 0;
}