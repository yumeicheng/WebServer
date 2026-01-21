cmake_minimum_required(VERSION 3.10)
project(WebServer)

set(CMAKE_CXX_STANDARD 20)

include_directories(src/base
                    src/net)

add_executable(WebServer main.cpp
        src/base/SafeQueue.h
        src/base/ThreadPool.cpp
        src/base/ThreadPool.h
        src/net/Epoller.cpp
        src/net/Epoller.h
        src/http/HttpConn.cpp
        src/http/HttpConn.h
        src/net/Buffer.cpp
        src/net/Buffer.h
        src/http/HttpReqest.cpp
        src/http/HttpReqest.h
)

target_link_libraries(WebServer pthread)