//
// Created by OO on 2026/1/22.
//

#include "Timer.h"

void HeapTimer::siftup_(size_t i) {
    while(i > 0) { // 只要不是根节点
        size_t j = (i - 1) / 2;
        if(heap_[j] < heap_[i]) break; // 父节点比我小，停止上浮
        SwapNode_(i, j);
        i = j;
    }
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

bool HeapTimer::siftdown_(size_t index, size_t n) {
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n) {
        if(j + 1 < n && heap_[j + 1] < heap_[j]) j++;
        if(heap_[i] < heap_[j]) break;
        SwapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    if(ref_.count(id)) { // 已存在则更新
        size_t i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if(!siftdown_(i, heap_.size())) siftup_(i);
    } else { // 新增
        size_t i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        siftup_(i);
    }
}

void HeapTimer::tick() {
    /* 清理堆顶过期的定时器 */
    while(!heap_.empty()) {
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
            break;
        }
        node.cb();
        del_(0);
    }
}

void HeapTimer::del_(size_t index) {
    size_t i = index;
    size_t n = heap_.size() - 1;
    if(i < n) {
        SwapNode_(i, n);
        if(!siftdown_(i, n)) siftup_(i);
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::clear()
{
    ref_.clear();
    heap_.clear();
}

// 获取下一次 epoll_wait 应该等待的时间
int HeapTimer::GetNextTick() {
    tick(); // 先清理一遍过期的
    if(heap_.empty()) { return -1; }
    // 计算堆顶（最近的定时器）距离现在还有多少毫秒
    int res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
    return res < 0 ? 0 : res;
}

void HeapTimer::doWork(int id) {
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb(); // 执行关闭连接的回调
    del_(i);   // 从堆中移除
}

void HeapTimer::adjust(int id, int timeout) {
    /* 1. 查找是否存在该 FD 的定时器 */
    if(heap_.empty() || ref_.count(id) == 0) {
        return;
    }

    /* 2. 更新过期时间点 */
    size_t i = ref_[id];
    heap_[i].expires = Clock::now() + MS(timeout);

    /* 3. 调整堆结构 */
    // 因为是延长时间，节点值变大，所以应该尝试向下调整（下沉）
    if(!siftdown_(i, heap_.size())) {
        // 如果下沉没动，说明它可能原本很小，现在依然比子节点小，
        // 虽然在“续命”场景下罕见，但为了代码严谨，可以尝试向上调整
        siftup_(i);
    }
}