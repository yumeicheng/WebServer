//
// Created by OO on 2026/1/22.
//

#include "SqlConnPool.h"

SqlConnPool::SqlConnPool() {
    useCount_ = 0;
    freeCount_ = 0;
}

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char* host, int port,
              const char* user, const char* pwd,
              const char* dbName, int connSize = 10) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; i++) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            // 错误处理...
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
        if (!sql) {
            // 这里的错误通常是用户名密码不对或数据库没开
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_); // 初始化信号量，数值为连接总数
}

MYSQL* SqlConnPool::GetConn() {
    MYSQL *sql = nullptr;
    if(connQue_.empty()){
        // 如果池子空了，这里会阻塞，直到有人还连接
        sem_wait(&semId_);
    }
    // 信号量减 1
    sem_wait(&semId_);

    std::lock_guard<std::mutex> locker(mtx_);
    sql = connQue_.front();
    connQue_.pop();
    return sql;
}

void SqlConnPool::FreeConn(MYSQL* sql) {
    assert(sql);
    std::lock_guard<std::mutex> locker(mtx_);
    connQue_.push(sql);
    sem_post(&semId_); // 信号量加 1，唤醒等待连接的线程
}

void SqlConnPool::ClosePool() {
    std::lock_guard<std::mutex> locker(mtx_);
    while(!connQue_.empty()) {
        auto item = connQue_.front();
        connQue_.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}