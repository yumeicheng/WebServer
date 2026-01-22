//
// Created by OO on 2026/1/21.
//

#include "HttpReqest.h"

bool HttpReqest::parse(Buffer& buff)
{
    const char CRLF[] = "\r\n";
    while (buff.ReadableBytes() && state_ != FINISH)
    {
        //1、在buffer中寻找 \r\n
        const char* lineEnd = std::search(buff.Peek(), buff.BeginWrite(),CRLF,CRLF+2);

        //2、将数据转化为字符串，方便后续处理
        std::string line(buff.Peek(),lineEnd);

        switch (state_)
        {
        case REQUEST_LINE:
            if (!ParseRequestLine_(line))return false;
            state_ = HEADERS;
            break;
        case HEADERS:
            ParseHeader_(line);
            if (buff.ReadableBytes() <= 2)
            {
                state_ = FINISH;
            }
            break;
        }
        //处理完这一行，必须移动Buffer的读取标
        //游标应该跳过这一行加上那两个\r\n的长度
        buff.RetrieveUntil(lineEnd + 2);
    }
    return true;
}

void HttpReqest::ParseHeader_(const std::string& line)
{
    size_t pos1 = line.find(':');
    std::string key = line.substr(0,pos1);
    std::string value = line.substr(pos1+1,line.size()-pos1-1);
    header_[key] = value;
}
