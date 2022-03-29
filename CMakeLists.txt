project(k2)
cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")

add_subdirectory(lib)
add_subdirectory(src)