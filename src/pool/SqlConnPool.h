//
// Created by OO on 2026/1/22.
//

#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <assert.h>

class SqlConnPool {
public:
    // 获取单例实例
    static SqlConnPool *Instance();

    MYSQL *GetConn();          // 获取一个连接
    void FreeConn(MYSQL * conn); // 释放一个连接回池子
    int GetFreeConnCount();    // 获取空闲连接数

    void Init(const char* host, int port,
              const char* user, const char* pwd,
              const char* dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool();
    ~SqlConnPool();

    int MAX_CONN_;
    int useCount_;
    int freeCount_;

    std::queue<MYSQL *> connQue_;               //连接队列
    std::mutex mtx_;                            //队列锁
    sem_t semId_;                               // 信号量：用于阻塞等待空闲连接
};

/* RAII 机制：利用局部对象的生命周期来管理连接 */
class SqlConnRAII {
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool *connpool) {
        assert(connpool);
        *sql = connpool->GetConn();
        sql_ = *sql;
        connpool_ = connpool;
    }

    ~SqlConnRAII() {
        if(sql_) { connpool_->FreeConn(sql_); }
    }

private:
    MYSQL *sql_;
    SqlConnPool* connpool_;
};

#endif // SQLCONNPOOL_H