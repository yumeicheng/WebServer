//
// Created by OO on 2026/1/21.
//

#ifndef WEBSERVER_BUFFER_H
#define WEBSERVER_BUFFER_H

#include <vector>
#include <string>
#include <atomic>
#include <iostream>
#include <cassert>
#include <filesystem>

class Buffer
{
public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    //可读字节数（待解析）
    size_t ReadableBytes() const {return writePos_ - readPos_;}

    //可写字节数（剩余空间）
    size_t WritableBytes() const {return buffer_.size() - writePos_;}

    //前面已读完、可回收的空间
    size_t PrependableBytes() const {return readPos_;}

    //获取当前读位置的指针
    const char* Peek() const {return &buffer_[readPos_];}

    //当前解析器读了len字节后，移动readPos_
    void Retrieve(size_t len)
    {
        assert(len <= ReadableBytes());
        readPos_ += len;
    }

    //当socket读入数据到Buffer后，移动writePos_
    void HasWritten(size_t len)
    {
        writePos_ += len;
    }

    //确保有足够的空间写数据
    void EnsureWritable(size_t len);

    //真正的读取接口：从socket读入
    ssize_t ReadFd(int fd, int *saveErrno);

    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);

    char* BeginWrite() { return &buffer_[writePos_]; }
    std::string RetrieveAllToStr();

    void RetrieveAll() {
        readPos_ = 0;
        writePos_ = 0;
    }

private:
    std::vector<char> buffer_;
    std::atomic<size_t>readPos_;                //已处理的数据
    std::atomic<size_t>writePos_;               //待解析的数据
};

#endif //WEBSERVER_BUFFER_H