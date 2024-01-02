find_path(MIMALLOC_INCLUDE_DIR
    NAMES mimalloc.h
    PATHS "${CMAKE_SOURCE_DIR}/sdk/include/mimalloc"
    NO_DEFAULT_PATH
)

find_file(MIMALLOC_LIBRARY_RELEASE
    NAMES mimalloc-static.lib
    PATHS "${CMAKE_SOURCE_DIR}/sdk/libraries/${ARCH_TYPE}"
    NO_DEFAULT_PATH
)

find_file(MIMALLOC_LIBRARY_DEBUG
    NAMES mimalloc-static-debug.lib
    PATHS "${CMAKE_SOURCE_DIR}/sdk/libraries/${ARCH_TYPE}"
    NO_DEFAULT_PATH
)

mark_as_advanced(
    MIMALLOC_INCLUDE_DIR
    MIMALLOC_LIBRARY_RELEASE
    MIMALLOC_LIBRARY_DEBUG
)

add_library(mimalloc_mimalloc STATIC IMPORTED GLOBAL)
add_library(mimalloc::mimalloc ALIAS mimalloc_mimalloc)

set_target_properties(mimalloc_mimalloc PROPERTIES
    IMPORTED_LOCATION_RELEASE "${MIMALLOC_LIBRARY_RELEASE}"
    IMPORTED_LOCATION_DEBUG "${MIMALLOC_LIBRARY_DEBUG}"
    INTERFACE_INCLUDE_DIRECTORIES "${MIMALLOC_INCLUDE_DIR}"
)
