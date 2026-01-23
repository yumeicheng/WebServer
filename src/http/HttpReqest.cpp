//
// Created by OO on 2026/1/21.
//

#include "HttpReqest.h"
#include <algorithm>

bool HttpReqest::parse(Buffer& buff)
{
    const char CRLF[] = "\r\n";
    //只要缓冲区有数据，且没有解析完，就一直循环
    while (buff.ReadableBytes() > 0 && state_ != FINISH)
    {
        //如果当前状态是解析BODY（比如POST请求）
        //BODY的长度由Content-Length决定，不依赖\r\n
        if (state_ == BODY)
        {
            //这里简单处理：将剩余的数据全部转化为body
            //实际需要根据header_中的Content-Length字段来判断
            ParseBody_(std::string(buff.Peek(),buff.ReadableBytes()));
            buff.Retrieve(buff.ReadableBytes());    //清空buff
            state_ = FINISH;
            break;
        }

        //对于REQUEST_LINE和HEADERS，必须寻找\r\n来区分行
        const char *readableEnd = buff.Peek() + buff.ReadableBytes();
        const char *lineEnd = std::search(buff.Peek(),readableEnd,CRLF,CRLF+2);

        //如果没有找到\r\n，说明数据不完整（半包），跳出循环，等待下一次epoll_wait读取
        if (lineEnd == readableEnd)
        {
            break;
        }

        //提取这一行数据（不包含\r\n）
        std::string line(buff.Peek(),lineEnd);

        //移动读指针：跳过当前行长度+2（\r\n）
        size_t lineLen = line.size();
        buff.Retrieve(lineLen+2);

        switch (state_)
        {
        case REQUEST_LINE:
            //如果请求解析行失败，直接返回false
            if (!ParseRequestLine_(line))return false;
            state_ = HEADERS;
            break;

        case HEADERS:
            if (line.empty())
            {
                //如果解析行是空行，说明头部结束了
                //判断进入的是BODY状态（POST）还是直接FINISH（GET）
                if (method_ == "POST" && header_.count("Content-Length"))
                {
                    state_ = BODY;
                }
                else
                {
                    state_ = FINISH;
                }
            }else
            {
                //不是空行，正常解析Key-Value
                ParseHeader_(line);
            }
            break;

        default:
            break;
        }
    }
    return true;    //数据暂时解析成功（可能已FINISH，也可能在等待更多数据）
}

void HttpReqest::ParseHeader_(const std::string& line)
{
    size_t pos1 = line.find(':');
    std::string key = line.substr(0,pos1);
    std::string value = line.substr(pos1+1,line.size()-pos1-1);
    header_[key] = value;
}

void HttpReqest::Init()
{
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpReqest::ParseRequestLine_(const std::string& line)
{
    //正则表达式解析：GET /index.html HTTP/1.1
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line,subMatch,patten))
    {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        return true;
    }
    return false;
}

void HttpReqest::ParseBody_(const std::string& line)
{
    if(line.empty()) { return; }

    // POST 数据通常是: user=admin&password=123456
    // 我们需要解析并存入 map
    std::string key, value;
    int num = 0;
    int n = line.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = line[i];
        switch (ch) {
        case '=':
            key = line.substr(j, i - j);
            j = i + 1;
            break;
        case '&':
            value = line.substr(j, i - j);
            j = i + 1;
            post_[key] = value; // 存入一个 unordered_map<string, string> post_
            break;
        default:
            break;
        }
    }
    if(post_.count(key) <= 0 && j < i) {
        value = line.substr(j, i - j);
        post_[key] = value;
    }
}

std::string HttpReqest::GetPost(const std::string& key) const {
    if(post_.count(key) > 0) return post_.at(key);
    return "";
}