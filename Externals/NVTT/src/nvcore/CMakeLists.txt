option(NVCORE_SHARED "Build nvcore as shared library" OFF)

add_subdirectory(poshlib)

if (NVCORE_SHARED)
    add_library(nvcore SHARED)

    target_compile_definitions(nvcore
        PRIVATE
        NVCORE_SHARED
    )
else()
    add_library(nvcore STATIC)
endif()

target_sources(nvcore
    PRIVATE
    nvcore.h
    Ptr.h
    BitArray.h
    Memory.h
    Memory.cpp
    Debug.h
    Debug.cpp
    Containers.h
    StrLib.h
    StrLib.cpp
    Stream.h
    StdStream.h
    TextReader.h
    TextReader.cpp
    TextWriter.h
    TextWriter.cpp
    Radix.h
    Radix.cpp
    Library.h
    Library.cpp
)

target_include_directories(nvcore
    PUBLIC
    "${CMAKE_SOURCE_DIR}/Externals/NVTT/src"
    "${CMAKE_SOURCE_DIR}/Externals/NVTT/include"
)

target_compile_definitions(nvcore
    PRIVATE
    NVCORE_EXPORTS
    HAVE_EXECINFO_H
    HAVE_SIGNAL_H
)

set_target_properties(nvcore PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

install(TARGETS nvcore LIBRARY
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
