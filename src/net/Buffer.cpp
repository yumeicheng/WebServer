//
// Created by OO on 2026/1/21.
//

#include "Buffer.h"
#include <sys/uio.h>

Buffer::Buffer(int initBuffSize):buffer_(initBuffSize),readPos_(0),writePos_(0)
{

}

void Buffer::EnsureWritable(size_t len)
{
    if (WritableBytes() < len)
    {
        //如果剩余空间足够+前面腾出的已读空间足够，就移动数据
        if (WritableBytes() + PrependableBytes() >= len)
        {
            size_t readable = ReadableBytes();
            std::copy(buffer_.begin() + readPos_, buffer_.begin() + writePos_,buffer_.begin());
            readPos_ = 0;
            writePos_ = readable;
        }
        else
        {
            //否则扩容
            buffer_.resize(writePos_ + len+1);
        }
    }
}

ssize_t Buffer::ReadFd(int fd, int* saveErrno)
{
    char extrabuf[65536];       //栈上缓冲区，防止初始Buffer过小
    struct iovec vec[2];
    const size_t writable = WritableBytes();

    vec[0].iov_base = &buffer_[writePos_];
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const ssize_t len = readv(fd, vec, 2);

    if (len < 0)
    {
        *saveErrno = errno;
    }else if (static_cast<size_t>(len) <= writable)
    {
        writePos_ += len;
    }else
    {
        writePos_ = buffer_.size();
        //将溢出的数据append到buffer中（会自动触发扩容）
        size_t extraLen = len - writable;
        EnsureWritable(extraLen);
        std::copy(extrabuf,extrabuf + extraLen, buffer_.begin()+writePos_);
        writePos_ += extraLen;
    }
    return len;
}

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWritable(len); // 确保空间足够，不够会自动扩容或平移
    std::copy(str, str + len, buffer_.begin() + writePos_);
    writePos_ += len;
}