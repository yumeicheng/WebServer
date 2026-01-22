#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "ThreadPool.h"
#include "Epoller.h"
#include "HttpConn.h"
#include "Timer.h"

using namespace std;

const int MAX_FD = 65535;

void CloseConn(HttpConn *user) {
    if (user) {
        user->Close();
    }
}

// !!! 将文件描述符设为非阻塞（这是 ET 模式的硬性要求）
int SetNonBlocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int main() {
    int port = 9090;

    Epoller epoller(1024);
    ThreadPool pool(8);
    HeapTimer timer;

    vector<HttpConn> users(MAX_FD);
    HttpConn::epollFd = epoller.GetEpollFd();

    // 1. 创建监听 Socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0) {
        perror("socket error");
        return -1;
    }

    // 2. 端口复用
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 绑定与监听
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if(bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind error");
        return -1;
    }

    if(listen(listen_fd, 5) < 0) {
        perror("listen error");
        return -1;
    }

    // !!! 关键修改点 1：必须先设为非阻塞，再加入 epoll
    SetNonBlocking(listen_fd);
    epoller.AddFd(listen_fd, EPOLLIN | EPOLLET);

    cout << "Server started on port " << port << "..." << endl;

    while (true) {
        int timeMS = timer.GetNextTick();
        int eventCnt = epoller.Wait(timeMS);

        for (int i = 0; i < eventCnt; ++i) {
            int fd = epoller.GetEventFd(i);
            uint32_t events = epoller.GetEvents(i);

            if (fd == listen_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                // !!! 关键修改点 2：ET模式必须循环 accept 直到返回 EAGAIN
                while(true) {
                    int conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);
                    if(conn_fd <= 0) break;

                    if(conn_fd >= MAX_FD) {
                        close(conn_fd);
                        continue;
                    }

                    // !!! 关键修改点 3：新连接也必须设为非阻塞
                    SetNonBlocking(conn_fd);

                    users[conn_fd].init(conn_fd, client_addr);
                    epoller.AddFd(conn_fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
                    timer.add(conn_fd, 60000, std::bind(CloseConn, &users[conn_fd]));

                    cout << "New Client connected, fd: " << conn_fd << endl;
                }
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                timer.doWork(fd);
            }
            else if (events & EPOLLIN) {
                timer.adjust(fd, 60000);
                pool.enqueue([&users, fd] {
                    users[fd].process();
                });
            }
            else if (events & EPOLLOUT)
            {
                timer.adjust(fd, 60000);
                pool.enqueue([&users, fd] {
                    int error = 0;
                    if(users[fd].Write(&error) < 0) {
                        users[fd].Close();
                    } else {
                        // 写完之后，通常我们要重新监听读事件（长连接）
                        // 或者直接 Close（短连接）
                        // 这里我们先重新监听读
                        struct epoll_event ev = {0};
                        ev.data.fd = fd;
                        ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                        epoll_ctl(HttpConn::epollFd, EPOLL_CTL_MOD, fd, &ev);
                    }
                });
            }
        }
    }
    return 0;
}