cmake_minimum_required(VERSION 3.10)

project(nvtt)

add_definitions(-DHAVE_SIGNAL_H -DHAVE_EXECINFO_H)

add_subdirectory(src/nvcore)
add_subdirectory(src/nvimage)
add_subdirectory(src/nvmath)
add_subdirectory(src/nvtt)
