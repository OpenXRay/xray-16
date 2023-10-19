add_library(xrLCUtil SHARED)

target_sources(xrLCUtil PRIVATE
    #ILevelCompilerLogger.hpp
    #LevelCompilerLoggerWindow.hpp
    #LevelCompilerLoggerWindow.cpp
    pch.cpp
    pch.cpp
    resource.h
    xrLCUtil.hpp
    xrLCUtil.cpp
    xrThread.hpp
    xrThread.cpp
)

target_include_directories(xrLCUtil
    PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(xrLCUtil
    PRIVATE
    xrCore
)

target_compile_definitions(xrLCUtil
    PRIVATE
    LEVEL_COMPILER
    XRLCUTIL_EXPORTS
    _USE_MATH_DEFINES
)

set_target_properties(xrLCUtil PROPERTIES
    PREFIX ""
)

target_precompile_headers(xrLCUtil
    PRIVATE
    pch.hpp
)
