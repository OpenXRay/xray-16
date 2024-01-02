# TODO add new info comment

find_path(LZO_INCLUDE_DIR
    NAMES lzo/lzo1x.h
    PATHS
        /usr/local
        "${CMAKE_SOURCE_DIR}/sdk/include"
    NO_DEFAULT_PATH
)

# TODO check if works correctly on all systems
find_library(LZO_LIBRARY
    NAMES lzo
    PATHS
    # /usr/local/lib64
    # /usr/local/lib
    #PATH_SUFFIXES lib64 lib
    "${CMAKE_SOURCE_DIR}/sdk/libraries/${ARCH_TYPE}"
    #NO_DEFAULT_PATH
)

add_library(LZO_LZO STATIC IMPORTED GLOBAL)
add_library(LZO::LZO ALIAS LZO_LZO)

set_target_properties(LZO_LZO PROPERTIES
    IMPORTED_LOCATION "${LZO_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LZO_INCLUDE_DIR}"
)

mark_as_advanced(
    LZO_INCLUDE_DIR
    LZO_LIBRARY
)
