//
// Created by OO on 2026/1/21.
//

#include "ThreadPool.h"

ThreadPool::ThreadPool(int numThreads)
{
    for (int i = 0; i < numThreads; i++)
    {
        workers.emplace_back([this] {
            while (true)
            {
                std::function<void()> task;
                tasks.wait_and_pop(task);
                if (!task) return; // 毒药丸
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    for (size_t i = 0; i < workers.size(); i++)
    {
        tasks.push(nullptr); // 毒药丸
    }
    for (auto& worker : workers)
    {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task)
{
    tasks.push(std::move(task));
}
