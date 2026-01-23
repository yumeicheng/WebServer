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
#include "SqlConnPool.h"
#include "Log.h"

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
    //初始化日志：等级1（INFO），存放路径./log，后缀.log，队列容量1024
    Log::Instance()->init(3,"./log",".log",1024);

    LOG_INFO("==================Server init=========================");
    int port = 9090;

    Epoller epoller(1024);
    ThreadPool pool(8);
    HeapTimer timer;

    vector<HttpConn> users(MAX_FD);
    HttpConn::epollFd = epoller.GetEpollFd();

    SqlConnPool::Instance()->Init("localhost", 3306, "root", "123456", "webdb", 10);

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

    LOG_INFO("Server started on port %d",port);

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

                    LOG_INFO("New Client connected, fd: %d", conn_fd);
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
            else if (events & EPOLLOUT) {
                timer.adjust(fd, 60000);
                pool.enqueue([&users, fd] {
                    int error = 0;
                    ssize_t ret = users[fd].Write(&error);

                    if(ret < 0) {
                        // 真的出错了（比如 EPIPE），直接关闭
                        if(error != EAGAIN) {
                            users[fd].Close();
                        } else {
                            // 缓冲区满了，下次再写
                            struct epoll_event ev = {0};
                            ev.data.fd = fd;
                            ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
                            epoll_ctl(HttpConn::epollFd, EPOLL_CTL_MOD, fd, &ev);
                        }
                    }
                    else if(ret == 0) {
                        // 【关键修改】返回 0 代表数据全部写完了！
                        // 为了压测，我们发完就关（短连接），不要再监听 EPOLLIN 了
                        users[fd].Close();
                    }
                    else {
                        // ret > 0 但没返回 0，说明还没写完（大文件），继续注册写事件
                        // (注意：你的 Write 实现里如果一次写完返回的是 0，所以这里通常不会进，但为了保险起见)
                        struct epoll_event ev = {0};
                        ev.data.fd = fd;
                        ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
                        epoll_ctl(HttpConn::epollFd, EPOLL_CTL_MOD, fd, &ev);
                    }
                });
            }
        }
    }
    return 0;
}