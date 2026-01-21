//
// Created by OO on 2026/1/21.
//

#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H
#include <vector>
#include <thread>
#include <functional>
#include "SafeQueue.h"

class ThreadPool
{
private:
    SafeQueue<std::function<void()>> tasks;
    std::vector<std::thread> workers;

public:
    ThreadPool(int numThreads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);
};


#endif //WEBSERVER_THREADPOOL_H