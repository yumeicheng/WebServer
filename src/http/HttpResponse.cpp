//
// Created by OO on 2026/1/22.
//

#include "HttpResponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".png",   "image/png" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".gif",   "image/gif" },
    { ".css",   "text/css" },
    { ".js",    "text/javascript" },
    { ".zip",   "application/zip" },
    { ".pdf",   "application/pdf" },
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr;
    mmFileStat_ = { 0 };
};

HttpResponse::~HttpResponse() { UnmapFile(); }

void HttpResponse::MakeResponse(Buffer& buff) {

    std::string fullPath = srcDir_ + path_; // 拼接完整路径
    LOG_INFO("DEBUG: Trying to open file: %d", fullPath.c_str());

    if(stat(fullPath.c_str(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        LOG_INFO("DEBUG: stat failed!");
        code_ = 404;
    }

    // 检查资源是否存在
    if(stat((srcDir_ + path_).c_str(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) {
        code_ = 200;
    }

    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

void HttpResponse::AddStateLine_(Buffer& buff) {
    std::string status;
    if(code_ == 200) status = "OK";
    else if(code_ == 404) status = "Not Found";
    else if(code_ == 403) status = "Forbidden";
    else status = "Bad Request";
    buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer& buff) {
    buff.Append("Connection: ");
    if(isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buff.Append("close\r\n");
    }
    // 自动识别MIME类型
    std::string type = "text/plain";
    auto it = SUFFIX_TYPE.find(path_.substr(path_.find_last_of('.')));
    if(it != SUFFIX_TYPE.end()) {
        type = it->second;
    }
    buff.Append("Content-Type: " + type + "\r\n");
}

void HttpResponse::AddContent_(Buffer& buff) {
    int srcFd = open((srcDir_ + path_).c_str(), O_RDONLY);
    if(srcFd < 0) {
        std::string errorContent = "File NotFound!";
        buff.Append("Content-Length: " + std::to_string(errorContent.size()) + "\r\n\r\n");
        buff.Append(errorContent);
        return;
    }

    // 重点：将文件映射到内存地址 mmFile_
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        std::string errorContent = "File NotFound!";
        buff.Append("Content-Length: " + std::to_string(errorContent.size()) + "\r\n\r\n");
        buff.Append(errorContent);
        return;
    }
    mmFile_ = (char*)mmRet;
    close(srcFd);
    buff.Append("Content-Length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile() {
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

void HttpResponse::Init(const std::string& srcDir, std::string& path, bool isKeepAlive, int code){
    if(mmFile_) { UnmapFile(); }
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFile_ = nullptr;
    mmFileStat_ = { 0 };
}

