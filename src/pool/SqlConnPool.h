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

    MYSQL *GetConn();          // 借一只铲子（连接）
    void FreeConn(MYSQL * conn); // 还一只铲子
    int GetFreeConnCount();    // 看看池子里还剩多少

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

    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_; // 信号量：用于阻塞等待空闲连接
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