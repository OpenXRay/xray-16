project(xrLCUtil)

set(SRC_FILES
    #"ILevelCompilerLogger.hpp"
    #"LevelCompilerLoggerWindow.hpp"
    #"LevelCompilerLoggerWindow.cpp"
    "pch.cpp"
    "pch.cpp"
    "resource.h"
    "xrLCUtil.hpp"
    "xrLCUtil.cpp"
    "xrThread.hpp"
    "xrThread.cpp"
)

group_sources(SRC_FILES)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    xrCore
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DLEVEL_COMPILER
    -DXRLCUTIL_EXPORTS
    -D_USE_MATH_DEFINES
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
    "pch.hpp"
)
