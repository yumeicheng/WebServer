//
// Created by OO on 2026/1/23.
//

#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         // mkdir
#include "BlockQueue.h"
#include "Buffer.h"

class Log {
public:
    // 初始化日志实例（阻塞队列最大容量、日志保存路径、日志文件后缀）
    void init(int level, const char* path = "./log",
                const char* suffix =".log",
                int maxQueueCapacity = 1024);

    static Log* Instance();
    static void FlushLogThread(); // 异步写日志的线程函数

    void write(int level, const char *format, ...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_; }

private:
    Log();
    virtual ~Log();
    void AppendLogLevelTitle_(int level); // 添加日志等级前缀
    void AsyncWrite_(); // 真正写文件的逻辑

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000; // 单个日志文件最大行数

    const char* path_;
    const char* suffix_;

    int MAX_LINES_;
    int lineCount_;
    int toDay_;

    bool isOpen_;

    Buffer buff_;       // 临时缓冲区
    int level_;         // 日志等级
    bool isAsync_;      // 是否开启异步

    FILE* fp_;
    std::unique_ptr<BlockQueue<std::string>> deque_; // 阻塞队列
    std::unique_ptr<std::thread> writeThread_;       // 写线程
    std::mutex mtx_;
};

// 四个宏定义，以后用 LOG_INFO("msg") 就能打印
#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...)  do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...)  do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //LOG_H