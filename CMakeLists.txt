
cmake_minimum_required(VERSION 3.10)
project(MyLLVMLang)

find_package(LLVM REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 17)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(mycompiler source/main.cpp)

target_link_libraries(mycompiler LLVM)
