cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(ppdbscan FullyPPdbscan.cpp)

target_link_libraries(ppdbscan helib)
target_link_libraries(ppdbscan ntl)
target_link_libraries(ppdbscan pthread)
