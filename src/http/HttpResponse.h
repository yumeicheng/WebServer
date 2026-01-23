//
// Created by OO on 2026/1/22.
//

#ifndef WEBSERVER_HTTPRESPONSE_H
#define WEBSERVER_HTTPRESPONSE_H

#include <unordered_map>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "Buffer.h"
#include "Log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer& buff); // 生成响应头并存入 Buffer
    void UnmapFile();               // 释放内存映射
    char* File() { return mmFile_; }
    size_t FileLen() const { return mmFileStat_.st_size; }

private:
    void AddStateLine_(Buffer& buff);
    void AddHeader_(Buffer& buff);
    void AddContent_(Buffer& buff);

    int code_;
    bool isKeepAlive_;
    std::string path_;
    std::string srcDir_;

    char* mmFile_;              // 内存映射地址
    struct stat mmFileStat_;    // 文件属性
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
};

#endif