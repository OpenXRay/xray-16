cmake_minimum_required(VERSION 3.10)

project(cximage)

set(CMAKE_CXX_STANDARD 17)

set(SRC_FILES
    "xfile.h"
    "ximacfg.cpp"
    "ximacfg.h"
    "ximadef.h"
    "ximaenc.cpp"
    "ximaexif.cpp"
    "ximage.cpp"
    "ximage.h"
    "ximainfo.cpp"
    "ximaint.cpp"
    "ximaiter.h"
    "ximajpg.cpp"
    "ximajpg.h"
    "ximalpha.cpp"
    "ximalyr.cpp"
    "ximapal.cpp"
    "ximasel.cpp"
    "ximath.cpp"
    "ximath.h"
    "xiofile.h"
    "xmemfile.cpp"
    "xmemfile.h"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Externals
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    jpeg
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DCXIMAGE_BUILD
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    POSITION_INDEPENDENT_CODE ON
)
