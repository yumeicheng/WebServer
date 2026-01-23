cmake_minimum_required(VERSION 3.10)
project(WebServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

include_directories(src/base
                    src/net
                    src/http
                    src/pool
                    src/log)

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
        src/base/Timer.cpp
        src/base/Timer.h
        src/http/HttpResponse.cpp
        src/http/HttpResponse.h
        src/pool/SqlConnPool.cpp
        src/pool/SqlConnPool.h
        src/log/BlockQueue.h
        src/log/Log.cpp
        src/log/Log.h
)

target_link_libraries(WebServer pthread mysqlclient)  # Link with pthread and mysqlclient