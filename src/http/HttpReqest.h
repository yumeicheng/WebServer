//
// Created by OO on 2026/1/21.
//

#ifndef WEBSERVER_HTTPREQEST_H
#define WEBSERVER_HTTPREQEST_H

#include "Buffer.h"
#include <unordered_map>
#include <regex>


class HttpReqest
{
public:
    enum PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    };

    PARSE_STATE state() const { return state_; }

    HttpReqest() {Init();}
    void Init();

    //解析入口
    bool parse(Buffer& buff);

    //获取解析结果
    std::string path() const {return path_;}
    std::string method() const {return method_;}

    // 通过 key 获取 POST 里的值 (比如输入 "user" 返回 "admin")
    std::string GetPost(const std::string& key) const;

private:
    bool ParseRequestLine_(const std::string& line);
    void ParseHeader_(const std::string& line);
    void ParseBody_(const std::string& line);

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
};


#endif //WEBSERVER_HTTPREQEST_H