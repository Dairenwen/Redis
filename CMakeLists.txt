cmake_minimum_required(VERSION 3.10)
project(RedisClient LANGUAGES CXX)

# redis-plus-plus 当前安装版本使用 std::string_view/std::optional/std::variant 因此必须启用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PkgConfig 用于读取系统库的 .pc 文件；本项目中的 cmake/Findhiredis.cmake 会通过 hiredis.pc 补齐 hiredis 的 CMake 查找能力。
find_package(PkgConfig REQUIRED)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# hiredis 是 redis++ 底层依赖，jsoncpp 用于解析 .env 后保存为 Json::Value。
find_package(hiredis REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(redis++ REQUIRED)

# redis_test 是当前示例程序：读取 .env，连接 Redis，并执行 PING。
add_executable(redis_test src/test.cpp)

# 使用导入目标链接第三方库，可以自动携带 include 路径和链接参数。
target_link_libraries(redis_test PRIVATE
    redis++::redis++
    JsonCpp::JsonCpp
)
