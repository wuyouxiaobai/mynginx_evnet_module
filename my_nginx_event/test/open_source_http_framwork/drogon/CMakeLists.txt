cmake_minimum_required(VERSION 3.5)
project(hello_drogon)

set(CMAKE_CXX_STANDARD 17)

find_package(Drogon REQUIRED)

add_executable(hello_drogon main.cpp)
target_link_libraries(hello_drogon PRIVATE Drogon::Drogon)
