//
// Created by OO on 2026/1/21.
//

#ifndef WEBSERVER_SAFEQUEUE_H
#define WEBSERVER_SAFEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class SafeQueue {
    std::queue<T> q;
    std::mutex mtx;
    std::condition_variable cv;
public:
    void push(T val) {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(move(val));
        cv.notify_one();
    }
    void wait_and_pop(T& val) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !q.empty(); });
        val = move(q.front());
        q.pop();
    }
};
#endif //WEBSERVER_SAFEQUEUE_H