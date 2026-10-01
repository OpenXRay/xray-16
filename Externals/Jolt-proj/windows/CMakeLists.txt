cmake_minimum_required(VERSION 3.24)
project(XRayJoltDependency LANGUAGES CXX)
add_subdirectory(.. JoltProject)
set_target_properties(Jolt PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
