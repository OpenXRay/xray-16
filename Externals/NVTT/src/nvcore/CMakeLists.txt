project(nvcore)

set(NVCORE_SHARED OFF)

# XXX: check if poshlib is needed?
add_subdirectory(poshlib)

set(SRC_FILES
    "nvcore.h"
    "Ptr.h"
    "BitArray.h"
    "Memory.h"
    "Memory.cpp"
    "Debug.h"
    "Debug.cpp"
    "Containers.h"
    "StrLib.h"
    "StrLib.cpp"
    "Stream.h"
    "StdStream.h"
    "TextReader.h"
    "TextReader.cpp"
    "TextWriter.h"
    "TextWriter.cpp"
    "Radix.h"
    "Radix.cpp"
    "Library.h"
    "Library.cpp"
)

group_sources(SRC_FILES)

if (NVCORE_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
else()
    add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Externals/NVTT/src
    ${CMAKE_SOURCE_DIR}/Externals/NVTT/include
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    -DNVCORE_EXPORTS
    $<$<BOOL:NVCORE_SHARED>:-DNVCORE_SHARED=1>
)

if (UNIX)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_DL_LIBS} # XXX: why?
    )
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
